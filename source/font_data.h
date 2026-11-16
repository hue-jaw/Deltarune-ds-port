#ifndef FONT_DATA_H
#define FONT_DATA_H

#include <nds.h>

#define FONT_FIRST_CHAR 32
#define FONT_LAST_CHAR  126
#define FONT_CHAR_COUNT (FONT_LAST_CHAR - FONT_FIRST_CHAR + 1)
#define FONT_WIDTH      8
#define FONT_HEIGHT     16

extern const u8 font_determination_8x16[FONT_CHAR_COUNT][FONT_HEIGHT];

#endif
