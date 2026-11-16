#include "launcher.h"
#include "renderer.h"
#include "sound_manager.h"
#include "assets_gfx.h"
#include "assets_undertale.h"
#include <fat.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_GAMES 32

typedef struct {
    char title[48];
    char author[32];
    char path[64];
    char badge[24];
    char desc[96];
    LauncherAction action;
    bool is_sd;
} GameEntry;

static GameEntry games[MAX_GAMES];
static int game_count = 0;
static int selected_idx = 0;
static int scroll_offset = 0;
static int anim_timer = 0;
static bool fat_initialized = false;
static bool show_sd_guide = false;

static char sd_root_path[64] = "/snail-pie";

static void add_game(const char *title, const char *author, const char *path, const char *badge, const char *desc, LauncherAction action, bool is_sd) {
    if (game_count >= MAX_GAMES) return;
    strncpy(games[game_count].title, title, sizeof(games[game_count].title) - 1);
    strncpy(games[game_count].author, author, sizeof(games[game_count].author) - 1);
    strncpy(games[game_count].path, path, sizeof(games[game_count].path) - 1);
    strncpy(games[game_count].badge, badge, sizeof(games[game_count].badge) - 1);
    strncpy(games[game_count].desc, desc, sizeof(games[game_count].desc) - 1);
    games[game_count].action = action;
    games[game_count].is_sd = is_sd;
    game_count++;
}

static void ensure_sd_folder_and_readme(const char *root) {
    mkdir(root, 0777);

    char readme_path[96];
    siprintf(readme_path, "%s/README.txt", root);

    FILE *f = fopen(readme_path, "w");
    if (f) {
        fputs("=====================================================\n", f);
        fputs("              SNAIL PIE DS - GAME DIRECTORY          \n", f);
        fputs("=====================================================\n", f);
        fputs("Drop any Deltarune, Undertale, or Fan Game in this   \n", f);
        fputs("folder at the root of your SD card:                  \n", f);
        fputs("  /snail-pie/                                        \n\n", f);
        fputs("Supported Game Formats:                              \n", f);
        fputs("- Subfolders with data.win / game.win:               \n", f);
        fputs("  /snail-pie/deltarune_ch1/                          \n", f);
        fputs("  /snail-pie/deltarune_ch2/                          \n", f);
        fputs("  /snail-pie/undertale/                              \n", f);
        fputs("  /snail-pie/ts_underswap/                           \n", f);
        fputs("  /snail-pie/undertale_yellow/                       \n", f);
        fputs("  /snail-pie/disbelief_papyrus/                      \n", f);
        fputs("  /snail-pie/sans_fight/                             \n\n", f);
        fputs("- Standalone .win or .nds files:                     \n", f);
        fputs("  /snail-pie/my_fan_game.nds                         \n\n", f);
        fputs("Snail Pie DS will automatically detect, compile, and \n", f);
        fputs("launch your games in real-time on Nintendo DS!       \n", f);
        fputs("=====================================================\n", f);
        fclose(f);
    }
}

static void create_sample_sd_games(void) {
    char subpath[96];

    // Create sample game subfolders in /snail-pie/
    const char *sample_folders[] = {
        "deltarune_ch2",
        "undertale_yellow",
        "ts_underswap",
        "disbelief_papyrus",
        "my_fan_game"
    };

    for (int i = 0; i < 5; i++) {
        siprintf(subpath, "%s/%s", sd_root_path, sample_folders[i]);
        mkdir(subpath, 0777);

        char manifest[128];
        siprintf(manifest, "%s/game.txt", subpath);
        FILE *f = fopen(manifest, "w");
        if (f) {
            fprintf(f, "Title: %s\nAuthor: Community Fan Game\nStatus: Ready for Snail Pie DS\n", sample_folders[i]);
            fclose(f);
        }
    }
}

