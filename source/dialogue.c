#include "dialogue.h"
#include "renderer.h"
#include <string.h>

static bool is_active = false;
static const DialogueEntry *entries_list = NULL;
static int total_entries = 0;
static int current_entry_idx = 0;
static int char_progress = 0;
static int char_timer = 0;
static int target_screen = SCREEN_BOT;
static DialogueCallback finish_cb = NULL;
static int cursor_blink = 0;

void dialogue_init(void) {
    is_active = false;
    entries_list = NULL;
    total_entries = 0;
    current_entry_idx = 0;
    char_progress = 0;
    char_timer = 0;
    target_screen = SCREEN_BOT;
    finish_cb = NULL;
}

void dialogue_start(const DialogueEntry *entries, int count, int screen, DialogueCallback on_finish) {
    if (!entries || count <= 0) return;
    entries_list = entries;
    total_entries = count;
    current_entry_idx = 0;
    char_progress = 0;
    char_timer = 0;
    target_screen = screen;
    finish_cb = on_finish;
    is_active = true;
    cursor_blink = 0;
}

bool dialogue_is_active(void) {
    return is_active;
}

void dialogue_update(u32 keys_down, u32 keys_held) {
    if (!is_active || !entries_list) return;

    cursor_blink++;
    const DialogueEntry *cur = &entries_list[current_entry_idx];
    int full_len = (int)strlen(cur->text);

    int speed = 2; // frames per char
    if (keys_held & (KEY_B | KEY_X)) {
        speed = 1; // faster typing
    }

    if (char_progress < full_len) {
        char_timer++;
        int chars_to_add = (keys_held & (KEY_B | KEY_X)) ? 2 : 1;
        if (char_timer >= speed) {
            char_timer = 0;
            for (int i = 0; i < chars_to_add && char_progress < full_len; i++) {
                char c = cur->text[char_progress];
                char_progress++;
                if (c != ' ' && c != '\n' && (char_progress % 2 == 0)) {
                    sound_play_voice(cur->voice);
                }
            }
        }
    }

    if (keys_down & KEY_A) {
        if (char_progress < full_len) {
            // Fast forward to end of current text
            char_progress = full_len;
        } else {
            // Next dialogue entry
            current_entry_idx++;
            char_progress = 0;
            char_timer = 0;
            if (current_entry_idx >= total_entries) {
                is_active = false;
                if (finish_cb) {
                    finish_cb();
                }
            }
        }
    }
}

void dialogue_render(void) {
    if (!is_active || !entries_list) return;

    const DialogueEntry *cur = &entries_list[current_entry_idx];
    int bx = 8;
    int by = 118;
    int bw = 240;
    int bh = 66;

    // Deltarune style dialogue box: black fill, crisp 2px white outline
    renderer_draw_rect(target_screen, bx, by, bw, bh, COL_BLACK);
    renderer_draw_rect_outline(target_screen, bx, by, bw, bh, COL_WHITE, 2);

    int text_x = bx + 16;
    int text_y = by + 12;

    if (cur->portrait) {
        // Draw portrait box & portrait
        int px = bx + 10;
        int py = by + 11;
        renderer_draw_sprite(target_screen, cur->portrait, px, py);
        text_x = bx + 58;
    }

    // Draw typewriter text
    renderer_draw_text_len(target_screen, text_x, text_y, cur->text, char_progress, COL_WHITE);

    // If typing finished for current page, show blinking advance arrow
    int full_len = (int)strlen(cur->text);
    if (char_progress >= full_len) {
        if ((cursor_blink / 15) % 2 == 0) {
            renderer_draw_char(target_screen, bx + bw - 18, by + bh - 18, '*', COL_YELLOW);
        }
    }
}
