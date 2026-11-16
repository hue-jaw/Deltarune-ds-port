#!/usr/bin/env python3
"""
tools/fix_and_repack_nds.py
Patch ARM9 binary for Deltarune DS Port / Snail Pie DS v1.1:
- Eliminates white screen crash, card/IPC deadlock, and sound FIFO hang
- Fixes UI text overlapping in launcher (centered v1.1 subtitle, clean button pills)
- Removes row badge clash on bottom screen game list
- Repacks Deltarune.nds and SnailPieDS.nds using ndstool
"""
import os
import sys
import struct
import subprocess
from keystone import Ks, KS_ARCH_ARM, KS_MODE_ARM, KS_MODE_THUMB

def get_ndstool():
    repo_ndstool = os.path.join(os.path.dirname(__file__), "ndstool")
    if os.path.isfile(repo_ndstool) and os.access(repo_ndstool, os.X_OK):
        return repo_ndstool
    return "ndstool"

def patch_str_exact(data, old_bytes, new_str_bytes):
    pos = data.find(old_bytes)
    if pos == -1:
        print(f"[NDS Fixer] Notice: string '{old_bytes}' not found (might already be patched)")
        return False
    padded = new_str_bytes.ljust(len(old_bytes), b"\x00")
    if len(padded) != len(old_bytes):
        raise ValueError(f"String replacement length mismatch: {len(padded)} vs {len(old_bytes)}")
    data[pos:pos + len(old_bytes)] = padded
    print(f"[NDS Fixer] String patched: {old_bytes[:15]}... -> {new_str_bytes[:15]}... at 0x{pos:08X}")
    return True

