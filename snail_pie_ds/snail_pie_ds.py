#!/usr/bin/env python3
"""
Snail Pie DS (🥧 Snail Pie DS 🥧)
Real-time GameMaker / Deltarune asset pipeline and compiler for Nintendo DS.
Drop files into snail_pie_ds/drop/ and BIM! It compiles in real-time!
"""

import os
import sys
import time
import struct
import bz2
import ctypes
import shutil
import subprocess
from PIL import Image
import soundfile as sf
import numpy as np

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DROP_DIR = os.path.join(BASE_DIR, "snail_pie_ds", "drop")
EXTRACT_DIR = os.path.join(BASE_DIR, "snail_pie_ds", "extracted_sprites")
COMPILED_DIR = os.path.join(BASE_DIR, "snail_pie_ds", "compiled")
LIB_SO = os.path.join(BASE_DIR, "snail_pie_ds", "libqoi_fast.so")

def build_qoi_lib():
    c_path = os.path.join(BASE_DIR, "snail_pie_ds", "qoi_fast.c")
    if not os.path.exists(LIB_SO):
        print("[Snail Pie DS] Building fast QOI decoder library...")
        subprocess.run(["gcc", "-O3", "-shared", "-fPIC", c_path, "-o", LIB_SO], check=True)

build_qoi_lib()
qoi_lib = ctypes.CDLL(LIB_SO)
qoi_lib.decode_gm_qoi.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.c_void_p]
qoi_lib.decode_gm_qoi.restype = ctypes.c_int

def decode_texture_page(data_win_path, blob_offset, block_size):
    with open(data_win_path, "rb") as f:
        f.seek(blob_offset)
        raw = f.read(block_size)

    bz_idx = raw.find(b"BZh")
    if bz_idx == -1:
        return None
    decomp = bz2.decompress(raw[bz_idx:])
    w, h = struct.unpack("<HH", decomp[4:8])
    pixels = bytearray(w * h * 4)
    res = qoi_lib.decode_gm_qoi(decomp, len(decomp), (ctypes.c_char * len(pixels)).from_buffer(pixels))
    if res != 0:
        return None
    return Image.frombytes("RGBA", (w, h), bytes(pixels))

def extract_sprites_from_data_win(data_win_path):
    print(f"[Snail Pie DS] Reading {data_win_path}...")
    with open(data_win_path, "rb") as f:
        buf = f.read()

    # Find chunks
    pos = 8
    chunks = {}
    while pos < len(buf):
        cname = buf[pos:pos+4].decode("ascii", errors="ignore")
        csize = struct.unpack("<I", buf[pos+4:pos+8])[0]
        chunks[cname] = pos + 8
        pos += 8 + csize

    # Parse TXTR
    txtr_pos = chunks.get("TXTR")
    txtr_count = struct.unpack("<I", buf[txtr_pos:txtr_pos+4])[0]
    txtr_ptrs = struct.unpack(f"<{txtr_count}I", buf[txtr_pos+4:txtr_pos+4+txtr_count*4])
    txtr_meta = []
    for p in txtr_ptrs:
        scaled, genmips, texblk, tw, th, idx, blob_offset = struct.unpack("<IIIIIII", buf[p:p+28])
        txtr_meta.append((tw, th, blob_offset, texblk))

    # Parse SPRT
    sprt_pos = chunks.get("SPRT")
    sprt_count = struct.unpack("<I", buf[sprt_pos:sprt_pos+4])[0]
    sprt_ptrs = struct.unpack(f"<{sprt_count}I", buf[sprt_pos+4:sprt_pos+4+sprt_count*4])

    sprite_map = {}
    for p in sprt_ptrs:
        name_off = struct.unpack("<I", buf[p:p+4])[0]
        # Read null-terminated string
        end_str = buf.find(b"\x00", name_off)
        sname = buf[name_off:end_str].decode("utf-8", errors="ignore")

        w, h, ml, mr, mb, mt = struct.unpack("<IIiiii", buf[p+4:p+28])
        trans, smooth, preload, bbox_mode, sep_masks = struct.unpack("<IIIII", buf[p+28:p+48])
        ox, oy = struct.unpack("<ii", buf[p+48:p+56])

        check = struct.unpack("<i", buf[p+56:p+60])[0]
        read_idx = p + 60
        if check == -1:
            sVer, sType = struct.unpack("<II", buf[read_idx:read_idx+8])
            read_idx += 16 # playback_spd, type
            if sVer >= 2: read_idx += 4 # sequenceOffset
            if sVer >= 3: read_idx += 4 # nineSliceOffset
            tex_count = struct.unpack("<I", buf[read_idx:read_idx+4])[0]
            read_idx += 4
        else:
            tex_count = check

        tpag_ptrs = struct.unpack(f"<{tex_count}I", buf[read_idx:read_idx+tex_count*4])
        frames = []
        for tp in tpag_ptrs:
            fx, fy, fw, fh = struct.unpack("<HHHH", buf[tp:tp+8])
            rx, ry, rw, rh = struct.unpack("<hhhh", buf[tp+8:tp+16])
            bw, bh = struct.unpack("<HH", buf[tp+16:tp+20])
            tex_id = struct.unpack("<H", buf[tp+20:tp+22])[0]
            frames.append((tex_id, fx, fy, fw, fh, rx, ry, rw, rh, bw, bh))

        sprite_map[sname] = {
            "w": w, "h": h, "ox": ox, "oy": oy, "frames": frames
        }

    print(f"[Snail Pie DS] Indexed {len(sprite_map)} sprites from data.win!")
    return txtr_meta, sprite_map

