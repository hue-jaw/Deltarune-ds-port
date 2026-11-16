#!/usr/bin/env python3
import os
import sys
import numpy as np
from PIL import Image
import soundfile as sf

def convert_image(im):
    im = im.convert('RGBA')
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
        data = data.mean(axis=1) # stereo to mono
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

def main():
    print("Processing Deltarune assets for Nintendo DS...")
    os.makedirs('source', exist_ok=True)

    gfx_c = open('source/assets_gfx.c', 'w')
    gfx_h = open('source/assets_gfx.h', 'w')
    audio_c = open('source/assets_audio.c', 'w')
    audio_h = open('source/assets_audio.h', 'w')

    # Headers
    gfx_h.write("""#ifndef ASSETS_GFX_H
#define ASSETS_GFX_H

#include <nds.h>

typedef struct {
    u16 w, h;
    const u16 *data;
} GfxImage;

// Overworld walk sprites: [direction 0..3][frame 0..3]
// 0=Down, 1=Left, 2=Right, 3=Up
extern const GfxImage spr_kris_dw[4][4];
extern const GfxImage spr_susie_dw[4][4];
extern const GfxImage spr_ralsei_dw[4][4];
extern const GfxImage spr_kris_lw[4][4];
extern const GfxImage spr_lancer_dw[4][2];

// Battle player sprites
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
extern const GfxImage spr_lancer_b_idle[2];
extern const GfxImage spr_lancer_b_hurt;

extern const GfxImage spr_jevil_b_idle[2];
extern const GfxImage spr_jevil_b_attack;
extern const GfxImage spr_jevil_b_hurt;

extern const GfxImage spr_rudinn_b_idle[2];
extern const GfxImage spr_rudinn_b_hurt;

// Soul & Bullets
extern const GfxImage spr_soul;
extern const GfxImage spr_soul_graze;
extern const GfxImage spr_bullet_spade;
extern const GfxImage spr_bullet_diamond;
extern const GfxImage spr_bullet_scythe;
extern const GfxImage spr_bullet_pellet;

// UI Buttons (FIGHT=0, ACT=1, ITEM=2, SPARE=3, DEFEND=4), [state 0=norm, 1=high, 2=active]
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

// Backgrounds / patterns
extern const GfxImage bg_battle_tile;
extern const GfxImage bg_field_banner;

#endif
""")

    gfx_c.write("""#include "assets_gfx.h"\n\n""")

    audio_h.write("""#ifndef ASSETS_AUDIO_H
#define ASSETS_AUDIO_H

#include <nds.h>

typedef struct {
    const s8 *data;
    u32 size;
    u16 freq;
} AudioSample;

extern const AudioSample snd_text_default;
extern const AudioSample snd_text_susie;
extern const AudioSample snd_text_ralsei;
extern const AudioSample snd_text_toriel;
extern const AudioSample snd_menu_switch;
extern const AudioSample snd_menu_confirm;
extern const AudioSample snd_attack;
extern const AudioSample snd_damage;
extern const AudioSample snd_hurt;
extern const AudioSample snd_graze;
extern const AudioSample snd_spell;
extern const AudioSample snd_door;
extern const AudioSample snd_bell;
extern const AudioSample snd_jevil_chaos;
extern const AudioSample snd_jevil_anything;

extern const AudioSample bgm_dark_world;
extern const AudioSample bgm_rude_buster;
extern const AudioSample bgm_jevil;

#endif
""")

    audio_c.write("""#include "assets_audio.h"\n\n""")

    def emit_gfx_data(data_name, im):
        w, h, px = convert_image(im)
        gfx_c.write(f"static const u16 {data_name}[{len(px)}] = {{\n")
        for i in range(0, len(px), 12):
            chunk = px[i:i+12]
            gfx_c.write("    " + ", ".join(f"0x{p:04X}" for p in chunk) + ",\n")
        gfx_c.write("};\n\n")
        return w, h

    def emit_standalone_gfx(var_name, im):
        data_name = f"data_{var_name}"
        w, h = emit_gfx_data(data_name, im)
        gfx_c.write(f"const GfxImage {var_name} = {{ {w}, {h}, {data_name} }};\n\n")

    def emit_audio(var_name, path, max_dur=None, rate=11025):
        sr, samples = convert_audio(path, max_dur, rate)
        audio_c.write(f"static const s8 data_{var_name}[{len(samples)}] = {{\n")
        for i in range(0, len(samples), 16):
            chunk = samples[i:i+16]
            audio_c.write("    " + ", ".join(str(s) for s in chunk) + ",\n")
        audio_c.write("};\n")
        audio_c.write(f"const AudioSample {var_name} = {{ data_{var_name}, {len(samples)}, {sr} }};\n\n")

    # 1. Overworld walk sprites
    dirs = ['down', 'left', 'right', 'up']
    print("Converting Overworld Walk Sprites...")
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
                p = f"{folder}/{d}_{f_idx}.png"
                im = Image.open(p)
                dname = f"data_{char}_{d_idx}_{f_idx-1}"
                w, h = emit_gfx_data(dname, im)
                d_meta.append((w, h, dname))
            meta.append(d_meta)

        gfx_c.write(f"const GfxImage spr_{char}[4][4] = {{\n")
        for d_idx in range(4):
            line = ", ".join(f"{{ {w}, {h}, {dn} }}" for (w, h, dn) in meta[d_idx])
            gfx_c.write(f"    {{ {line} }},\n")
        gfx_c.write("};\n\n")

    # Lancer walk
    lancer_meta = []
    for d_idx, d in enumerate(dirs):
        p = f"/tmp/kristal/assets/sprites/kristal/lancer/walk/{d}.png"
        im = Image.open(p).resize((32, 32), Image.NEAREST)
        dname = f"data_lancer_dw_{d_idx}"
        w, h = emit_gfx_data(dname, im)
        lancer_meta.append((w, h, dname))
    gfx_c.write("const GfxImage spr_lancer_dw[4][2] = {\n")
    for d_idx in range(4):
        w, h, dn = lancer_meta[d_idx]
        gfx_c.write(f"    {{ {{ {w}, {h}, {dn} }}, {{ {w}, {h}, {dn} }} }},\n")
    gfx_c.write("};\n\n")

    # 2. Battle sprites
    print("Converting Battle Sprites...")
    # Kris battle
    kris_idle_meta = []
    for idx, p in enumerate(['idle_1.png', 'idle_2.png']):
        dname = f"data_kris_b_idle_{idx}"
        w, h = emit_gfx_data(dname, Image.open(f"/tmp/kristal/assets/sprites/party/kris/dark/battle/{p}"))
        kris_idle_meta.append((w, h, dname))
    gfx_c.write("const GfxImage spr_kris_b_idle[2] = {\n    " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w, h, dn) in kris_idle_meta) + "\n};\n\n")

    kris_atk_meta = []
    for idx, p in enumerate(['attack_1.png', 'attack_3.png', 'attack_5.png']):
        dname = f"data_kris_b_atk_{idx}"
        w, h = emit_gfx_data(dname, Image.open(f"/tmp/kristal/assets/sprites/party/kris/dark/battle/{p}"))
        kris_atk_meta.append((w, h, dname))
    gfx_c.write("const GfxImage spr_kris_b_attack[3] = {\n    " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w, h, dn) in kris_atk_meta) + "\n};\n\n")

    emit_standalone_gfx("spr_kris_b_defend", Image.open('/tmp/kristal/assets/sprites/party/kris/dark/battle/defend_1.png'))
    emit_standalone_gfx("spr_kris_b_hurt", Image.open('/tmp/kristal/assets/sprites/party/kris/dark/battle/hurt_1.png'))

    # Susie battle
    susie_idle_meta = []
    for idx, p in enumerate(['idle_1.png', 'idle_2.png']):
        dname = f"data_susie_b_idle_{idx}"
        w, h = emit_gfx_data(dname, Image.open(f"/tmp/kristal/assets/sprites/party/susie/dark/battle/{p}"))
        susie_idle_meta.append((w, h, dname))
    gfx_c.write("const GfxImage spr_susie_b_idle[2] = {\n    " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w, h, dn) in susie_idle_meta) + "\n};\n\n")

    susie_atk_meta = []
    for idx, p in enumerate(['attack_1.png', 'attack_3.png', 'attack_5.png']):
        dname = f"data_susie_b_atk_{idx}"
        w, h = emit_gfx_data(dname, Image.open(f"/tmp/kristal/assets/sprites/party/susie/dark/battle/{p}"))
        susie_atk_meta.append((w, h, dname))
    gfx_c.write("const GfxImage spr_susie_b_attack[3] = {\n    " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w, h, dn) in susie_atk_meta) + "\n};\n\n")

    emit_standalone_gfx("spr_susie_b_defend", Image.open('/tmp/kristal/assets/sprites/party/susie/dark/battle/defend_1.png'))
    emit_standalone_gfx("spr_susie_b_hurt", Image.open('/tmp/kristal/assets/sprites/party/susie/dark/battle/hurt_1.png'))

    # Ralsei battle
    ralsei_idle_meta = []
    for idx, p in enumerate(['idle_1.png', 'idle_2.png']):
        dname = f"data_ralsei_b_idle_{idx}"
        w, h = emit_gfx_data(dname, Image.open(f"/tmp/kristal/assets/sprites/party/ralsei/dark/battle/{p}"))
        ralsei_idle_meta.append((w, h, dname))
    gfx_c.write("const GfxImage spr_ralsei_b_idle[2] = {\n    " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w, h, dn) in ralsei_idle_meta) + "\n};\n\n")

    ralsei_spl_meta = []
    for idx, p in enumerate(['spell_01.png', 'spell_04.png', 'spell_07.png']):
        dname = f"data_ralsei_b_spl_{idx}"
        w, h = emit_gfx_data(dname, Image.open(f"/tmp/kristal/assets/sprites/party/ralsei/dark/battle/{p}"))
        ralsei_spl_meta.append((w, h, dname))
    gfx_c.write("const GfxImage spr_ralsei_b_spell[3] = {\n    " + ", ".join(f"{{ {w}, {h}, {dn} }}" for (w, h, dn) in ralsei_spl_meta) + "\n};\n\n")

    emit_standalone_gfx("spr_ralsei_b_defend", Image.open('/tmp/kristal/assets/sprites/party/ralsei/dark/battle/defend_1.png'))
    emit_standalone_gfx("spr_ralsei_b_hurt", Image.open('/tmp/kristal/assets/sprites/party/ralsei/dark/battle/hurt_1.png'))

    # Lancer battle
    im_lancer_idle = Image.open('/tmp/battle-engine/sprites/spr_lancer_idle/e1ac897d-83ff-4deb-a5c4-fc857d331713.png')
    im_lancer_hurt = Image.open('/tmp/battle-engine/sprites/spr_lancer_hurt/ae7294fc-c9df-4e8a-9689-9c6fcd240343.png')
    w, h = emit_gfx_data("data_lancer_b_idle", im_lancer_idle)
    gfx_c.write(f"const GfxImage spr_lancer_b_idle[2] = {{\n    {{ {w}, {h}, data_lancer_b_idle }}, {{ {w}, {h}, data_lancer_b_idle }}\n}};\n\n")
    emit_standalone_gfx("spr_lancer_b_hurt", im_lancer_hurt)

    # Rudinn battle
    im_rudinn1 = Image.open('/tmp/battle-engine/sprites/spr_testenemy/458b654e-6f01-4bcf-85f1-44329049f3db.png')
    im_rudinn2 = Image.open('/tmp/battle-engine/sprites/spr_testenemy/7e147e21-f183-407c-b1d0-d4d10fce12ab.png')
    w1, h1 = emit_gfx_data("data_rudinn_b_idle_0", im_rudinn1)
    w2, h2 = emit_gfx_data("data_rudinn_b_idle_1", im_rudinn2)
    gfx_c.write(f"const GfxImage spr_rudinn_b_idle[2] = {{\n    {{ {w1}, {h1}, data_rudinn_b_idle_0 }}, {{ {w2}, {h2}, data_rudinn_b_idle_1 }}\n}};\n\n")
    emit_standalone_gfx("spr_rudinn_b_hurt", im_rudinn2)

    # Jevil battle
    im_jv_jump = Image.open('/tmp/jevil-sim/src/sprites/jevil-jumping.png').convert('RGBA')
    f0 = im_jv_jump.crop((0, 0, 43, 48))
    f1 = im_jv_jump.crop((43, 0, 86, 48))
    w0, h0 = emit_gfx_data("data_jevil_b_idle_0", f0)
    w1, h1 = emit_gfx_data("data_jevil_b_idle_1", f1)
    gfx_c.write(f"const GfxImage spr_jevil_b_idle[2] = {{\n    {{ {w0}, {h0}, data_jevil_b_idle_0 }}, {{ {w1}, {h1}, data_jevil_b_idle_1 }}\n}};\n\n")

    im_jv_hit = Image.open('/tmp/jevil-sim/src/sprites/jevil-hit.png').convert('RGBA').crop((0, 0, 59, 28))
    emit_standalone_gfx("spr_jevil_b_hurt", im_jv_hit)
    im_jv_tp = Image.open('/tmp/jevil-sim/src/sprites/jevil-teleport.png').convert('RGBA').crop((0, 0, 43, 41))
    emit_standalone_gfx("spr_jevil_b_attack", im_jv_tp)

    # Soul & Bullets
    print("Converting Soul and Bullets...")
    emit_standalone_gfx("spr_soul", Image.open('/tmp/kristal/assets/sprites/player/heart.png'))
    im_grz = Image.open('/tmp/kristal/assets/sprites/player/graze.png').resize((24, 24), Image.BILINEAR)
    emit_standalone_gfx("spr_soul_graze", im_grz)

    im_spade = Image.open('/tmp/battle-engine/sprites/spr_bullet/e92f4304-5ade-4d51-aa4f-ea50aaeb6f13.png').resize((18, 17), Image.BILINEAR)
    emit_standalone_gfx("spr_bullet_spade", im_spade)

    im_diamond = Image.open('/tmp/jevil-sim/src/sprites/bullets/bullet-diamond.png')
    emit_standalone_gfx("spr_bullet_diamond", im_diamond)

    im_scythe = Image.open('/tmp/jevil-sim/src/sprites/bullets/Devilsknife.png').resize((28, 28), Image.BILINEAR)
    emit_standalone_gfx("spr_bullet_scythe", im_scythe)

    im_pellet = Image.new('RGBA', (8, 8), (0, 0, 0, 0))
    for py in range(8):
        for px in range(8):
            if (px-3.5)**2 + (py-3.5)**2 <= 12:
                im_pellet.putpixel((px, py), (255, 255, 255, 255))
    emit_standalone_gfx("spr_bullet_pellet", im_pellet)

    # UI Buttons
    print("Converting Buttons...")
    btn_names = ['fight', 'act', 'item', 'spare', 'defend']
    btn_meta = []
    for b in btn_names:
        norm = Image.open(f"/tmp/kristal/assets/sprites/ui/battle/btn/{b}.png")
        high = Image.open(f"/tmp/kristal/assets/sprites/ui/battle/btn/{b}_h.png")
        act = Image.open(f"/tmp/kristal/assets/sprites/ui/battle/btn/{b}_a.png")
        w0, h0 = emit_gfx_data(f"data_btn_{b}_0", norm)
        w1, h1 = emit_gfx_data(f"data_btn_{b}_1", high)
        w2, h2 = emit_gfx_data(f"data_btn_{b}_2", act)
        btn_meta.append([(w0, h0, f"data_btn_{b}_0"), (w1, h1, f"data_btn_{b}_1"), (w2, h2, f"data_btn_{b}_2")])

    gfx_c.write("const GfxImage spr_btn[5][3] = {\n")
    for states in btn_meta:
        line = ", ".join(f"{{ {w}, {h}, {dn} }}" for (w, h, dn) in states)
        gfx_c.write(f"    {{ {line} }},\n")
    gfx_c.write("};\n\n")

    # Portraits
    print("Converting Portraits...")
    emit_standalone_gfx("spr_face_susie_neutral", Image.open('/tmp/kristal/assets/sprites/face/susie/neutral.png').resize((40, 44), Image.BILINEAR))
    emit_standalone_gfx("spr_face_susie_smile", Image.open('/tmp/kristal/assets/sprites/face/susie/smile.png').resize((40, 44), Image.BILINEAR))
    emit_standalone_gfx("spr_face_susie_angry", Image.open('/tmp/kristal/assets/sprites/face/susie/angry.png').resize((40, 44), Image.BILINEAR))

    emit_standalone_gfx("spr_face_ralsei_neutral", Image.open('/tmp/kristal/assets/sprites/face/ralsei_hat/neutral.png').resize((42, 42), Image.BILINEAR))
    emit_standalone_gfx("spr_face_ralsei_smile", Image.open('/tmp/kristal/assets/sprites/face/ralsei_hat/smile.png').resize((42, 42), Image.BILINEAR))
    emit_standalone_gfx("spr_face_ralsei_blush", Image.open('/tmp/kristal/assets/sprites/face/ralsei_hat/blush.png').resize((42, 42), Image.BILINEAR))

    emit_standalone_gfx("spr_face_lancer_smile", Image.open('/tmp/battle-engine/sprites/spr_lancer_idle/e1ac897d-83ff-4deb-a5c4-fc857d331713.png').resize((40, 40), Image.NEAREST))

    im_toriel = Image.new('RGBA', (40, 40), (0, 0, 0, 0))
    for py in range(40):
        for px in range(40):
            dx = (px - 20) / 13.0
            dy = (py - 22) / 14.0
            if dx*dx + dy*dy <= 1.0:
                im_toriel.putpixel((px, py), (245, 245, 250, 255))
            if (px in range(11, 15) or px in range(25, 29)) and py in range(5, 12):
                im_toriel.putpixel((px, py), (230, 220, 180, 255))
            if (px in [15, 16] or px in [23, 24]) and py in [18, 19]:
                im_toriel.putpixel((px, py), (40, 20, 10, 255))
            if px in range(18, 22) and py == 24:
                im_toriel.putpixel((px, py), (180, 180, 190, 255))
            if (px in range(17, 23) and py == 27) or (px in [16, 23] and py == 26):
                im_toriel.putpixel((px, py), (80, 40, 40, 255))
            if dy > 0.8 and abs(dx) < 1.1:
                im_toriel.putpixel((px, py), (140, 40, 150, 255))
    emit_standalone_gfx("spr_face_toriel_warm", im_toriel)

    im_bb = Image.open('/tmp/battle-engine/sprites/spr_battleback/d6ad0b26-42d2-4dae-a2d6-f05bcc79cc38.png')
    emit_standalone_gfx("bg_battle_tile", im_bb)

    im_field = Image.open('/tmp/unity-engine/Assets/Sprites/background/field.png').resize((256, 112), Image.BILINEAR)
    emit_standalone_gfx("bg_field_banner", im_field)

    # 3. Audio conversion
    print("Converting Audio Samples & Music Loops...")
    emit_audio("snd_text_default", '/tmp/battle-engine/sounds/snd_text/snd_text')
    emit_audio("snd_text_susie", '/tmp/battle-engine/sounds/snd_text_voice_susie/snd_text_voice_susie')
    emit_audio("snd_text_ralsei", '/tmp/kristal/assets/sounds/voice/ralsei.wav')
    emit_audio("snd_text_toriel", '/tmp/kristal/assets/sounds/voice/toriel.wav')
    emit_audio("snd_menu_switch", '/tmp/battle-engine/sounds/snd_menu_switch/snd_menu_switch')
    emit_audio("snd_menu_confirm", '/tmp/battle-engine/sounds/snd_menu_confirm/snd_menu_confirm')
    emit_audio("snd_attack", '/tmp/battle-engine/sounds/snd_attack/snd_attack')
    emit_audio("snd_damage", '/tmp/battle-engine/sounds/snd_damage/snd_damage')
    emit_audio("snd_hurt", '/tmp/battle-engine/sounds/snd_hurt/snd_hurt')
    emit_audio("snd_graze", '/tmp/battle-engine/sounds/snd_graze/snd_graze')
    emit_audio("snd_spell", '/tmp/kristal/assets/sounds/laz_c.wav')
    emit_audio("snd_door", '/tmp/kristal/assets/sounds/dooropen.wav')
    emit_audio("snd_bell", '/tmp/kristal/assets/sounds/bell.wav')
    emit_audio("snd_jevil_chaos", '/tmp/jevil-sim/src/sounds/chaoschaos.wav')
    emit_audio("snd_jevil_anything", '/tmp/jevil-sim/src/sounds/icandoanything.wav')

    # Music loops (11025 Hz mono)
    # Dark world ambient loop (12 seconds)
    emit_audio("bgm_dark_world", '/tmp/kristal/assets/music/AUDIO_DARKNESS.ogg', max_dur=12.0)
    # Rude Buster battle theme loop (14 seconds)
    emit_audio("bgm_rude_buster", '/tmp/kristal/assets/music/battle.ogg', max_dur=14.0)
    # Jevil boss theme loop (12 seconds)
    emit_audio("bgm_jevil", '/tmp/jevil-sim/src/sounds/joker.flac', max_dur=12.0)

    gfx_c.close()
    gfx_h.close()
    audio_c.close()
    audio_h.close()
    print("Asset conversion complete!")

if __name__ == '__main__':
    main()