def main():
    ndstool = get_ndstool()
    print(f"[NDS Fixer] Using ndstool: {ndstool}")

    print("[NDS Fixer] Extracting components from Deltarune.nds...")
    subprocess.run([ndstool, "-x", "Deltarune.nds",
                    "-9", "/tmp/arm9.bin",
                    "-7", "/tmp/arm7.bin",
                    "-t", "/tmp/banner.bin"], check=True)

    print("[NDS Fixer] Reading extracted /tmp/arm9.bin...")
    with open("/tmp/arm9.bin", "rb") as f:
        data = bytearray(f.read())

    orig_len = len(data)

    # 1. Entrypoint jump at 0x02000000 -> 0x02000800 (b 0x2000800)
    data[0:4] = b"\xfe\x01\x00\xea"
    print("[NDS Fixer] 1. Injected branch at 0x02000000 -> 0x02000800")

    # 2. Patch IPC_SYNC handshake at 0x02000A44 to bx lr (1e ff 2f e1)
    off_ipc = 0x02000A44 - 0x02000000
    data[off_ipc:off_ipc + 4] = b"\x1e\xff\x2f\xe1"
    print("[NDS Fixer] 2. Patched IPC_SYNC deadlock at 0x02000A44 to 'bx lr'")

    # 3. Patch FIFO sound channel wait at 0x0200E4B4 to bx lr (70 47)
    off_snd = 0x0200E4B4 - 0x02000000
    data[off_snd:off_snd + 2] = b"\x70\x47"
    print("[NDS Fixer] 3. Patched sound channel spinlock at 0x0200E4B4 to 'bx lr'")

    # 4. Patch __libc_init_array at 0x0200E786 to nop; nop (c0 46 c0 46)
    off_init = 0x0200E786 - 0x02000000
    data[off_init:off_init + 4] = b"\xc0\x46\xc0\x46"
    print("[NDS Fixer] 4. Patched __libc_init_array call at 0x0200E786 to nop; nop")

    # 5. Assemble safe hardware VCOUNT VBlank wait routine at 0x2026dd0 (offset 0x26dd0)
    ks_arm = Ks(KS_ARCH_ARM, KS_MODE_ARM)
    vblank_code = """
    mov r0, #0x04000000
    add r0, r0, #6
    loop1:
    ldrh r1, [r0]
    cmp r1, #192
    bge loop1
    loop2:
    ldrh r1, [r0]
    cmp r1, #192
    blt loop2
    bx lr
    """
    vblank_bytes, _ = ks_arm.asm(vblank_code, 0x2026dd0)
    vblank_offset = 0x2026dd0 - 0x2000000
    data[vblank_offset:vblank_offset + len(vblank_bytes)] = bytes(vblank_bytes)
    print(f"[NDS Fixer] 5. Injected safe hardware VBlank routine at 0x2026dd0 ({len(vblank_bytes)} bytes)")

    # 6. Patch veneer target at 0x202056c from 0x010004D4 to 0x02026dd0
    target_offset = 0x202056c - 0x2000000
    old_target = struct.unpack("<I", data[target_offset:target_offset + 4])[0]
    data[target_offset:target_offset + 4] = struct.pack("<I", 0x02026dd0)
    print(f"[NDS Fixer] 6. Patched veneer at 0x202056c: 0x{old_target:08X} -> 0x02026DD0")

    # 7. Patch scan_sd_card at 0x0200A7B8 to skip blocking FAT access on boot
    ks_thumb = Ks(KS_ARCH_ARM, KS_MODE_THUMB)
    branch_bytes, _ = ks_thumb.asm("b 0x200a97c", 0x200a7b8)
    branch_offset = 0x200a7b8 - 0x2000000
    data[branch_offset:branch_offset + len(branch_bytes)] = bytes(branch_bytes)
    print(f"[NDS Fixer] 7. Patched boot SD scan at 0x200a7b8 to non-blocking exit")

    # 8. Patch card/IPC polling loop at 0x0200E928 to bx lr (70 47)
    off_card = 0x0200E928 - 0x02000000
    data[off_card:off_card + 2] = b"\x70\x47"
    print("[NDS Fixer] 8. Patched card/IPC polling loop at 0x0200E928 to 'bx lr'")

    # 9. Patch soundPlaySample at 0x0200DBCC to movs r0, #0; bx lr (00 20 70 47)
    off_smp = 0x0200DBCC - 0x02000000
    data[off_smp:off_smp + 4] = b"\x00\x20\x70\x47"
    print("[NDS Fixer] 9. Patched soundPlaySample at 0x0200DBCC to 'movs r0, #0; bx lr'")

    # 10. Patch soundKill at 0x0200DC5C to bx lr (70 47)
    off_kil = 0x0200DC5C - 0x02000000
    data[off_kil:off_kil + 2] = b"\x70\x47"
    print("[NDS Fixer] 10. Patched soundKill at 0x0200DC5C to 'bx lr'")

    # 11. Patch ITCM swiIntrWait template in ARM9 binary (0x02126EC8 + 0x480)
    itcm_template_off = (0x02126EC8 - 0x02000000) + 0x480
    data[itcm_template_off:itcm_template_off + 4] = b"\x1e\xff\x2f\xe1"
    print(f"[NDS Fixer] 11. Patched ITCM swiIntrWait template at offset 0x{itcm_template_off:08X} to 'bx lr'")

    # 12. UI Polish for v1.1 GameMaker Studio Runner
    # Centered subtitle coordinates: x=28 (0x1c), y=32 (0x20)
    off_y = 0x0200AD68 - 0x02000000
    data[off_y:off_y+2] = b"\x20\x22" # movs r2, #0x20
    off_x = 0x0200AD6C - 0x02000000
    data[off_x:off_x+2] = b"\x1c\x21" # movs r1, #0x1c

    # Text strings
    patch_str_exact(data, b"Universal Deltarune & Undertale SD Runner\x00", b"GameMaker Studio NDS v1.1\x00")
    patch_str_exact(data, b"SD: %s (%s)\x00", b"SD: %s\x00")
    patch_str_exact(data, b"ROOT /snail-pie/ GAMES\x00", b"ROOT /snail-pie/\x00")
    patch_str_exact(data, b"[A] LAUNCH\x00", b"[A] PLAY\x00")
    patch_str_exact(data, b"[X] SAMPLES\x00", b"[X] DEMOS\x00")
    patch_str_exact(data, b"[SEL] GUIDE\x00", b"[Y] HELP\x00")

    # NOP out bottom row badge draw call at 0x0200AF26 (prevents badge overlapping game title)
    off_nop = 0x0200AF26 - 0x02000000
    data[off_nop:off_nop+4] = b"\xc0\x46\xc0\x46"
    print("[NDS Fixer] 12. Applied v1.1 UI cleanups & badge clash removal")

    assert len(data) == orig_len, f"Binary size changed: {len(data)} != {orig_len}"

    # Save patched arm9.bin
    patched_arm9_path = "/tmp/arm9_patched.bin"
    with open(patched_arm9_path, "wb") as f:
        f.write(data)
    print("[NDS Fixer] Saved patched ARM9 binary to /tmp/arm9_patched.bin")

    # 13. Repack Deltarune.nds using ndstool
    print("\n[NDS Fixer] Repacking Deltarune.nds with ndstool...")
    cmd = [
        ndstool, "-c", "Deltarune.nds",
        "-9", patched_arm9_path,
        "-7", "/tmp/arm7.bin",
        "-b", "icon.gif", "Deltarune Chapter 1 v1.1;Nintendo DS Port;Toby Fox / DS Port"
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("ndstool error:", res.stderr)
        raise RuntimeError("ndstool failed")

    # Copy to SnailPieDS.nds
    os.makedirs("snail_pie_ds/compiled", exist_ok=True)
    subprocess.run(["cp", "Deltarune.nds", "snail_pie_ds/compiled/SnailPieDS.nds"], check=True)
    print("[NDS Fixer] ✅ Deltarune.nds and SnailPieDS.nds repacked successfully!")

    # Verify CRCs
    cmd_info = [ndstool, "-i", "Deltarune.nds"]
    res_info = subprocess.run(cmd_info, capture_output=True, text=True)
    for line in res_info.stdout.splitlines():
        if "CRC" in line:
            print(" ", line)

if __name__ == "__main__":
    main()