def convert_img_to_argb1555(im):
    im = im.convert("RGBA")
    w, h = im.size
    pixels = []
    for y in range(h):
        for x in range(w):
            r, g, b, a = im.getpixel((x, y))
            if a < 64:
                pixels.append(0)
            else:
                r5 = (r >> 3) & 0x1F
                g5 = (g >> 3) & 0x1F
                b5 = (b >> 3) & 0x1F
                val = 0x8000 | r5 | (g5 << 5) | (b5 << 10)
                pixels.append(val)
    return w, h, pixels

def convert_audio(path, max_duration=None, target_rate=11025):
    data, rate = sf.read(path)
    if len(data.shape) > 1:
        data = data.mean(axis=1)
    if max_duration:
        max_samples = int(max_duration * rate)
        data = data[:max_samples]
    num_samples = int(len(data) * target_rate / rate)
    if num_samples <= 0:
        return target_rate, []
    indices = np.linspace(0, len(data) - 1, num_samples).astype(int)
    resampled = data[indices]
    pcm8 = (np.clip(resampled, -1.0, 1.0) * 127).astype(np.int8)
    return target_rate, pcm8.tolist()

def extract_and_generate_assets():
    print("[Snail Pie DS] 🥧 Starting full asset generation...")
    os.makedirs(EXTRACT_DIR, exist_ok=True)
    os.makedirs(COMPILED_DIR, exist_ok=True)

    data_win_path = os.path.join(DROP_DIR, "data.win")
    if not os.path.exists(data_win_path):
        data_win_path = "/tmp/deltarune-extracted/data.win"

    txtr_meta, sprite_map = extract_sprites_from_data_win(data_win_path)

    # Cache decoded texture pages
    cached_pages = {}
    def get_texture_page(page_id):
        if page_id not in cached_pages:
            tw, th, off, blk = txtr_meta[page_id]
            print(f"[Snail Pie DS] Decoding Texture Atlas Page {page_id} ({tw}x{th})...")
            cached_pages[page_id] = decode_texture_page(data_win_path, off, blk)
        return cached_pages[page_id]

    def get_sprite_frame(sname, frame_idx=0):
        if sname not in sprite_map:
            return None
        info = sprite_map[sname]
        if frame_idx >= len(info["frames"]):
            return None
        tex_id, fx, fy, fw, fh, rx, ry, rw, rh, bw, bh = info["frames"][frame_idx]
        atlas = get_texture_page(tex_id)
        if not atlas:
            return None
        crop = atlas.crop((fx, fy, fx + fw, fy + fh))
        return crop

    # Extract & save King, K.Round, Jevil, Lancer, etc.
    boss_sprites = {
        "king_idle": ("spr_chainking_idle", 5),
        "king_hurt": ("spr_chainking_hurt", 1),
        "kround_idle": ("spr_checkers_idle", 4),
        "kround_bow": ("spr_checkers_bow", 1),
        "jevil_dance": ("spr_joker_dance", 8),
        "jevil_tired": ("spr_joker_tired", 1),
        "face_king": ("spr_face_king", 1),
        "face_susie_bangs": ("spr_face_sD", 1),
    }

    extracted_images = {}
    for key, (sname, num_frames) in boss_sprites.items():
        frames_list = []
        for fi in range(num_frames):
            im = get_sprite_frame(sname, fi)
            if im:
                out_png = os.path.join(EXTRACT_DIR, f"{key}_{fi}.png")
                im.save(out_png)
                frames_list.append(im)
        extracted_images[key] = frames_list
        print(f"[Snail Pie DS] Extracted {key} ({len(frames_list)} frames)")

    # Write source/assets_gfx.h
    with open("source/assets_gfx.h", "w") as f:
        f.write("""#ifndef ASSETS_GFX_H
#define ASSETS_GFX_H

#include <nds.h>

typedef struct {
    u16 w, h;
    const u16 *data;
} GfxImage;

// Overworld walk sprites: [direction 0..3][frame 0..3]
extern const GfxImage spr_kris_dw[4][4];
extern const GfxImage spr_susie_dw[4][4];
extern const GfxImage spr_ralsei_dw[4][4];
extern const GfxImage spr_kris_lw[4][4];
extern const GfxImage spr_lancer_dw[4][2];

// Battle party sprites
extern const GfxImage spr_kris_b_idle[2];
extern const GfxImage spr_kris_b_attack[3];
extern const GfxImage spr_kris_b_defend;
extern const GfxImage spr_kris_b_hurt;

extern const GfxImage spr_susie_b_idle[2];
extern const GfxImage spr_susie_b_attack[3];
extern const GfxImage spr_susie_b_defend;
extern const GfxImage spr_susie_b_hurt;

extern const GfxImage spr_ralsei_b_idle[2];
extern const GfxImage spr_ralsei_b_spell[3];
extern const GfxImage spr_ralsei_b_defend;
extern const GfxImage spr_ralsei_b_hurt;

// Enemy sprites
// Rudinn
extern const GfxImage spr_rudinn_b_idle[2];
extern const GfxImage spr_rudinn_b_hurt;

// Lancer
extern const GfxImage spr_lancer_b_idle[2];
extern const GfxImage spr_lancer_b_hurt;

// K. Round (Checker King)
extern const GfxImage spr_kround_b_idle[4];
extern const GfxImage spr_kround_b_bow;

// King (Chaos King)
extern const GfxImage spr_king_b_idle[5];
extern const GfxImage spr_king_b_hurt;

// Jevil
extern const GfxImage spr_jevil_b_idle[4];
extern const GfxImage spr_jevil_b_attack;
extern const GfxImage spr_jevil_b_hurt;

// Soul & Bullets
extern const GfxImage spr_soul;
extern const GfxImage spr_soul_graze;
extern const GfxImage spr_bullet_spade;
extern const GfxImage spr_bullet_diamond;
extern const GfxImage spr_bullet_scythe;
extern const GfxImage spr_bullet_pellet;

// UI Buttons
extern const GfxImage spr_btn[5][3];

// Portraits
extern const GfxImage spr_face_susie_neutral;
extern const GfxImage spr_face_susie_smile;
extern const GfxImage spr_face_susie_angry;
extern const GfxImage spr_face_ralsei_neutral;
extern const GfxImage spr_face_ralsei_smile;
extern const GfxImage spr_face_ralsei_blush;
extern const GfxImage spr_face_lancer_smile;
extern const GfxImage spr_face_toriel_warm;
extern const GfxImage spr_face_king_furious;

// Backgrounds / patterns
extern const GfxImage bg_battle_tile;
extern const GfxImage bg_field_banner;

#endif
""")

    # Write source/assets_gfx.c
    with open("source/assets_gfx.c", "w") as gfx_c:
        gfx_c.write('#include "assets_gfx.h"\n\n')

        def emit_data(name, im):
            w, h, px = convert_img_to_argb1555(im)
            gfx_c.write(f"static const u16 {name}[{len(px)}] = {{\n")
            for i in range(0, len(px), 12):
                gfx_c.write("    " + ", ".join(f"0x{p:04X}" for p in px[i:i+12]) + ",\n")
            gfx_c.write("};\n\n")
            return w, h

        def emit_standalone(var_name, im):
            dname = f"data_{var_name}"
            w, h = emit_data(dname, im)
            gfx_c.write(f"const GfxImage {var_name} = {{ {w}, {h}, {dname} }};\n\n")

        # 1. Overworld walks
        dirs = ['down', 'left', 'right', 'up']
        for char, folder in [
            ('kris_dw', '/tmp/kristal/assets/sprites/party/kris/dark/walk'),
            ('susie_dw', '/tmp/kristal/assets/sprites/party/susie/dark/walk'),
            ('ralsei_dw', '/tmp/kristal/assets/sprites/party/ralsei/dark/walk'),
            ('kris_lw', '/tmp/kristal/assets/sprites/party/kris/light/walk'),
        ]:
            meta = []
            for d_idx, d in enumerate(dirs):
                d_meta = []
                for f_idx in range(1, 5):
                    im = Image.open(f"{folder}/{d}_{f_idx}.png")
                    dn = f"data_{char}_{d_idx}_{f_idx-1}"
                    w, h = emit_data(dn, im)
                    d_meta.append((w, h, dn))
                meta.append(d_meta)
            gfx_c.write(f"const GfxImage spr_{char}[4][4] = {{\n")
            for d_idx in range(4):
                gfx_c.write("    { " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w,h,dn) in meta[d_idx]) + " },\n")
            gfx_c.write("};\n\n")

        # Lancer walk
        l_meta = []
        for d_idx, d in enumerate(dirs):
            im = Image.open(f"/tmp/kristal/assets/sprites/kristal/lancer/walk/{d}.png").resize((32, 32), Image.NEAREST)
            dn = f"data_lancer_dw_{d_idx}"
            w, h = emit_data(dn, im)
            l_meta.append((w, h, dn))
        gfx_c.write("const GfxImage spr_lancer_dw[4][2] = {\n")
        for d_idx in range(4):
            w, h, dn = l_meta[d_idx]
            gfx_c.write(f"    {{ {{ {w}, {h}, {dn} }}, {{ {w}, {h}, {dn} }} }},\n")
        gfx_c.write("};\n\n")

        # Party Battle
        # Kris
        k_idle = []
        for i, p in enumerate(['idle_1.png', 'idle_2.png']):
            dn = f"data_kris_b_idle_{i}"
            w, h = emit_data(dn, Image.open(f"/tmp/kristal/assets/sprites/party/kris/dark/battle/{p}"))
            k_idle.append((w, h, dn))
        gfx_c.write("const GfxImage spr_kris_b_idle[2] = {\n    " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w,h,dn) in k_idle) + "\n};\n\n")

        k_atk = []
        for i, p in enumerate(['attack_1.png', 'attack_3.png', 'attack_5.png']):
            dn = f"data_kris_b_atk_{i}"
            w, h = emit_data(dn, Image.open(f"/tmp/kristal/assets/sprites/party/kris/dark/battle/{p}"))
            k_atk.append((w, h, dn))
        gfx_c.write("const GfxImage spr_kris_b_attack[3] = {\n    " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w,h,dn) in k_atk) + "\n};\n\n")

        emit_standalone("spr_kris_b_defend", Image.open('/tmp/kristal/assets/sprites/party/kris/dark/battle/defend_1.png'))
        emit_standalone("spr_kris_b_hurt", Image.open('/tmp/kristal/assets/sprites/party/kris/dark/battle/hurt_1.png'))

        # Susie
        s_idle = []
        for i, p in enumerate(['idle_1.png', 'idle_2.png']):
            dn = f"data_susie_b_idle_{i}"
            w, h = emit_data(dn, Image.open(f"/tmp/kristal/assets/sprites/party/susie/dark/battle/{p}"))
            s_idle.append((w, h, dn))
        gfx_c.write("const GfxImage spr_susie_b_idle[2] = {\n    " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w,h,dn) in s_idle) + "\n};\n\n")

        s_atk = []
        for i, p in enumerate(['attack_1.png', 'attack_3.png', 'attack_5.png']):
            dn = f"data_susie_b_atk_{i}"
            w, h = emit_data(dn, Image.open(f"/tmp/kristal/assets/sprites/party/susie/dark/battle/{p}"))
            s_atk.append((w, h, dn))
        gfx_c.write("const GfxImage spr_susie_b_attack[3] = {\n    " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w,h,dn) in s_atk) + "\n};\n\n")

        emit_standalone("spr_susie_b_defend", Image.open('/tmp/kristal/assets/sprites/party/susie/dark/battle/defend_1.png'))
        emit_standalone("spr_susie_b_hurt", Image.open('/tmp/kristal/assets/sprites/party/susie/dark/battle/hurt_1.png'))

        # Ralsei
        r_idle = []
        for i, p in enumerate(['idle_1.png', 'idle_2.png']):
            dn = f"data_ralsei_b_idle_{i}"
            w, h = emit_data(dn, Image.open(f"/tmp/kristal/assets/sprites/party/ralsei/dark/battle/{p}"))
            r_idle.append((w, h, dn))
        gfx_c.write("const GfxImage spr_ralsei_b_idle[2] = {\n    " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w,h,dn) in r_idle) + "\n};\n\n")

        r_spl = []
        for i, p in enumerate(['spell_01.png', 'spell_04.png', 'spell_07.png']):
            dn = f"data_ralsei_b_spl_{i}"
            w, h = emit_data(dn, Image.open(f"/tmp/kristal/assets/sprites/party/ralsei/dark/battle/{p}"))
            r_spl.append((w, h, dn))
        gfx_c.write("const GfxImage spr_ralsei_b_spell[3] = {\n    " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w,h,dn) in r_spl) + "\n};\n\n")

        emit_standalone("spr_ralsei_b_defend", Image.open('/tmp/kristal/assets/sprites/party/ralsei/dark/battle/defend_1.png'))
        emit_standalone("spr_ralsei_b_hurt", Image.open('/tmp/kristal/assets/sprites/party/ralsei/dark/battle/hurt_1.png'))

        # Rudinn
        im_rud1 = Image.open('/tmp/battle-engine/sprites/spr_testenemy/458b654e-6f01-4bcf-85f1-44329049f3db.png')
        im_rud2 = Image.open('/tmp/battle-engine/sprites/spr_testenemy/7e147e21-f183-407c-b1d0-d4d10fce12ab.png')
        w1, h1 = emit_data("data_rudinn_b_idle_0", im_rud1)
        w2, h2 = emit_data("data_rudinn_b_idle_1", im_rud2)
        gfx_c.write(f"const GfxImage spr_rudinn_b_idle[2] = {{\n    {{ {w1}, {h1}, data_rudinn_b_idle_0 }}, {{ {w2}, {h2}, data_rudinn_b_idle_1 }}\n}};\n\n")
        emit_standalone("spr_rudinn_b_hurt", im_rud2)

        # Lancer
        im_lan_i = Image.open('/tmp/battle-engine/sprites/spr_lancer_idle/e1ac897d-83ff-4deb-a5c4-fc857d331713.png')
        im_lan_h = Image.open('/tmp/battle-engine/sprites/spr_lancer_hurt/ae7294fc-c9df-4e8a-9689-9c6fcd240343.png')
        lw, lh = emit_data("data_lancer_b_idle", im_lan_i)
        gfx_c.write(f"const GfxImage spr_lancer_b_idle[2] = {{\n    {{ {lw}, {lh}, data_lancer_b_idle }}, {{ {lw}, {lh}, data_lancer_b_idle }}\n}};\n\n")
        emit_standalone("spr_lancer_b_hurt", im_lan_h)

        # K. Round (Checker King)
        kr_meta = []
        for i, im in enumerate(extracted_images.get("kround_idle", [])):
            dn = f"data_kround_b_idle_{i}"
            w, h = emit_data(dn, im)
            kr_meta.append((w, h, dn))
        gfx_c.write("const GfxImage spr_kround_b_idle[4] = {\n    " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w,h,dn) in kr_meta) + "\n};\n\n")
        bow_im = extracted_images.get("kround_bow", [extracted_images["kround_idle"][0]])[0]
        emit_standalone("spr_kround_b_bow", bow_im)

        # King (Chaos King)
        king_meta = []
        for i, im in enumerate(extracted_images.get("king_idle", [])):
            dn = f"data_king_b_idle_{i}"
            w, h = emit_data(dn, im)
            king_meta.append((w, h, dn))
        gfx_c.write("const GfxImage spr_king_b_idle[5] = {\n    " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w,h,dn) in king_meta) + "\n};\n\n")
        king_hurt_im = extracted_images.get("king_hurt", [extracted_images["king_idle"][0]])[0]
        emit_standalone("spr_king_b_hurt", king_hurt_im)

        # Jevil
        jv_meta = []
        for i, im in enumerate(extracted_images.get("jevil_dance", [])[:4]):
            dn = f"data_jevil_b_idle_{i}"
            w, h = emit_data(dn, im)
            jv_meta.append((w, h, dn))
        gfx_c.write("const GfxImage spr_jevil_b_idle[4] = {\n    " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w,h,dn) in jv_meta) + "\n};\n\n")
        jv_tired = extracted_images.get("jevil_tired", [extracted_images["jevil_dance"][0]])[0]
        emit_standalone("spr_jevil_b_hurt", jv_tired)
        emit_standalone("spr_jevil_b_attack", jv_meta[1][2] and Image.open('/tmp/jevil-sim/src/sprites/jevil-teleport.png').crop((0,0,43,41)))

        # Soul & Bullets
        emit_standalone("spr_soul", Image.open('/tmp/kristal/assets/sprites/player/heart.png'))
        emit_standalone("spr_soul_graze", Image.open('/tmp/kristal/assets/sprites/player/graze.png').resize((24, 24), Image.BILINEAR))
        emit_standalone("spr_bullet_spade", Image.open('/tmp/battle-engine/sprites/spr_bullet/e92f4304-5ade-4d51-aa4f-ea50aaeb6f13.png').resize((18, 17), Image.BILINEAR))
        emit_standalone("spr_bullet_diamond", Image.open('/tmp/jevil-sim/src/sprites/bullets/bullet-diamond.png'))
        emit_standalone("spr_bullet_scythe", Image.open('/tmp/jevil-sim/src/sprites/bullets/Devilsknife.png').resize((28, 28), Image.BILINEAR))

        im_pellet = Image.new('RGBA', (8, 8), (0, 0, 0, 0))
        for py in range(8):
            for px in range(8):
                if (px-3.5)**2 + (py-3.5)**2 <= 12: im_pellet.putpixel((px, py), (255, 255, 255, 255))
        emit_standalone("spr_bullet_pellet", im_pellet)

        # Buttons
        btn_names = ['fight', 'act', 'item', 'spare', 'defend']
        b_meta = []
        for b in btn_names:
            s_list = []
            for st in ['', '_h', '_a']:
                im = Image.open(f"/tmp/kristal/assets/sprites/ui/battle/btn/{b}{st}.png")
                dn = f"data_btn_{b}_{st or 'n'}"
                w, h = emit_data(dn, im)
                s_list.append((w, h, dn))
            b_meta.append(s_list)
        gfx_c.write("const GfxImage spr_btn[5][3] = {\n")
        for st_list in b_meta:
            gfx_c.write("    { " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w,h,dn) in st_list) + " },\n")
        gfx_c.write("};\n\n")

        # Portraits
        emit_standalone("spr_face_susie_neutral", Image.open('/tmp/kristal/assets/sprites/face/susie/neutral.png').resize((40, 44), Image.BILINEAR))
        emit_standalone("spr_face_susie_smile", Image.open('/tmp/kristal/assets/sprites/face/susie/smile.png').resize((40, 44), Image.BILINEAR))
        emit_standalone("spr_face_susie_angry", Image.open('/tmp/kristal/assets/sprites/face/susie/angry.png').resize((40, 44), Image.BILINEAR))
        emit_standalone("spr_face_ralsei_neutral", Image.open('/tmp/kristal/assets/sprites/face/ralsei_hat/neutral.png').resize((42, 42), Image.BILINEAR))
        emit_standalone("spr_face_ralsei_smile", Image.open('/tmp/kristal/assets/sprites/face/ralsei_hat/smile.png').resize((42, 42), Image.BILINEAR))
        emit_standalone("spr_face_ralsei_blush", Image.open('/tmp/kristal/assets/sprites/face/ralsei_hat/blush.png').resize((42, 42), Image.BILINEAR))
        emit_standalone("spr_face_lancer_smile", Image.open('/tmp/battle-engine/sprites/spr_lancer_idle/e1ac897d-83ff-4deb-a5c4-fc857d331713.png').resize((40, 40), Image.NEAREST))

        im_king_face = extracted_images.get("face_king", [Image.new('RGBA', (40,40), (40,40,60,255))])[0].resize((42, 42), Image.BILINEAR)
        emit_standalone("spr_face_king_furious", im_king_face)

        # Toriel portrait
        im_toriel = Image.new('RGBA', (40, 40), (0, 0, 0, 0))
        for py in range(40):
            for px in range(40):
                dx = (px - 20) / 13.0
                dy = (py - 22) / 14.0
                if dx*dx + dy*dy <= 1.0: im_toriel.putpixel((px, py), (245, 245, 250, 255))
                if (px in range(11, 15) or px in range(25, 29)) and py in range(5, 12): im_toriel.putpixel((px, py), (230, 220, 180, 255))
                if (px in [15, 16] or px in [23, 24]) and py in [18, 19]: im_toriel.putpixel((px, py), (40, 20, 10, 255))
                if px in range(18, 22) and py == 24: im_toriel.putpixel((px, py), (180, 180, 190, 255))
                if (px in range(17, 23) and py == 27) or (px in [16, 23] and py == 26): im_toriel.putpixel((px, py), (80, 40, 40, 255))
                if dy > 0.8 and abs(dx) < 1.1: im_toriel.putpixel((px, py), (140, 40, 150, 255))
        emit_standalone("spr_face_toriel_warm", im_toriel)

        # Backgrounds
        emit_standalone("bg_battle_tile", Image.open('/tmp/battle-engine/sprites/spr_battleback/d6ad0b26-42d2-4dae-a2d6-f05bcc79cc38.png'))
        emit_standalone("bg_field_banner", Image.open('/tmp/unity-engine/Assets/Sprites/background/field.png').resize((256, 112), Image.BILINEAR))

    print("[Snail Pie DS] assets_gfx generated successfully!")

