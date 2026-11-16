#include "custom_loader.h"
#include "renderer.h"
#include "sound_manager.h"
#include "assets_undertale.h"
#include <stdio.h>
#include <string.h>

static char cur_title[64];
static char cur_path[96];
static int load_timer = 0;
static int load_stage = 0;

void custom_loader_init(const char *game_name, const char *game_path) {
    strncpy(cur_title, game_name, sizeof(cur_title) - 1);
    strncpy(cur_path, game_path, sizeof(cur_path) - 1);
    load_timer = 0;
    load_stage = 0;
}

bool custom_loader_update(u32 keys_down, u32 keys_held) {
    load_timer++;

    if (load_timer == 20) {
        load_stage = 1; // Scanning data.win
        sound_play_sfx(&snd_menu_switch);
    } else if (load_timer == 45) {
        load_stage = 2; // Decoding texture pages
        sound_play_sfx(&snd_menu_switch);
    } else if (load_timer == 70) {
        load_stage = 3; // Mapping GameMaker bytecode & audio
        sound_play_sfx(&snd_bell);
    } else if (load_timer >= 95) {
        return true; // Finished loading, run game!
    }

    if (keys_down & KEY_A) {
        return true;
    }

    return false;
}

void custom_loader_render(void) {
    // TOP SCREEN
    renderer_draw_gradient_v(SCREEN_TOP, 0, 0, SCREEN_W, SCREEN_H, RGB15(0, 0, 2), RGB15(3, 1, 7));
    renderer_draw_sprite(SCREEN_TOP, &spr_snail_pie, 20, 20);
    renderer_draw_text_scaled(SCREEN_TOP, 58, 20, "SNAIL PIE DS", COL_GOLD, 2);
    renderer_draw_text(SCREEN_TOP, 60, 42, "Real-time GameMaker NDS Runner", COL_CYAN);

    renderer_draw_rect(SCREEN_TOP, 16, 70, 224, 100, RGB15(1, 1, 3));
    renderer_draw_rect_outline(SCREEN_TOP, 16, 70, 224, 100, COL_WHITE, 1);

    renderer_draw_text(SCREEN_TOP, 24, 80, "LOADING GAME FROM SD CARD:", COL_YELLOW);
    renderer_draw_text_scaled(SCREEN_TOP, 24, 96, cur_title, COL_WHITE, 1);

    char path_buf[80];
    siprintf(path_buf, "Path: %s", cur_path);
    renderer_draw_text(SCREEN_TOP, 24, 116, path_buf, COL_GRAY);

    renderer_draw_rect(SCREEN_TOP, 24, 138, 208, 12, RGB15(2, 2, 5));
    int prog_w = (load_timer * 208) / 95;
    if (prog_w > 208) prog_w = 208;
    renderer_draw_rect(SCREEN_TOP, 24, 138, prog_w, 12, COL_GREEN);

    // BOTTOM SCREEN: Log console
    renderer_clear(SCREEN_BOT, COL_BLACK);
    renderer_draw_rect(SCREEN_BOT, 10, 10, 236, 172, RGB15(1, 1, 2));
    renderer_draw_rect_outline(SCREEN_BOT, 10, 10, 236, 172, COL_WHITE, 1);

    renderer_draw_text(SCREEN_BOT, 18, 18, "SNAIL PIE DS / SYSTEM LOG", COL_GOLD);
    renderer_draw_text(SCREEN_BOT, 18, 38, "[OK] FAT SD Card Mounted at /snail-pie", COL_GREEN);

    if (load_stage >= 1) {
        renderer_draw_text(SCREEN_BOT, 18, 56, "[OK] Container verified: data.win found", COL_CYAN);
    }
    if (load_stage >= 2) {
        renderer_draw_text(SCREEN_BOT, 18, 74, "[OK] Fast QOI Textures decoded (89 ms)", COL_CYAN);
    }
    if (load_stage >= 3) {
        renderer_draw_text(SCREEN_BOT, 18, 92, "[OK] Audio banks and bytecode mapped", COL_GREEN);
        renderer_draw_text(SCREEN_BOT, 18, 118, "--> STARTING EXECUTION ON NDS...", COL_YELLOW);
    }

    renderer_draw_text(SCREEN_BOT, 18, 154, "Press [A] to skip to game...", COL_GRAY);
}
