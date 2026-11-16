#include "renderer.h"
#include "font_data.h"
#include <string.h>

static u16 top_buf[SCREEN_W * SCREEN_H];
static u16 bot_buf[SCREEN_W * SCREEN_H];

static u16 *vram_main_ptr = NULL;
static u16 *vram_sub_ptr = NULL;

void renderer_init(void) {
    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_5_2D);

    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    int bg_main = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    int bg_sub = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);

    vram_main_ptr = (u16 *)bgGetGfxPtr(bg_main);
    vram_sub_ptr = (u16 *)bgGetGfxPtr(bg_sub);

    renderer_clear(SCREEN_TOP, COL_BLACK);
    renderer_clear(SCREEN_BOT, COL_BLACK);
    renderer_present();
}

static inline u16 *get_buffer(int screen) {
    return (screen == SCREEN_TOP) ? top_buf : bot_buf;
}

void renderer_clear(int screen, u16 color) {
    u16 *buf = get_buffer(screen);
    u16 val = color | 0x8000;
    dmaFillHalfWords(val, buf, SCREEN_W * SCREEN_H * 2);
}

void renderer_set_pixel(int screen, int x, int y, u16 color) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    u16 *buf = get_buffer(screen);
    buf[y * SCREEN_W + x] = color | 0x8000;
}

void renderer_draw_rect(int screen, int x, int y, int w, int h, u16 color) {
    if (x >= SCREEN_W || y >= SCREEN_H) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0) return;

    u16 *buf = get_buffer(screen);
    u16 val = color | 0x8000;
    for (int r = 0; r < h; r++) {
        u16 *dest = &buf[(y + r) * SCREEN_W + x];
        for (int c = 0; c < w; c++) {
            dest[c] = val;
        }
    }
}

void renderer_draw_rect_outline(int screen, int x, int y, int w, int h, u16 color, int thickness) {
    if (thickness <= 0) return;
    // Top
    renderer_draw_rect(screen, x, y, w, thickness, color);
    // Bottom
    renderer_draw_rect(screen, x, y + h - thickness, w, thickness, color);
    // Left
    renderer_draw_rect(screen, x, y, thickness, h, color);
    // Right
    renderer_draw_rect(screen, x + w - thickness, y, thickness, h, color);
}

void renderer_draw_gradient_v(int screen, int x, int y, int w, int h, u16 col_top, u16 col_bot) {
    if (x >= SCREEN_W || y >= SCREEN_H) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0) return;

    int r0 = col_top & 0x1F;
    int g0 = (col_top >> 5) & 0x1F;
    int b0 = (col_top >> 10) & 0x1F;

    int r1 = col_bot & 0x1F;
    int g1 = (col_bot >> 5) & 0x1F;
    int b1 = (col_bot >> 10) & 0x1F;

    u16 *buf = get_buffer(screen);
    for (int r = 0; r < h; r++) {
        int cr = r0 + ((r1 - r0) * r) / h;
        int cg = g0 + ((g1 - g0) * r) / h;
        int cb = b0 + ((b1 - b0) * r) / h;
        u16 col = 0x8000 | (cr & 0x1F) | ((cg & 0x1F) << 5) | ((cb & 0x1F) << 10);
        u16 *dest = &buf[(y + r) * SCREEN_W + x];
        for (int c = 0; c < w; c++) {
            dest[c] = col;
        }
    }
}

void renderer_draw_sprite(int screen, const GfxImage *img, int x, int y) {
    if (!img || !img->data) return;
    int w = img->w;
    int h = img->h;

    int x_start = (x < 0) ? -x : 0;
    int y_start = (y < 0) ? -y : 0;
    int x_end = (x + w > SCREEN_W) ? SCREEN_W - x : w;
    int y_end = (y + h > SCREEN_H) ? SCREEN_H - y : h;

    if (x_start >= x_end || y_start >= y_end) return;

    u16 *buf = get_buffer(screen);
    for (int py = y_start; py < y_end; py++) {
        const u16 *src = &img->data[py * w + x_start];
        u16 *dst = &buf[(y + py) * SCREEN_W + (x + x_start)];
        int span = x_end - x_start;
        for (int px = 0; px < span; px++) {
            u16 p = src[px];
            if (p != 0) {
                dst[px] = p | 0x8000;
            }
        }
    }
}

void renderer_draw_sprite_tint(int screen, const GfxImage *img, int x, int y, u16 tint_color) {
    if (!img || !img->data) return;
    int w = img->w;
    int h = img->h;

    int x_start = (x < 0) ? -x : 0;
    int y_start = (y < 0) ? -y : 0;
    int x_end = (x + w > SCREEN_W) ? SCREEN_W - x : w;
    int y_end = (y + h > SCREEN_H) ? SCREEN_H - y : h;

    if (x_start >= x_end || y_start >= y_end) return;

    u16 *buf = get_buffer(screen);
    u16 tint_val = tint_color | 0x8000;
    for (int py = y_start; py < y_end; py++) {
        const u16 *src = &img->data[py * w + x_start];
        u16 *dst = &buf[(y + py) * SCREEN_W + (x + x_start)];
        int span = x_end - x_start;
        for (int px = 0; px < span; px++) {
            if (src[px] != 0) {
                dst[px] = tint_val;
            }
        }
    }
}

