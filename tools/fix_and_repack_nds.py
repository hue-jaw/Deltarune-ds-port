#!/usr/bin/env python3
"""
Patch ARM9 binary to eliminate white screen crash and repack NDS ROM.
"""
import struct
import subprocess
from keystone import *
from capstone import *

def main():
    print("[NDS Fixer] Reading extracted /tmp/arm9.bin...")
    with open("/tmp/arm9.bin", "rb") as f:
        data = bytearray(f.read())

    # 1. Assemble safe hardware VCOUNT VBlank wait routine at 0x2026dd0 (offset 0x26dd0)
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
    print(f"[NDS Fixer] Injected safe hardware VBlank routine at 0x2026dd0 ({len(vblank_bytes)} bytes)")

    # 2. Patch veneer target at 0x202056c from 0x010004D4 to 0x02026dd0
    target_offset = 0x202056c - 0x2000000
    old_target = struct.unpack("<I", data[target_offset:target_offset + 4])[0]
    data[target_offset:target_offset + 4] = struct.pack("<I", 0x02026dd0)
    print(f"[NDS Fixer] Patched veneer at 0x202056c: 0x{old_target:08X} -> 0x02026DD0")

    # 3. Patch scan_sd_card at 0x0200A7B8 to skip blocking FAT access on boot
    ks_thumb = Ks(KS_ARCH_ARM, KS_MODE_THUMB)
    branch_bytes, _ = ks_thumb.asm("b 0x200a97c", 0x200a7b8)
    branch_offset = 0x200a7b8 - 0x2000000
    data[branch_offset:branch_offset + len(branch_bytes)] = bytes(branch_bytes)
    print(f"[NDS Fixer] Patched boot SD scan at 0x200a7b8 to non-blocking exit")

    # Save patched arm9.bin
    patched_arm9_path = "/tmp/arm9_patched.bin"
    with open(patched_arm9_path, "wb") as f:
        f.write(data)
    print("[NDS Fixer] Saved patched ARM9 binary to /tmp/arm9_patched.bin")

    # Verify disassembly
    md_arm = Cs(CS_ARCH_ARM, CS_MODE_ARM)
    print("\n[NDS Fixer] Disassembly of injected routine:")
    for inst in md_arm.disasm(bytes(vblank_bytes), 0x2026dd0):
        print(f"  0x{inst.address:08X}: {inst.mnemonic}\t{inst.op_str}")

    # 4. Repack Deltarune.nds using ndstool
    print("\n[NDS Fixer] Repacking Deltarune.nds with ndstool...")
    ndstool = "/tmp/blocksds-sdk/tools/ndstool/ndstool"
    cmd = [
        ndstool, "-c", "Deltarune.nds",
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
    cmd_info = [ndstool, "-i", "Deltarune.nds"]
    res_info = subprocess.run(cmd_info, capture_output=True, text=True)
    for line in res_info.stdout.splitlines():
        if "CRC" in line:
            print(" ", line)

if __name__ == "__main__":
    main()
