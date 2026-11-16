#include "title_screen.h"
#include "renderer.h"
#include "sound_manager.h"

#define TOTAL_OPTIONS 7

static int selected_option = 0;
static int anim_timer = 0;

static const char *options[TOTAL_OPTIONS] = {
    "1. Start Story Campaign",
    "2. Explore: Field of Dreams",
    "3. Boss: Lancer (Roadblock)",
    "4. Boss: K. Round (Checker King)",
    "5. Final Boss: Chaos King",
    "6. Secret Boss: Jevil (Chaos)",
    "7. The Legend of Delta Rune"
};

void title_screen_init(void) {
    selected_option = 0;
    anim_timer = 0;
    sound_play_bgm(&bgm_dark_world);
}

TitleChoice title_screen_update(u32 keys_down, u32 keys_held, touchPosition touch) {
    anim_timer++;

    if (keys_down & KEY_UP) {
        selected_option = (selected_option + TOTAL_OPTIONS - 1) % TOTAL_OPTIONS;
        sound_play_sfx(&snd_menu_switch);
    }
    if (keys_down & KEY_DOWN) {
        selected_option = (selected_option + 1) % TOTAL_OPTIONS;
        sound_play_sfx(&snd_menu_switch);
    }

    if (keys_down & KEY_TOUCH) {
        int row = (touch.py - 42) / 18;
        if (row >= 0 && row < TOTAL_OPTIONS) {
            selected_option = row;
            sound_play_sfx(&snd_menu_confirm);
            return (TitleChoice)selected_option;
        }
    }

    if (keys_down & (KEY_A | KEY_START)) {
        sound_play_sfx(&snd_menu_confirm);
        return (TitleChoice)selected_option;
    }

    return TITLE_CHOICE_NONE;
}

void title_screen_render(void) {
    // ==========================================
    // TOP SCREEN: Title Logo & Crest
    // ==========================================
    renderer_draw_gradient_v(SCREEN_TOP, 0, 0, SCREEN_W, SCREEN_H, RGB15(0, 0, 2), RGB15(4, 1, 8));

    for (int i = 0; i < 24; i++) {
        int sx = (i * 39 + 17) % SCREEN_W;
        int sy = (i * 23 + 11) % (SCREEN_H - 40);
        int twinkle = ((anim_timer / 10) + i) % 4;
        u16 star_col = (twinkle == 0) ? COL_WHITE : (twinkle == 1 ? COL_LIGHT_GRAY : COL_DARK_GRAY);
        renderer_set_pixel(SCREEN_TOP, sx, sy, star_col);
    }

    // Delta Rune Crest
    int cx = 128;
    int cy = 40;
    int pulse = (anim_timer / 12) % 3;
    u16 crest_col = (pulse == 0) ? COL_WHITE : RGB15(26, 26, 31);

    renderer_draw_rect(SCREEN_TOP, cx - 8, cy - 8, 16, 16, crest_col);
    renderer_draw_rect(SCREEN_TOP, cx - 24, cy - 4, 14, 8, crest_col);
    renderer_draw_rect(SCREEN_TOP, cx + 10, cy - 4, 14, 8, crest_col);

    renderer_draw_rect(SCREEN_TOP, cx - 22, cy + 20, 12, 12, crest_col);
    renderer_draw_rect(SCREEN_TOP, cx + 10, cy + 20, 12, 12, crest_col);
    renderer_draw_rect(SCREEN_TOP, cx - 6, cy + 28, 12, 12, crest_col);

    // DELTARUNE Title
    renderer_draw_text_scaled(SCREEN_TOP, 54, 85, "DELTARUNE", COL_WHITE, 2);
    renderer_draw_text(SCREEN_TOP, 50, 126, "CHAPTER 1 - THE BEGINNING", COL_CYAN);
    renderer_draw_text(SCREEN_TOP, 34, 150, "Snail Pie DS / GameMaker Engine", COL_YELLOW);
    renderer_draw_text(SCREEN_TOP, 42, 168, "Toby Fox / Nintendo DS Port", COL_GRAY);

    // ==========================================
    // BOTTOM SCREEN: Chapter Select Menu
    // ==========================================
    renderer_clear(SCREEN_BOT, COL_BLACK);

    // Header
    renderer_draw_rect(SCREEN_BOT, 12, 8, 232, 26, RGB15(2, 2, 6));
    renderer_draw_rect_outline(SCREEN_BOT, 12, 8, 232, 26, COL_WHITE, 1);
    renderer_draw_text(SCREEN_BOT, 36, 14, "SELECT CHAPTER / BOSS BATTLE", COL_GOLD);

    for (int i = 0; i < TOTAL_OPTIONS; i++) {
        int by = 42 + i * 18;
        bool is_sel = (i == selected_option);

        renderer_draw_rect(SCREEN_BOT, 14, by, 228, 16, is_sel ? RGB15(4, 2, 8) : COL_BLACK);
        if (is_sel) {
            renderer_draw_rect_outline(SCREEN_BOT, 14, by, 228, 16, COL_YELLOW, 1);
            renderer_draw_char(SCREEN_BOT, 18, by, '*', COL_YELLOW);
        }
        renderer_draw_text(SCREEN_BOT, 30, by, options[i], is_sel ? COL_YELLOW : COL_WHITE);
    }

    renderer_draw_rect(SCREEN_BOT, 0, 172, SCREEN_W, 20, RGB15(2, 2, 4));
    renderer_draw_text(SCREEN_BOT, 14, 175, "[D-PAD / TOUCH] Select   [A / START] Enter", COL_GRAY);
}