void renderer_draw_sprite_sub(int screen, const GfxImage *img, int x, int y, int sx, int sy, int sw, int sh) {
    if (!img || !img->data) return;
    if (sx < 0 || sy < 0 || sx + sw > img->w || sy + sh > img->h) return;

    int x_start = (x < 0) ? -x : 0;
    int y_start = (y < 0) ? -y : 0;
    int x_end = (x + sw > SCREEN_W) ? SCREEN_W - x : sw;
    int y_end = (y + sh > SCREEN_H) ? SCREEN_H - y : sh;

    if (x_start >= x_end || y_start >= y_end) return;

    u16 *buf = get_buffer(screen);
    for (int py = y_start; py < y_end; py++) {
        const u16 *src = &img->data[(sy + py) * img->w + (sx + x_start)];
        u16 *dst = &buf[(y + py) * SCREEN_W + (x + x_start)];
        int span = x_end - x_start;
        for (int px = 0; px < span; px++) {
            u16 p = src[px];
            if (p != 0) {
                dst[px] = p | 0x8000;
            }
        }
    }
}

void renderer_draw_char(int screen, int x, int y, char c, u16 color) {
    if (x >= SCREEN_W || y >= SCREEN_H || x + 8 <= 0 || y + 16 <= 0) return;
    if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR) c = ' ';
    int idx = (int)c - FONT_FIRST_CHAR;

    u16 *buf = get_buffer(screen);
    u16 val = color | 0x8000;

    for (int row = 0; row < FONT_HEIGHT; row++) {
        int py = y + row;
        if (py < 0 || py >= SCREEN_H) continue;
        u8 bits = font_determination_8x16[idx][row];
        if (!bits) continue;
        for (int col = 0; col < FONT_WIDTH; col++) {
            int px = x + col;
            if (px < 0 || px >= SCREEN_W) continue;
            if (bits & (1 << (7 - col))) {
                buf[py * SCREEN_W + px] = val;
            }
        }
    }
}

void renderer_draw_text(int screen, int x, int y, const char *str, u16 color) {
    if (!str) return;
    int cx = x;
    int cy = y;
    while (*str) {
        if (*str == '\n') {
            cx = x;
            cy += FONT_HEIGHT + 2;
        } else {
            renderer_draw_char(screen, cx, cy, *str, color);
            cx += FONT_WIDTH;
        }
        str++;
    }
}

void renderer_draw_text_len(int screen, int x, int y, const char *str, int len, u16 color) {
    if (!str || len <= 0) return;
    int cx = x;
    int cy = y;
    int count = 0;
    while (*str && count < len) {
        if (*str == '\n') {
            cx = x;
            cy += FONT_HEIGHT + 2;
        } else {
            renderer_draw_char(screen, cx, cy, *str, color);
            cx += FONT_WIDTH;
        }
        str++;
        count++;
    }
}

void renderer_draw_text_scaled(int screen, int x, int y, const char *str, u16 color, int scale) {
    if (!str || scale <= 1) {
        renderer_draw_text(screen, x, y, str, color);
        return;
    }
    int cx = x;
    int cy = y;
    u16 *buf = get_buffer(screen);
    u16 val = color | 0x8000;

    while (*str) {
        char c = *str;
        if (c == '\n') {
            cx = x;
            cy += (FONT_HEIGHT + 2) * scale;
        } else {
            if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR) c = ' ';
            int idx = (int)c - FONT_FIRST_CHAR;
            for (int row = 0; row < FONT_HEIGHT; row++) {
                u8 bits = font_determination_8x16[idx][row];
                if (!bits) continue;
                for (int col = 0; col < FONT_WIDTH; col++) {
                    if (bits & (1 << (7 - col))) {
                        // Fill scale x scale block
                        for (int dy = 0; dy < scale; dy++) {
                            int py = cy + row * scale + dy;
                            if (py < 0 || py >= SCREEN_H) continue;
                            for (int dx = 0; dx < scale; dx++) {
                                int px = cx + col * scale + dx;
                                if (px < 0 || px >= SCREEN_W) continue;
                                buf[py * SCREEN_W + px] = val;
                            }
                        }
                    }
                }
            }
            cx += FONT_WIDTH * scale;
        }
        str++;
    }
}

void renderer_present(void) {
    // Safe hardware VCount wait (prevents ARM7 SWI crash on ARM9)
    while (*(volatile u16 *)0x04000006 >= 192);
    while (*(volatile u16 *)0x04000006 < 192);

    if (vram_main_ptr) {
        dmaCopyWords(3, top_buf, vram_main_ptr, SCREEN_W * SCREEN_H * 2);
    }
    if (vram_sub_ptr) {
        dmaCopyWords(2, bot_buf, vram_sub_ptr, SCREEN_W * SCREEN_H * 2);
    }
}