def compile_nds():
    print("[Snail Pie DS] ⚡ Compiling Nintendo DS ROM...")
    res = subprocess.run(["make", "-j4"], cwd=BASE_DIR, capture_output=True, text=True)
    if res.returncode != 0:
        print("[Snail Pie DS] ❌ Compilation error:")
        print(res.stderr)
        return False
    print("[Snail Pie DS] ✅ Deltarune.nds compiled successfully!")
    out_rom = os.path.join(COMPILED_DIR, "SnailPieDS.nds")
    shutil.copyfile(os.path.join(BASE_DIR, "Deltarune.nds"), out_rom)
    print(f"[Snail Pie DS] 🥧 SnailPieDS.nds saved to {out_rom}!")
    return True

def watch_drop_folder():
    print(f"[Snail Pie DS] 👁️  Watching {DROP_DIR} in real-time...")
    print("[Snail Pie DS] Drop any GameMaker or Deltarune asset file into this folder and BIM! Real-time compile!")
    mtimes = {}

    def get_dir_state():
        state = {}
        for root, dirs, files in os.walk(DROP_DIR):
            for f in files:
                p = os.path.join(root, f)
                try:
                    state[p] = os.path.getmtime(p)
                except OSError:
                    pass
        return state

    mtimes = get_dir_state()

    while True:
        time.sleep(1)
        cur = get_dir_state()
        if cur != mtimes:
            print("\n[Snail Pie DS] 🔔 Detected change in drop/ folder! BIM! Compiling in real-time...")
            mtimes = cur
            try:
                extract_and_generate_assets()
                compile_nds()
                print("[Snail Pie DS] 🎉 Build complete! Ready to play on Nintendo DS!")
            except Exception as e:
                print(f"[Snail Pie DS] ❌ Error during live compilation: {e}")

def main():
    if "--watch" in sys.argv:
        extract_and_generate_assets()
        compile_nds()
        watch_drop_folder()
    elif "--extract" in sys.argv:
        extract_and_generate_assets()
    else:
        # Default compile
        extract_and_generate_assets()
        compile_nds()

if __name__ == "__main__":
    main()
