#!/usr/bin/env python3
"""
Generate Undertale & Snail Pie DS assets in ARGB1555 format.
"""
from PIL import Image, ImageDraw

def rgba_to_argb1555(r, g, b, a):
    if a < 128:
        return 0
    r5 = (r * 31) // 255
    g5 = (g * 31) // 255
    b5 = (b * 31) // 255
    return (1 << 15) | (r5) | (g5 << 5) | (b5 << 10)

def convert_to_c(im):
    w, h = im.size
    px = []
    for y in range(h):
        for x in range(w):
            r, g, b, a = im.getpixel((x, y))
            px.append(rgba_to_argb1555(r, g, b, a))
    return w, h, px

def create_snail_pie_sprite():
    im = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # Plate
    d.ellipse([2, 18, 30, 28], fill=(180, 180, 190, 255), outline=(100, 100, 120, 255))
    # Crust
    d.ellipse([4, 12, 28, 24], fill=(210, 140, 50, 255), outline=(140, 80, 20, 255))
    # Pie Filling / Top
    d.ellipse([7, 14, 25, 21], fill=(240, 180, 80, 255))
    # Snail shell spiral pattern
    spiral_col = (110, 60, 20, 255)
    d.arc([10, 15, 22, 20], start=0, end=360, fill=spiral_col, width=1)
    d.arc([12, 16, 20, 19], start=0, end=270, fill=spiral_col, width=1)
    # Steam
    d.line([(10, 10), (12, 6), (10, 2)], fill=(255, 255, 255, 200), width=1)
    d.line([(16, 10), (18, 5), (16, 1)], fill=(255, 255, 255, 220), width=1)
    d.line([(22, 10), (24, 6), (22, 2)], fill=(255, 255, 255, 200), width=1)
    return im

