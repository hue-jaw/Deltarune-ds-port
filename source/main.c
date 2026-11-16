#include <nds.h>
#include "renderer.h"
#include "sound_manager.h"
#include "dialogue.h"
#include "title_screen.h"
#include "cutscene.h"
#include "overworld.h"
#include "battle.h"

typedef enum {
    GAME_STATE_TITLE,
    GAME_STATE_CUTSCENE,
    GAME_STATE_OVERWORLD,
    GAME_STATE_BATTLE
} GameMode;

int main(void) {
    // Initialize hardware systems
    renderer_init();
    sound_manager_init();
    dialogue_init();

    GameMode mode = GAME_STATE_TITLE;
    title_screen_init();

    touchPosition touch;

    while (1) {
        scanKeys();
        u32 keys_down = keysDown();
        u32 keys_held = keysHeld();
        touchRead(&touch);

        switch (mode) {
            case GAME_STATE_TITLE: {
                TitleChoice choice = title_screen_update(keys_down, keys_held, touch);
                title_screen_render();

                if (choice == TITLE_CHOICE_STORY) {
                    mode = GAME_STATE_CUTSCENE;
                    cutscene_init(CUTSCENE_INTRO_SCHOOL);
                } else if (choice == TITLE_CHOICE_FIELD) {
                    mode = GAME_STATE_OVERWORLD;
                    overworld_init(ROOM_FIELD);
                } else if (choice == TITLE_CHOICE_LANCER) {
                    mode = GAME_STATE_BATTLE;
                    battle_init(BATTLE_ENEMY_LANCER);
                } else if (choice == TITLE_CHOICE_JEVIL) {
                    mode = GAME_STATE_BATTLE;
                    battle_init(BATTLE_ENEMY_JEVIL);
                } else if (choice == TITLE_CHOICE_LEGEND) {
                    mode = GAME_STATE_CUTSCENE;
                    cutscene_init(CUTSCENE_LEGEND);
                }
                break;
            }

            case GAME_STATE_CUTSCENE: {
                cutscene_update(keys_down, keys_held);
                cutscene_render();

                if (cutscene_is_finished()) {
                    mode = GAME_STATE_OVERWORLD;
                    overworld_init(ROOM_DARK_CLIFFS);
                }
                break;
            }

            case GAME_STATE_OVERWORLD: {
                // If SELECT pressed, quick return to title
                if (keys_down & KEY_SELECT) {
                    mode = GAME_STATE_TITLE;
                    title_screen_init();
                    break;
                }

                overworld_update(keys_down, keys_held);
                overworld_render();

                int trigger_enemy = 0;
                if (overworld_check_battle_trigger(&trigger_enemy)) {
                    mode = GAME_STATE_BATTLE;
                    battle_init((BattleEnemyType)trigger_enemy);
                }
                break;
            }

            case GAME_STATE_BATTLE: {
                // If SELECT pressed, quick return to title
                if (keys_down & KEY_SELECT) {
                    mode = GAME_STATE_TITLE;
                    title_screen_init();
                    break;
                }

                battle_update(keys_down, keys_held, touch);
                battle_render();

                if (battle_is_finished()) {
                    mode = GAME_STATE_OVERWORLD;
                    overworld_return_from_battle();
                }
                break;
            }
        }

        renderer_present();
    }

    return 0;
}