static void scan_sd_card(void) {
    game_count = 0;

    // 1. Always populate built-in titles first
    add_game(
        "Deltarune: Chapter 1",
        "Toby Fox / Temmie Chang",
        "[ROM BUILT-IN]",
        "[BUILT-IN / DELTARUNE]",
        "Full Story Campaign, Overworld exploration, All 5 Bosses (Rudinn, Lancer, K. Round, King, Jevil), Cutscenes.",
        LAUNCH_DELTARUNE_CH1,
        false
    );

    add_game(
        "Undertale: Ruins & Toriel",
        "Toby Fox / Temmie Chang",
        "[ROM BUILT-IN]",
        "[BUILT-IN / UNDERTALE]",
        "Frisk, Flowey, Toriel & Froggit battle with Mercy, Spare, and Fight mechanics. Authentic battle theater.",
        LAUNCH_UNDERTALE_RUINS,
        false
    );

    add_game(
        "Sans Boss Fight: Bad Time",
        "Bad Time Simulator / Toby Fox",
        "[ROM BUILT-IN]",
        "[BUILT-IN / SANS]",
        "Bad Time Simulator: Gravity Blue Soul jumping over moving bone waves, Gaster Blasters, and Megalovania!",
        LAUNCH_SANS_FIGHT,
        false
    );

    add_game(
        "Undertale Yellow: Clover",
        "Team Undertale Yellow",
        "[SD OR BUILT-IN]",
        "[FAN GAME / YELLOW]",
        "Undertale Yellow Clover encounter with Justice Yellow Soul aiming and shooting mechanics.",
        LAUNCH_UNDERTALE_YELLOW,
        false
    );

    add_game(
        "TS!Underswap: Starlight",
        "Team Swapped",
        "[SD OR BUILT-IN]",
        "[FAN GAME / SWAP]",
        "TS!Underswap fan game encounter with swapped characters and custom battle patterns.",
        LAUNCH_TS_UNDERSWAP,
        false
    );

    // 2. Now check FAT SD card for /snail-pie/
    if (!fat_initialized) {
        fat_initialized = fatInitDefault();
    }

    if (fat_initialized) {
        // Check drive prefixes
        const char *prefixes[] = { "/snail-pie", "sd:/snail-pie", "fat:/snail-pie" };
        bool found = false;

        for (int p = 0; p < 3; p++) {
            DIR *d = opendir(prefixes[p]);
            if (d) {
                strncpy(sd_root_path, prefixes[p], sizeof(sd_root_path) - 1);
                closedir(d);
                found = true;
                break;
            }
        }

        if (!found) {
            strncpy(sd_root_path, "/snail-pie", sizeof(sd_root_path) - 1);
            ensure_sd_folder_and_readme(sd_root_path);
        }

        DIR *dir = opendir(sd_root_path);
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                if (ent->d_name[0] == '.') continue;
                if (strcmp(ent->d_name, "README.txt") == 0) continue;

                char game_title[48];
                siprintf(game_title, "Mod: %s", ent->d_name);

                char full_path[64];
                siprintf(full_path, "%s/%s", sd_root_path, ent->d_name);

                char desc[96];
                siprintf(desc, "SD Card game detected in %s. Ready to run on Snail Pie DS runner!", full_path);

                add_game(game_title, "SD Community", full_path, "[SD CARD]", desc, LAUNCH_SD_CUSTOM, true);
            }
            closedir(dir);
        }
    }
}

void launcher_init(void) {
    selected_idx = 0;
    scroll_offset = 0;
    anim_timer = 0;
    show_sd_guide = false;

    scan_sd_card();
    sound_play_bgm(&bgm_dark_world);
}

LauncherAction launcher_update(u32 keys_down, u32 keys_held, touchPosition touch) {
    anim_timer++;

    if (keys_down & KEY_SELECT) {
        show_sd_guide = !show_sd_guide;
        sound_play_sfx(&snd_menu_switch);
        return LAUNCH_NONE;
    }

    if (show_sd_guide) {
        if (keys_down & (KEY_A | KEY_B | KEY_START)) {
            show_sd_guide = false;
            sound_play_sfx(&snd_menu_switch);
        }
        return LAUNCH_NONE;
    }

    if (keys_down & KEY_UP) {
        if (selected_idx > 0) {
            selected_idx--;
            if (selected_idx < scroll_offset) scroll_offset = selected_idx;
            sound_play_sfx(&snd_menu_switch);
        }
    }
    if (keys_down & KEY_DOWN) {
        if (selected_idx < game_count - 1) {
            selected_idx++;
            if (selected_idx >= scroll_offset + 5) scroll_offset = selected_idx - 4;
            sound_play_sfx(&snd_menu_switch);
        }
    }

    // Touch screen selection
    if (keys_down & KEY_TOUCH) {
        if (touch.py >= 40 && touch.py <= 165) {
            int tapped_row = (touch.py - 42) / 24;
            int target_idx = scroll_offset + tapped_row;
            if (target_idx >= 0 && target_idx < game_count) {
                if (selected_idx == target_idx) {
                    // Double tap / confirm
                    sound_play_sfx(&snd_menu_confirm);
                    return games[selected_idx].action;
                } else {
                    selected_idx = target_idx;
                    sound_play_sfx(&snd_menu_switch);
                }
            }
        } else if (touch.py > 168) {
            if (touch.px < 80) { // [LAUNCH]
                sound_play_sfx(&snd_menu_confirm);
                return games[selected_idx].action;
            } else if (touch.px >= 80 && touch.px < 160) { // [CREATE SAMPLES]
                create_sample_sd_games();
                scan_sd_card();
                sound_play_sfx(&snd_bell);
            } else { // [HELP]
                show_sd_guide = true;
                sound_play_sfx(&snd_menu_switch);
            }
        }
    }

    if (keys_down & KEY_X) {
        create_sample_sd_games();
        scan_sd_card();
        sound_play_sfx(&snd_bell);
    }

    if (keys_down & KEY_Y) {
        scan_sd_card();
        sound_play_sfx(&snd_bell);
    }

    if (keys_down & (KEY_A | KEY_START)) {
        sound_play_sfx(&snd_menu_confirm);
        return games[selected_idx].action;
    }

    return LAUNCH_NONE;
}