def create_sans_idle():
    im = Image.new("RGBA", (32, 36), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # Head (white skull)
    d.ellipse([10, 2, 22, 13], fill=(255, 255, 255, 255), outline=(0, 0, 0, 255))
    # Eyes
    d.rectangle([12, 6, 14, 8], fill=(0, 0, 0, 255))
    d.rectangle([18, 6, 20, 8], fill=(0, 0, 0, 255))
    # Nose
    d.rectangle([15, 8, 16, 9], fill=(0, 0, 0, 255))
    # Grin
    d.line([(12, 11), (20, 11)], fill=(0, 0, 0, 255), width=1)
    d.line([(14, 10), (14, 12)], fill=(0, 0, 0, 255), width=1)
    d.line([(16, 10), (16, 12)], fill=(0, 0, 0, 255), width=1)
    d.line([(18, 10), (18, 12)], fill=(0, 0, 0, 255), width=1)
    # Blue Hoodie
    d.rectangle([7, 14, 25, 26], fill=(45, 120, 220, 255), outline=(0, 0, 0, 255))
    # White T-Shirt
    d.rectangle([13, 14, 19, 24], fill=(240, 240, 240, 255))
    d.rectangle([14, 15, 18, 23], fill=(210, 210, 210, 255))
    # Pockets
    d.rectangle([8, 20, 12, 25], fill=(30, 90, 180, 255))
    d.rectangle([20, 20, 24, 25], fill=(30, 90, 180, 255))
    # Hands in pockets
    d.rectangle([9, 21, 11, 24], fill=(255, 255, 255, 255))
    d.rectangle([21, 21, 23, 24], fill=(255, 255, 255, 255))
    # Shorts (black with white stripe)
    d.rectangle([9, 27, 23, 31], fill=(15, 15, 15, 255))
    d.line([(10, 27), (10, 31)], fill=(255, 255, 255, 255), width=1)
    d.line([(22, 27), (22, 31)], fill=(255, 255, 255, 255), width=1)
    # Legs (white bones)
    d.rectangle([12, 31, 14, 33], fill=(255, 255, 255, 255))
    d.rectangle([18, 31, 20, 33], fill=(255, 255, 255, 255))
    # Slippers (pink/light purple)
    d.rectangle([10, 33, 15, 35], fill=(245, 170, 200, 255), outline=(0, 0, 0, 255))
    d.rectangle([17, 33, 22, 35], fill=(245, 170, 200, 255), outline=(0, 0, 0, 255))
    return im

def create_sans_eye():
    im = create_sans_idle()
    d = ImageDraw.Draw(im)
    # Left eye glowing cyan and yellow
    d.rectangle([12, 5, 14, 8], fill=(0, 255, 255, 255))
    d.rectangle([13, 6, 14, 7], fill=(255, 255, 0, 255))
    # Flame aura
    d.line([(13, 4), (12, 2)], fill=(0, 255, 255, 200), width=1)
    d.line([(14, 4), (15, 1)], fill=(0, 200, 255, 200), width=1)
    return im

def create_sans_dodge():
    im = Image.new("RGBA", (32, 36), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # Tilted dodging posture shifted to the right
    d.ellipse([14, 4, 26, 14], fill=(255, 255, 255, 255), outline=(0, 0, 0, 255))
    d.rectangle([17, 8, 18, 9], fill=(0, 0, 0, 255))
    d.rectangle([22, 8, 23, 9], fill=(0, 0, 0, 255))
    d.line([(17, 12), (24, 12)], fill=(0, 0, 0, 255), width=1)
    # Body shifted
    d.rectangle([10, 15, 28, 27], fill=(45, 120, 220, 255), outline=(0, 0, 0, 255))
    d.rectangle([16, 15, 22, 24], fill=(240, 240, 240, 255))
    # Shorts
    d.rectangle([12, 28, 26, 31], fill=(15, 15, 15, 255))
    # Slippers
    d.rectangle([12, 33, 17, 35], fill=(245, 170, 200, 255))
    d.rectangle([21, 33, 26, 35], fill=(245, 170, 200, 255))
    return im

def create_gaster_blaster():
    im = Image.new("RGBA", (28, 28), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # Blaster Skull
    d.polygon([(14, 2), (24, 8), (22, 22), (14, 26), (6, 22), (4, 8)], fill=(255, 255, 255, 255), outline=(0, 0, 0, 255))
    # Eye sockets
    d.polygon([(8, 10), (12, 12), (10, 16)], fill=(0, 0, 0, 255))
    d.polygon([(20, 10), (16, 12), (18, 16)], fill=(0, 0, 0, 255))
    # Glowing center
    d.rectangle([(10, 13), (11, 14)], fill=(0, 255, 255, 255))
    d.rectangle([(17, 13), (18, 14)], fill=(0, 255, 255, 255))
    # Teeth / snout
    d.rectangle([(10, 20), (18, 24)], fill=(0, 0, 0, 255))
    d.line([(12, 20), (12, 24)], fill=(255, 255, 255, 255), width=1)
    d.line([(14, 20), (14, 24)], fill=(255, 255, 255, 255), width=1)
    d.line([(16, 20), (16, 24)], fill=(255, 255, 255, 255), width=1)
    return im

def create_gaster_blaster_fire():
    im = create_gaster_blaster()
    d = ImageDraw.Draw(im)
    # Open jaw and charging blast
    d.ellipse([(8, 18), (20, 27)], fill=(0, 0, 0, 255))
    d.ellipse([(11, 20), (17, 26)], fill=(0, 255, 255, 255))
    d.ellipse([(12, 21), (16, 25)], fill=(255, 255, 255, 255))
    return im

def create_froggit():
    im = Image.new("RGBA", (28, 24), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # Frog body
    d.ellipse([4, 6, 24, 20], fill=(255, 255, 255, 255), outline=(0, 0, 0, 255))
    # Big round black eyes
    d.ellipse([6, 3, 12, 10], fill=(255, 255, 255, 255), outline=(0, 0, 0, 255))
    d.ellipse([16, 3, 22, 10], fill=(255, 255, 255, 255), outline=(0, 0, 0, 255))
    d.rectangle([8, 5, 10, 8], fill=(0, 0, 0, 255))
    d.rectangle([18, 5, 20, 8], fill=(0, 0, 0, 255))
    # Mouth
    d.line([(9, 14), (19, 14)], fill=(0, 0, 0, 255), width=1)
    # Frog legs
    d.ellipse([2, 14, 7, 22], fill=(255, 255, 255, 255), outline=(0, 0, 0, 255))
    d.ellipse([21, 14, 26, 22], fill=(255, 255, 255, 255), outline=(0, 0, 0, 255))
    return im

def create_blue_soul():
    im = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # Blue heart
    d.polygon([(8, 14), (2, 8), (2, 4), (5, 2), (8, 5), (11, 2), (14, 4), (14, 8)], fill=(0, 180, 255, 255))
    d.polygon([(8, 12), (4, 8), (4, 5), (6, 3), (8, 6), (10, 3), (12, 5), (12, 8)], fill=(80, 220, 255, 255))
    return im

def create_yellow_soul():
    im = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # Yellow heart upside down (Justice)
    d.polygon([(8, 2), (2, 8), (2, 12), (5, 14), (8, 11), (11, 14), (14, 12), (14, 8)], fill=(255, 220, 0, 255))
    d.polygon([(8, 4), (4, 8), (4, 11), (6, 13), (8, 10), (10, 13), (12, 11), (12, 8)], fill=(255, 255, 100, 255))
    return im

def create_bone_v():
    im = Image.new("RGBA", (8, 28), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rectangle([2, 3, 5, 24], fill=(255, 255, 255, 255))
    d.ellipse([0, 0, 4, 4], fill=(255, 255, 255, 255))
    d.ellipse([3, 0, 7, 4], fill=(255, 255, 255, 255))
    d.ellipse([0, 23, 4, 27], fill=(255, 255, 255, 255))
    d.ellipse([3, 23, 7, 27], fill=(255, 255, 255, 255))
    return im

def create_bone_h():
    im = Image.new("RGBA", (28, 8), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rectangle([3, 2, 24, 5], fill=(255, 255, 255, 255))
    d.ellipse([0, 0, 4, 4], fill=(255, 255, 255, 255))
    d.ellipse([0, 3, 4, 7], fill=(255, 255, 255, 255))
    d.ellipse([23, 0, 27, 4], fill=(255, 255, 255, 255))
    d.ellipse([23, 3, 27, 7], fill=(255, 255, 255, 255))
    return im

def create_ut_button(label, active):
    im = Image.new("RGBA", (44, 20), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    bg = (240, 140, 20, 255) if active else (180, 80, 10, 255)
    border = (255, 220, 40, 255) if active else (220, 120, 20, 255)
    d.rectangle([0, 0, 43, 19], fill=bg, outline=border, width=2)
    # Label is drawn as text
    return im

def main():
    sprites = {
        "spr_snail_pie": create_snail_pie_sprite(),
        "spr_sans_idle": create_sans_idle(),
        "spr_sans_eye": create_sans_eye(),
        "spr_sans_dodge": create_sans_dodge(),
        "spr_gaster_blaster": create_gaster_blaster(),
        "spr_gaster_blaster_fire": create_gaster_blaster_fire(),
        "spr_froggit": create_froggit(),
        "spr_soul_blue": create_blue_soul(),
        "spr_soul_yellow": create_yellow_soul(),
        "spr_bone_v": create_bone_v(),
        "spr_bone_h": create_bone_h(),
    }

    with open("source/assets_undertale.h", "w") as f_h, open("source/assets_undertale.c", "w") as f_c:
        f_h.write("""#ifndef ASSETS_UNDERTALE_H
#define ASSETS_UNDERTALE_H

#include "assets_gfx.h"

""")
        f_c.write("""#include "assets_undertale.h"\n\n""")

        for name, im in sprites.items():
            w, h, px = convert_to_c(im)
            dname = f"data_{name}"
            f_c.write(f"static const u16 {dname}[{len(px)}] = {{\n")
            for i in range(0, len(px), 12):
                chunk = px[i:i+12]
                f_c.write("    " + ", ".join(f"0x{p:04X}" for p in chunk) + ",\n")
            f_c.write("};\n\n")
            f_c.write(f"const GfxImage {name} = {{ {w}, {h}, {dname} }};\n\n")
            f_h.write(f"extern const GfxImage {name};\n")

        f_h.write("\n#endif\n")

    print("[Undertale Assets] Generated source/assets_undertale.h & source/assets_undertale.c successfully!")

if __name__ == "__main__":
    main()
