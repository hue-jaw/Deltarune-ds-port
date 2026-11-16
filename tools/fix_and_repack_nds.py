#!/usr/bin/env python3
"""
Patch ARM9 binary to eliminate white screen crash, card/IPC deadlock, and sound FIFO hang,
then repack NDS ROM using ndstool.
"""
import struct
import subprocess
from keystone import *
from capstone import *

def main():
    print("[NDS Fixer] Extracting components from Deltarune.nds...")
    subprocess.run(["ndstool", "-x", "Deltarune.nds",
                    "-9", "/tmp/arm9.bin",
                    "-7", "/tmp/arm7.bin",
                    "-t", "/tmp/banner.bin"], check=True)

    print("[NDS Fixer] Reading extracted /tmp/arm9.bin...")
    with open("/tmp/arm9.bin", "rb") as f:
        data = bytearray(f.read())

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
    # Eliminates infinite traversal of uninitialized linked lists at 0x0200E8B8
    # and uninitialized hardware register polling at 0x0200E9F4
    off_card = 0x0200E928 - 0x02000000
    data[off_card:off_card + 2] = b"\x70\x47"
    print("[NDS Fixer] 8. Patched card/IPC polling loop at 0x0200E928 to 'bx lr'")

    # 9. Patch soundPlaySample at 0x0200DBCC to movs r0, #0; bx lr (00 20 70 47)
    # Eliminates synchronous FIFO yield to idle thread when ARM7 is desynchronized
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

    # Save patched arm9.bin
    patched_arm9_path = "/tmp/arm9_patched.bin"
    with open(patched_arm9_path, "wb") as f:
        f.write(data)
    print("[NDS Fixer] Saved patched ARM9 binary to /tmp/arm9_patched.bin")

    # 12. Repack Deltarune.nds using ndstool
    print("\n[NDS Fixer] Repacking Deltarune.nds with ndstool...")
    cmd = [
        "ndstool", "-c", "Deltarune.nds",
        "-9", patched_arm9_path,
        "-7", "/tmp/arm7.bin",
        "-b", "icon.gif", "Deltarune Chapter 1;Nintendo DS Port;Toby Fox / DS Port"
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("ndstool error:", res.stderr)
        raise RuntimeError("ndstool failed")

    # Copy to SnailPieDS.nds
    subprocess.run(["cp", "Deltarune.nds", "snail_pie_ds/compiled/SnailPieDS.nds"], check=True)
    print("[NDS Fixer] ✅ Deltarune.nds and SnailPieDS.nds repacked successfully!")

    # Verify CRCs
    cmd_info = ["ndstool", "-i", "Deltarune.nds"]
    res_info = subprocess.run(cmd_info, capture_output=True, text=True)
    for line in res_info.stdout.splitlines():
        if "CRC" in line:
            print(" ", line)

if __name__ == "__main__":
    main()