const char *launcher_get_selected_game_path(void) {
    if (selected_idx >= 0 && selected_idx < game_count) {
        return games[selected_idx].path;
    }
    return "/snail-pie";
}

const char *launcher_get_selected_game_name(void) {
    if (selected_idx >= 0 && selected_idx < game_count) {
        return games[selected_idx].title;
    }
    return "Custom Game";
}

void launcher_render(void) {
    // ==========================================
    // TOP SCREEN: Gorgeous Snail Pie DS Banner & Info
    // ==========================================
    renderer_draw_gradient_v(SCREEN_TOP, 0, 0, SCREEN_W, SCREEN_H, RGB15(0, 0, 2), RGB15(3, 1, 7));

    // Twinkling stars
    for (int i = 0; i < 28; i++) {
        int sx = (i * 37 + 13) % SCREEN_W;
        int sy = (i * 29 + 7) % (SCREEN_H - 10);
        int twinkle = ((anim_timer / 8) + i) % 4;
        u16 scol = (twinkle == 0) ? COL_WHITE : (twinkle == 1 ? COL_LIGHT_GRAY : COL_DARK_GRAY);
        renderer_set_pixel(SCREEN_TOP, sx, sy, scol);
    }

    // Title banner with Snail Pie graphic
    renderer_draw_sprite(SCREEN_TOP, &spr_snail_pie, 12, 10);
    renderer_draw_text_scaled(SCREEN_TOP, 48, 10, "SNAIL PIE DS", COL_GOLD, 2);
    renderer_draw_text(SCREEN_TOP, 50, 30, "Universal Deltarune & Undertale SD Runner", COL_CYAN);

    // SD Card Status Pill
    renderer_draw_rect(SCREEN_TOP, 10, 48, 236, 20, RGB15(1, 2, 4));
    renderer_draw_rect_outline(SCREEN_TOP, 10, 48, 236, 20, fat_initialized ? COL_GREEN : COL_ORANGE, 1);

    char sd_status_str[64];
    siprintf(sd_status_str, "SD: %s (%s)", fat_initialized ? "MOUNTED" : "INITIALIZING", sd_root_path);
    renderer_draw_text(SCREEN_TOP, 16, 54, sd_status_str, fat_initialized ? COL_GREEN : COL_YELLOW);

    char games_found_str[32];
    siprintf(games_found_str, "%d GAMES", game_count);
    renderer_draw_text(SCREEN_TOP, 185, 54, games_found_str, COL_GOLD);

    // Game Preview Card
    renderer_draw_rect(SCREEN_TOP, 10, 74, 236, 110, RGB15(1, 1, 3));
    renderer_draw_rect_outline(SCREEN_TOP, 10, 74, 236, 110, COL_WHITE, 1);

    if (selected_idx >= 0 && selected_idx < game_count) {
        GameEntry *cur = &games[selected_idx];

        renderer_draw_text_scaled(SCREEN_TOP, 16, 80, cur->title, COL_YELLOW, 1);
        renderer_draw_text(SCREEN_TOP, 16, 96, cur->author, COL_CYAN);
        renderer_draw_text(SCREEN_TOP, 16, 112, cur->badge, COL_GREEN);

        char path_lbl[64];
        siprintf(path_lbl, "Location: %s", cur->path);
        renderer_draw_text(SCREEN_TOP, 16, 128, path_lbl, COL_GRAY);

        // Word-wrapped description
        renderer_draw_text(SCREEN_TOP, 16, 146, cur->desc, COL_WHITE);
    }

    // ==========================================
    // BOTTOM SCREEN: SD Card Game List
    // ==========================================
    renderer_clear(SCREEN_BOT, COL_BLACK);

    // Header
    renderer_draw_rect(SCREEN_BOT, 8, 6, 240, 24, RGB15(2, 1, 5));
    renderer_draw_rect_outline(SCREEN_BOT, 8, 6, 240, 24, COL_GOLD, 1);
    renderer_draw_text(SCREEN_BOT, 14, 12, "ROOT /snail-pie/ GAMES", COL_GOLD);
    renderer_draw_text(SCREEN_BOT, 145, 12, "[TOUCH / D-PAD]", COL_GRAY);

    // Game Cards List (5 items visible)
    for (int i = 0; i < 5; i++) {
        int idx = scroll_offset + i;
        if (idx >= game_count) break;

        int cy = 36 + i * 26;
        bool is_sel = (idx == selected_idx);

        renderer_draw_rect(SCREEN_BOT, 8, cy, 240, 24, is_sel ? RGB15(4, 2, 8) : RGB15(1, 1, 2));
        renderer_draw_rect_outline(SCREEN_BOT, 8, cy, 240, 24, is_sel ? COL_YELLOW : RGB15(8, 8, 12), is_sel ? 2 : 1);

        if (is_sel) {
            renderer_draw_sprite(SCREEN_BOT, &spr_soul, 12, cy + 4);
        } else {
            renderer_draw_char(SCREEN_BOT, 14, cy + 6, '*', COL_DARK_GRAY);
        }

        renderer_draw_text(SCREEN_BOT, 32, cy + 6, games[idx].title, is_sel ? COL_YELLOW : COL_WHITE);

        // Badge pill on right
        renderer_draw_text(SCREEN_BOT, 165, cy + 6, games[idx].badge, is_sel ? COL_CYAN : COL_GRAY);
    }

    // Scrollbar indicator
    if (game_count > 5) {
        int sb_y = 36 + (scroll_offset * 100) / game_count;
        renderer_draw_rect(SCREEN_BOT, 250, 36, 4, 128, RGB15(3, 3, 6));
        renderer_draw_rect(SCREEN_BOT, 250, sb_y, 4, 24, COL_YELLOW);
    }

    // Bottom Action Bar
    renderer_draw_rect(SCREEN_BOT, 0, 168, SCREEN_W, 24, RGB15(2, 2, 4));
    renderer_draw_rect(SCREEN_BOT, 6, 170, 72, 20, RGB15(4, 2, 8));
    renderer_draw_rect_outline(SCREEN_BOT, 6, 170, 72, 20, COL_YELLOW, 1);
    renderer_draw_text(SCREEN_BOT, 12, 175, "[A] LAUNCH", COL_YELLOW);

    renderer_draw_rect(SCREEN_BOT, 82, 170, 88, 20, RGB15(2, 3, 5));
    renderer_draw_rect_outline(SCREEN_BOT, 82, 170, 88, 20, COL_CYAN, 1);
    renderer_draw_text(SCREEN_BOT, 86, 175, "[X] SAMPLES", COL_CYAN);

    renderer_draw_rect(SCREEN_BOT, 174, 170, 76, 20, RGB15(3, 1, 3));
    renderer_draw_rect_outline(SCREEN_BOT, 174, 170, 76, 20, COL_MAGENTA, 1);
    renderer_draw_text(SCREEN_BOT, 180, 175, "[SEL] GUIDE", COL_MAGENTA);

    // SD Guide Popup Modal
    if (show_sd_guide) {
        renderer_draw_rect(SCREEN_BOT, 12, 12, 232, 168, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, 12, 12, 232, 168, COL_GOLD, 2);

        renderer_draw_text_scaled(SCREEN_BOT, 24, 18, "SNAIL PIE DS - SD GUIDE", COL_GOLD, 1);
        renderer_draw_text(SCREEN_BOT, 20, 36, "HOW TO ADD GAMES & MODS:", COL_CYAN);
        renderer_draw_text(SCREEN_BOT, 20, 52, "1. Insert SD card into PC / Phone.", COL_WHITE);
        renderer_draw_text(SCREEN_BOT, 20, 68, "2. Create folder 'snail-pie' on SD root.", COL_WHITE);
        renderer_draw_text(SCREEN_BOT, 20, 84, "3. Put Deltarune / Undertale games or", COL_WHITE);
        renderer_draw_text(SCREEN_BOT, 20, 98, "   fan game folders inside /snail-pie/!", COL_YELLOW);
        renderer_draw_text(SCREEN_BOT, 20, 114, "4. Press [X] to generate sample folders.", COL_CYAN);
        renderer_draw_text(SCREEN_BOT, 20, 130, "5. Launch Snail Pie DS to play instantly!", COL_GREEN);
        renderer_draw_text(SCREEN_BOT, 20, 154, "Press [A / B / SELECT] to close.", COL_GRAY);
    }
}
