#include "title_screen.h"
#include "renderer.h"
#include "sound_manager.h"

static int selected_option = 0;
static int anim_timer = 0;

void title_screen_init(void) {
    selected_option = 0;
    anim_timer = 0;
    sound_play_bgm(&bgm_dark_world);
}

TitleChoice title_screen_update(u32 keys_down, u32 keys_held, touchPosition touch) {
    anim_timer++;

    if (keys_down & KEY_UP) {
        selected_option = (selected_option + 4) % 5;
        sound_play_sfx(&snd_menu_switch);
    }
    if (keys_down & KEY_DOWN) {
        selected_option = (selected_option + 1) % 5;
        sound_play_sfx(&snd_menu_switch);
    }

    if (keys_down & KEY_TOUCH) {
        // Touch screen row selection (y: 60..150)
        int row = (touch.py - 55) / 20;
        if (row >= 0 && row < 5) {
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

    // Ambient twinkling stars
    for (int i = 0; i < 24; i++) {
        int sx = (i * 39 + 17) % SCREEN_W;
        int sy = (i * 23 + 11) % (SCREEN_H - 40);
        int twinkle = ((anim_timer / 10) + i) % 4;
        u16 star_col = (twinkle == 0) ? COL_WHITE : (twinkle == 1 ? COL_LIGHT_GRAY : COL_DARK_GRAY);
        renderer_set_pixel(SCREEN_TOP, sx, sy, star_col);
    }

    // Glowing Delta Rune Crest
    int cx = 128;
    int cy = 45;
    int pulse = (anim_timer / 12) % 3;
    u16 crest_col = (pulse == 0) ? COL_WHITE : RGB15(26, 26, 31);

    // Orb and wings
    renderer_draw_rect(SCREEN_TOP, cx - 8, cy - 8, 16, 16, crest_col);
    renderer_draw_rect(SCREEN_TOP, cx - 24, cy - 4, 14, 8, crest_col);
    renderer_draw_rect(SCREEN_TOP, cx + 10, cy - 4, 14, 8, crest_col);

    // Three triangles
    renderer_draw_rect(SCREEN_TOP, cx - 22, cy + 20, 12, 12, crest_col);
    renderer_draw_rect(SCREEN_TOP, cx + 10, cy + 20, 12, 12, crest_col);
    renderer_draw_rect(SCREEN_TOP, cx - 6, cy + 28, 12, 12, crest_col);

    // DELTARUNE Title
    renderer_draw_text_scaled(SCREEN_TOP, 54, 95, "DELTARUNE", COL_WHITE, 2);
    renderer_draw_text(SCREEN_TOP, 50, 138, "CHAPTER 1 - THE BEGINNING", COL_CYAN);
    renderer_draw_text(SCREEN_TOP, 42, 165, "Toby Fox / Nintendo DS Port", COL_GRAY);

    // ==========================================
    // BOTTOM SCREEN: Chapter Select Menu
    // ==========================================
    renderer_clear(SCREEN_BOT, COL_BLACK);

    // Header
    renderer_draw_rect(SCREEN_BOT, 12, 12, 232, 28, RGB15(2, 2, 6));
    renderer_draw_rect_outline(SCREEN_BOT, 12, 12, 232, 28, COL_WHITE, 1);
    renderer_draw_text(SCREEN_BOT, 44, 20, "SELECT CHAPTER OR MODE", COL_GOLD);

    // Menu choices
    const char *options[5] = {
        "1. Start Story Campaign",
        "2. Explore: Field of Dreams",
        "3. Boss Battle: Lancer",
        "4. Secret Boss: Jevil",
        "5. The Legend (Cutscene)"
    };

    for (int i = 0; i < 5; i++) {
        int by = 55 + i * 22;
        bool is_sel = (i == selected_option);

        renderer_draw_rect(SCREEN_BOT, 16, by, 224, 18, is_sel ? RGB15(4, 2, 8) : COL_BLACK);
        if (is_sel) {
            renderer_draw_rect_outline(SCREEN_BOT, 16, by, 224, 18, COL_YELLOW, 1);
            renderer_draw_char(SCREEN_BOT, 22, by + 1, '*', COL_YELLOW);
        }
        renderer_draw_text(SCREEN_BOT, 36, by + 1, options[i], is_sel ? COL_YELLOW : COL_WHITE);
    }

    // Footer prompt
    renderer_draw_rect(SCREEN_BOT, 0, 170, SCREEN_W, 22, RGB15(2, 2, 4));
    renderer_draw_text(SCREEN_BOT, 14, 174, "[D-PAD / TOUCH] Select   [A / START] Enter", COL_GRAY);
}
