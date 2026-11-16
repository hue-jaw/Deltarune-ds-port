#!/usr/bin/env python3
import json
from PIL import Image

with open('/tmp/battle-engine/fonts/font_determination_mono/font_determination_mono.yy') as f:
    d = json.load(f)

im = Image.open('/tmp/battle-engine/fonts/font_determination_mono/font_determination_mono.png').convert('L')
glyphs = {g['Key']: g['Value'] for g in d['glyphs']}

font_bytes = []
for code in range(32, 127):
    if code in glyphs:
        g = glyphs[code]
        gx, gy, gw, gh = g['x'], g['y'], g['w'], g['h']
    else:
        gx, gy, gw, gh = 0, 0, 0, 0

    rows = []
    for row in range(16):
        byte_val = 0
        for col in range(8):
            if row < gh and col < gw:
                px = im.getpixel((gx + col, gy + row))
                if px > 128:
                    byte_val |= (1 << (7 - col))
        rows.append(byte_val)
    font_bytes.append(rows)

with open('source/font_data.h', 'w') as f:
    f.write("""#ifndef FONT_DATA_H
#define FONT_DATA_H

#include <nds.h>

#define FONT_FIRST_CHAR 32
#define FONT_LAST_CHAR  126
#define FONT_CHAR_COUNT (FONT_LAST_CHAR - FONT_FIRST_CHAR + 1)
#define FONT_WIDTH      8
#define FONT_HEIGHT     16

extern const u8 font_determination_8x16[FONT_CHAR_COUNT][FONT_HEIGHT];

#endif
""")

with open('source/font_data.c', 'w') as f:
    f.write("""#include "font_data.h"

const u8 font_determination_8x16[FONT_CHAR_COUNT][FONT_HEIGHT] = {
""")
    for i, rows in enumerate(font_bytes):
        c = chr(i + 32)
        if c == '\\': c_desc = "'\\\\'"
        elif c == '"': c_desc = "'\\\"'"
        elif c == ' ': c_desc = "space"
        else: c_desc = f"'{c}'"
        row_strs = [f"0x{r:02X}" for r in rows]
        f.write(f"    {{ {', '.join(row_strs)} }}, // {c_desc} ({i+32})\n")
    f.write("};\n")

print("font_data.c and font_data.h generated successfully!")
