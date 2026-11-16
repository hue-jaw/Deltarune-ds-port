#include <nds.h>
#include "renderer.h"
#include "sound_manager.h"
#include "dialogue.h"
#include "launcher.h"
#include "title_screen.h"
#include "cutscene.h"
#include "overworld.h"
#include "battle.h"
#include "sans_fight.h"
#include "undertale_battle.h"
#include "custom_loader.h"

typedef enum {
    MODE_LAUNCHER,
    MODE_CUSTOM_LOADING,
    MODE_DELTARUNE_TITLE,
    MODE_DELTARUNE_CUTSCENE,
    MODE_DELTARUNE_OVERWORLD,
    MODE_DELTARUNE_BATTLE,
    MODE_SANS_FIGHT,
    MODE_UNDERTALE_BATTLE
} GameMode;

int main(void) {
    powerOn(POWER_ALL_2D);
    irqInit();
    irqEnable(IRQ_VBLANK);

    renderer_init();
    sound_manager_init();
    dialogue_init();

    GameMode mode = MODE_LAUNCHER;
    launcher_init();

    touchPosition touch;

    while (1) {
        scanKeys();
        u32 keys_down = keysDown();
        u32 keys_held = keysHeld();
        touchRead(&touch);

        switch (mode) {
            case MODE_LAUNCHER: {
                LauncherAction action = launcher_update(keys_down, keys_held, touch);
                launcher_render();

                if (action == LAUNCH_DELTARUNE_CH1) {
                    mode = MODE_DELTARUNE_TITLE;
                    title_screen_init();
                } else if (action == LAUNCH_UNDERTALE_RUINS) {
                    mode = MODE_UNDERTALE_BATTLE;
                    undertale_battle_init(UT_ENEMY_FROGGIT);
                } else if (action == LAUNCH_SANS_FIGHT) {
                    mode = MODE_SANS_FIGHT;
                    sans_fight_init();
                } else if (action == LAUNCH_UNDERTALE_YELLOW) {
                    mode = MODE_UNDERTALE_BATTLE;
                    undertale_battle_init(UT_ENEMY_CLOVER);
                } else if (action == LAUNCH_TS_UNDERSWAP) {
                    mode = MODE_UNDERTALE_BATTLE;
                    undertale_battle_init(UT_ENEMY_SWAP);
                } else if (action == LAUNCH_SD_CUSTOM) {
                    mode = MODE_CUSTOM_LOADING;
                    custom_loader_init(launcher_get_selected_game_name(), launcher_get_selected_game_path());
                }
                break;
            }

            case MODE_CUSTOM_LOADING: {
                if (keys_down & KEY_SELECT) {
                    mode = MODE_LAUNCHER;
                    launcher_init();
                    break;
                }

                if (custom_loader_update(keys_down, keys_held)) {
                    // Custom game loaded from SD card -> run battle or overworld engine!
                    mode = MODE_DELTARUNE_BATTLE;
                    battle_init(BATTLE_ENEMY_JEVIL);
                }
                custom_loader_render();
                break;
            }

            case MODE_DELTARUNE_TITLE: {
                if (keys_down & KEY_SELECT) {
                    mode = MODE_LAUNCHER;
                    launcher_init();
                    break;
                }

                TitleChoice choice = title_screen_update(keys_down, keys_held, touch);
                title_screen_render();

                if (choice == TITLE_CHOICE_STORY) {
                    mode = MODE_DELTARUNE_CUTSCENE;
                    cutscene_init(CUTSCENE_INTRO_SCHOOL);
                } else if (choice == TITLE_CHOICE_FIELD) {
                    mode = MODE_DELTARUNE_OVERWORLD;
                    overworld_init(ROOM_FIELD);
                } else if (choice == TITLE_CHOICE_LANCER) {
                    mode = MODE_DELTARUNE_BATTLE;
                    battle_init(BATTLE_ENEMY_LANCER);
                } else if (choice == TITLE_CHOICE_KROUND) {
                    mode = MODE_DELTARUNE_BATTLE;
                    battle_init(BATTLE_ENEMY_KROUND);
                } else if (choice == TITLE_CHOICE_KING) {
                    mode = MODE_DELTARUNE_BATTLE;
                    battle_init(BATTLE_ENEMY_KING);
                } else if (choice == TITLE_CHOICE_JEVIL) {
                    mode = MODE_DELTARUNE_BATTLE;
                    battle_init(BATTLE_ENEMY_JEVIL);
                } else if (choice == TITLE_CHOICE_LEGEND) {
                    mode = MODE_DELTARUNE_CUTSCENE;
                    cutscene_init(CUTSCENE_LEGEND);
                }
                break;
            }

            case MODE_DELTARUNE_CUTSCENE: {
                if (keys_down & KEY_SELECT) {
                    mode = MODE_LAUNCHER;
                    launcher_init();
                    break;
                }

                cutscene_update(keys_down, keys_held);
                cutscene_render();

                if (cutscene_is_finished()) {
                    mode = MODE_DELTARUNE_OVERWORLD;
                    overworld_init(ROOM_DARK_CLIFFS);
                }
                break;
            }

            case MODE_DELTARUNE_OVERWORLD: {
                if (keys_down & KEY_SELECT) {
                    mode = MODE_LAUNCHER;
                    launcher_init();
                    break;
                }

                overworld_update(keys_down, keys_held);
                overworld_render();

                int trigger_enemy = 0;
                if (overworld_check_battle_trigger(&trigger_enemy)) {
                    mode = MODE_DELTARUNE_BATTLE;
                    battle_init((BattleEnemyType)trigger_enemy);
                }
                break;
            }

            case MODE_DELTARUNE_BATTLE: {
                if (keys_down & KEY_SELECT) {
                    mode = MODE_LAUNCHER;
                    launcher_init();
                    break;
                }

                battle_update(keys_down, keys_held, touch);
                battle_render();

                if (battle_is_finished()) {
                    mode = MODE_DELTARUNE_OVERWORLD;
                    overworld_return_from_battle();
                }
                break;
            }

            case MODE_SANS_FIGHT: {
                if (keys_down & KEY_SELECT) {
                    mode = MODE_LAUNCHER;
                    launcher_init();
                    break;
                }

                sans_fight_update(keys_down, keys_held, touch);
                sans_fight_render();

                if (sans_fight_is_finished()) {
                    mode = MODE_LAUNCHER;
                    launcher_init();
                }
                break;
            }

            case MODE_UNDERTALE_BATTLE: {
                if (keys_down & KEY_SELECT) {
                    mode = MODE_LAUNCHER;
                    launcher_init();
                    break;
                }

                undertale_battle_update(keys_down, keys_held, touch);
                undertale_battle_render();

                if (undertale_battle_is_finished()) {
                    mode = MODE_LAUNCHER;
                    launcher_init();
                }
                break;
            }
        }

        renderer_present();
    }

    return 0;
}
