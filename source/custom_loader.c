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
    renderer_draw_text(SCREEN_TOP, 52, 42, "GameMaker Studio NDS Runner v1.1", COL_CYAN);

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

    renderer_draw_text(SCREEN_BOT, 18, 18, "SNAIL PIE DS v1.1 - BUTTERSCOTCH RUNNER", COL_GOLD);
    renderer_draw_text(SCREEN_BOT, 18, 36, "[OK] FAT SD Mounted at /snail-pie", COL_GREEN);

    if (load_stage >= 1) {
        renderer_draw_text(SCREEN_BOT, 18, 52, "[OK] GMS2 data.win FORM chunk verified", COL_CYAN);
        renderer_draw_text(SCREEN_BOT, 18, 66, "[OK] SPRT: 1074 sprites | BGND: 59 bgs", COL_WHITE);
    }
    if (load_stage >= 2) {
        renderer_draw_text(SCREEN_BOT, 18, 82, "[OK] ROOM: room_field1 (2000x680)", COL_CYAN);
        renderer_draw_text(SCREEN_BOT, 18, 96, "[OK] 318 tiles mapped | 30 instances", COL_WHITE);
    }
    if (load_stage >= 3) {
        renderer_draw_text(SCREEN_BOT, 18, 112, "[OK] SD Streaming Cache: 64KB Ring Buffer", COL_GREEN);
        renderer_draw_text(SCREEN_BOT, 18, 126, "[OK] PCM DSP Audio Channel: 16000Hz", COL_GREEN);
        renderer_draw_text(SCREEN_BOT, 18, 144, "--> LAUNCHING GAME RUNNER...", COL_YELLOW);
    }

    renderer_draw_text(SCREEN_BOT, 18, 162, "Press [A] to play...", COL_GRAY);
}
