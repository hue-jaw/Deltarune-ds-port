#ifndef RENDERER_H
#define RENDERER_H

#include <nds.h>
#include "assets_gfx.h"

#define SCREEN_W 256
#define SCREEN_H 192

#define SCREEN_TOP 0
#define SCREEN_BOT 1

#define COL_BLACK       RGB15(0, 0, 0)
#define COL_WHITE       RGB15(31, 31, 31)
#define COL_RED         RGB15(31, 0, 0)
#define COL_GREEN       RGB15(0, 31, 0)
#define COL_BLUE        RGB15(0, 0, 31)
#define COL_YELLOW      RGB15(31, 31, 0)
#define COL_CYAN        RGB15(0, 31, 31)
#define COL_MAGENTA     RGB15(31, 0, 31)
#define COL_ORANGE      RGB15(31, 16, 0)
#define COL_PURPLE      RGB15(16, 0, 24)
#define COL_DARK_BLUE   RGB15(2, 2, 7)
#define COL_DARK_PURPLE RGB15(4, 0, 8)
#define COL_GRAY        RGB15(16, 16, 16)
#define COL_DARK_GRAY   RGB15(8, 8, 8)
#define COL_LIGHT_GRAY  RGB15(24, 24, 24)
#define COL_GOLD        RGB15(31, 25, 4)

void renderer_init(void);
void renderer_clear(int screen, u16 color);
void renderer_set_pixel(int screen, int x, int y, u16 color);
void renderer_draw_rect(int screen, int x, int y, int w, int h, u16 color);
void renderer_draw_rect_outline(int screen, int x, int y, int w, int h, u16 color, int thickness);
void renderer_draw_gradient_v(int screen, int x, int y, int w, int h, u16 col_top, u16 col_bot);

void renderer_draw_sprite(int screen, const GfxImage *img, int x, int y);
void renderer_draw_sprite_tint(int screen, const GfxImage *img, int x, int y, u16 tint_color);
void renderer_draw_sprite_sub(int screen, const GfxImage *img, int x, int y, int sx, int sy, int sw, int sh);

void renderer_draw_char(int screen, int x, int y, char c, u16 color);
void renderer_draw_text(int screen, int x, int y, const char *str, u16 color);
void renderer_draw_text_len(int screen, int x, int y, const char *str, int len, u16 color);
void renderer_draw_text_scaled(int screen, int x, int y, const char *str, u16 color, int scale);

void renderer_present(void);

#endif
