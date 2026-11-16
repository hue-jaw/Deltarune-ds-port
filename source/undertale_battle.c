#include "undertale_battle.h"
#include "renderer.h"
#include "sound_manager.h"
#include "assets_gfx.h"
#include "assets_undertale.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define UT_MAX_PELLETS 24

typedef enum {
    UT_STATE_INTRO,
    UT_STATE_MENU,
    UT_STATE_ACT_MENU,
    UT_STATE_ITEM_MENU,
    UT_STATE_MERCY_MENU,
    UT_STATE_ATTACK_METER,
    UT_STATE_EXECUTE_ATTACK,
    UT_STATE_TAUNT,
    UT_STATE_BULLET_HELL,
    UT_STATE_VICTORY,
    UT_STATE_GAMEOVER
} UtState;

typedef struct {
    float x, y;
    float vx, vy;
    bool active;
} UtPellet;

#define UT_BOX_X 52
#define UT_BOX_Y 42
#define UT_BOX_W 152
#define UT_BOX_H 84

static UtEnemyType cur_enemy = UT_ENEMY_FROGGIT;
static UtState state = UT_STATE_INTRO;
static int state_timer = 0;
static bool finished = false;

// Player
static int player_hp = 20;
static int player_max_hp = 20;
static int cur_btn = 0; // 0=FIGHT, 1=ACT, 2=ITEM, 3=MERCY
static int act_sel = 0;

// Enemy
static const char *enemy_name = "Froggit";
static int enemy_hp = 30;
static int enemy_max_hp = 30;
static bool enemy_spareable = false;
static bool enemy_is_hurt = false;

// Soul & Pellets
static float soul_x = UT_BOX_X + 76;
static float soul_y = UT_BOX_Y + 42;
static int soul_invuln = 0;
static UtPellet pellets[UT_MAX_PELLETS];
static int attack_timer = 0;
static float meter_x = 220.0f;

static void clear_pellets(void) {
    for (int i = 0; i < UT_MAX_PELLETS; i++) pellets[i].active = false;
}

static void spawn_pellet(float x, float y, float vx, float vy) {
    for (int i = 0; i < UT_MAX_PELLETS; i++) {
        if (!pellets[i].active) {
            pellets[i].x = x;
            pellets[i].y = y;
            pellets[i].vx = vx;
            pellets[i].vy = vy;
            pellets[i].active = true;
            return;
        }
    }
}

void undertale_battle_init(UtEnemyType enemy_type) {
    cur_enemy = enemy_type;
    state = UT_STATE_INTRO;
    state_timer = 0;
    finished = false;
    player_hp = 20;
    player_max_hp = 20;
    cur_btn = 0;
    act_sel = 0;
    enemy_spareable = false;
    enemy_is_hurt = false;

    if (enemy_type == UT_ENEMY_FROGGIT) {
        enemy_name = "Froggit";
        enemy_hp = 30;
        enemy_max_hp = 30;
        sound_play_bgm(&bgm_rude_buster);
    } else if (enemy_type == UT_ENEMY_CLOVER) {
        enemy_name = "Clover";
        enemy_hp = 80;
        enemy_max_hp = 80;
        sound_play_bgm(&bgm_dark_world);
    } else {
        enemy_name = "Underswap Foe";
        enemy_hp = 60;
        enemy_max_hp = 60;
        sound_play_bgm(&bgm_rude_buster);
    }

    clear_pellets();
}

bool undertale_battle_is_finished(void) {
    return finished;
}

void undertale_battle_update(u32 keys_down, u32 keys_held, touchPosition touch) {
    state_timer++;
    if (soul_invuln > 0) soul_invuln--;

    switch (state) {
        case UT_STATE_INTRO:
            if (state_timer > 30 || (keys_down & KEY_A)) {
                state = UT_STATE_MENU;
                state_timer = 0;
            }
            break;

        case UT_STATE_MENU:
            if (keys_down & KEY_LEFT) {
                cur_btn = (cur_btn + 3) % 4;
                sound_play_sfx(&snd_menu_switch);
            }
            if (keys_down & KEY_RIGHT) {
                cur_btn = (cur_btn + 1) % 4;
                sound_play_sfx(&snd_menu_switch);
            }

            if (keys_down & KEY_TOUCH) {
                if (touch.py >= 155 && touch.py <= 185) {
                    int b = (touch.px - 20) / 56;
                    if (b >= 0 && b < 4) {
                        cur_btn = b;
                        sound_play_sfx(&snd_menu_confirm);
                        keys_down |= KEY_A;
                    }
                }
            }

            if (keys_down & KEY_A) {
                sound_play_sfx(&snd_menu_confirm);
                if (cur_btn == 0) { // FIGHT
                    state = UT_STATE_ATTACK_METER;
                    meter_x = 220.0f;
                } else if (cur_btn == 1) { // ACT
                    state = UT_STATE_ACT_MENU;
                    act_sel = 0;
                } else if (cur_btn == 2) { // ITEM
                    state = UT_STATE_ITEM_MENU;
                } else if (cur_btn == 3) { // MERCY
                    state = UT_STATE_MERCY_MENU;
                }
            }
            break;

        case UT_STATE_ACT_MENU:
            if (keys_down & (KEY_UP | KEY_DOWN)) {
                act_sel = 1 - act_sel;
                sound_play_sfx(&snd_menu_switch);
            }
            if (keys_down & KEY_B) {
                state = UT_STATE_MENU;
                sound_play_sfx(&snd_menu_switch);
            }
            if (keys_down & KEY_A) {
                sound_play_sfx(&snd_bell);
                if (act_sel == 1) {
                    enemy_spareable = true; // Compliment makes spareable!
                }
                state = UT_STATE_TAUNT;
                state_timer = 0;
            }
            break;

        case UT_STATE_ITEM_MENU:
            if (keys_down & KEY_B) {
                state = UT_STATE_MENU;
                sound_play_sfx(&snd_menu_switch);
            }
            if (keys_down & KEY_A) {
                sound_play_sfx(&snd_bell);
                player_hp += 10;
                if (player_hp > player_max_hp) player_hp = player_max_hp;
                state = UT_STATE_TAUNT;
                state_timer = 0;
            }
            break;

        case UT_STATE_MERCY_MENU:
            if (keys_down & KEY_B) {
                state = UT_STATE_MENU;
                sound_play_sfx(&snd_menu_switch);
            }
            if (keys_down & KEY_A) {
                if (enemy_spareable) {
                    sound_play_sfx(&snd_bell);
                    state = UT_STATE_VICTORY;
                    state_timer = 0;
                } else {
                    state = UT_STATE_TAUNT;
                    state_timer = 0;
                }
            }
            break;

        case UT_STATE_ATTACK_METER:
            meter_x -= 4.2f;
            if (keys_down & KEY_A) {
                sound_play_sfx(&snd_attack);
                sound_play_sfx(&snd_damage);
                enemy_hp -= 18;
                enemy_is_hurt = true;
                state = UT_STATE_EXECUTE_ATTACK;
                state_timer = 0;
            } else if (meter_x < 45.0f) {
                state = UT_STATE_EXECUTE_ATTACK;
                state_timer = 0;
            }
            break;

        case UT_STATE_EXECUTE_ATTACK:
            if (state_timer > 40) {
                enemy_is_hurt = false;
                if (enemy_hp <= 0) {
                    state = UT_STATE_VICTORY;
                    state_timer = 0;
                } else {
                    state = UT_STATE_TAUNT;
                    state_timer = 0;
                }
            }
            break;

        case UT_STATE_TAUNT:
            if (state_timer > 50 || (keys_down & KEY_A)) {
                state = UT_STATE_BULLET_HELL;
                state_timer = 0;
                attack_timer = 0;
                clear_pellets();
                soul_x = UT_BOX_X + UT_BOX_W / 2;
                soul_y = UT_BOX_Y + UT_BOX_H / 2;
            }
            break;

        case UT_STATE_BULLET_HELL:
            attack_timer++;

            // Soul movement
            {
                float spd = 2.0f;
                if (keys_held & KEY_UP) soul_y -= spd;
                if (keys_held & KEY_DOWN) soul_y += spd;
                if (keys_held & KEY_LEFT) soul_x -= spd;
                if (keys_held & KEY_RIGHT) soul_x += spd;

                if (soul_x < UT_BOX_X + 8) soul_x = UT_BOX_X + 8;
                if (soul_x > UT_BOX_X + UT_BOX_W - 8) soul_x = UT_BOX_X + UT_BOX_W - 8;
                if (soul_y < UT_BOX_Y + 8) soul_y = UT_BOX_Y + 8;
                if (soul_y > UT_BOX_Y + UT_BOX_H - 8) soul_y = UT_BOX_Y + UT_BOX_H - 8;
            }

            // Spawn flies / pellets
            if (attack_timer % 24 == 0 && attack_timer < 240) {
                float side_x = UT_BOX_X + 10 + (rand() % (UT_BOX_W - 20));
                float vy = 1.6f + ((rand() % 10) / 10.0f);
                float vx = ((rand() % 20) - 10) / 12.0f;
                spawn_pellet(side_x, UT_BOX_Y + 4, vx, vy);
            }

            // Update pellets & collisions
            for (int i = 0; i < UT_MAX_PELLETS; i++) {
                if (!pellets[i].active) continue;
                pellets[i].x += pellets[i].vx;
                pellets[i].y += pellets[i].vy;

                if (pellets[i].y > UT_BOX_Y + UT_BOX_H + 8) {
                    pellets[i].active = false;
                    continue;
                }

                float dx = pellets[i].x - soul_x;
                float dy = pellets[i].y - soul_y;
                if (dx*dx + dy*dy < 36.0f && soul_invuln == 0) {
                    soul_invuln = 35;
                    player_hp -= 4;
                    sound_play_sfx(&snd_hurt);
                    if (player_hp <= 0) {
                        player_hp = 0;
                        state = UT_STATE_GAMEOVER;
                        state_timer = 0;
                    }
                }
            }

            if (attack_timer > 260) {
                clear_pellets();
                state = UT_STATE_MENU;
                state_timer = 0;
            }
            break;

        case UT_STATE_VICTORY:
            if (keys_down & KEY_A) {
                finished = true;
            }
            break;

        case UT_STATE_GAMEOVER:
            if (keys_down & KEY_A) {
                undertale_battle_init(cur_enemy);
            }
            break;
    }
}

void undertale_battle_render(void) {
    // ==========================================
    // TOP SCREEN: Froggit / Toriel Theater
    // ==========================================
    renderer_clear(SCREEN_TOP, COL_BLACK);

    int fx = 114;
    int fy = 65;

    if (cur_enemy == UT_ENEMY_FROGGIT) {
        if (enemy_is_hurt) {
            renderer_draw_sprite_tint(SCREEN_TOP, &spr_froggit, fx, fy, COL_WHITE);
        } else {
            int hop = (state == UT_STATE_BULLET_HELL && (state_timer / 12) % 2 == 0) ? -4 : 0;
            renderer_draw_sprite(SCREEN_TOP, &spr_froggit, fx, fy + hop);
        }
    } else if (cur_enemy == UT_ENEMY_CLOVER) {
        renderer_draw_sprite(SCREEN_TOP, &spr_soul_yellow, fx + 6, fy + 4);
    } else {
        renderer_draw_sprite(SCREEN_TOP, &spr_rudinn_b_idle[0], fx, fy);
    }

    // Name & HP Bar
    renderer_draw_text(SCREEN_TOP, 100, 28, enemy_name, enemy_spareable ? COL_YELLOW : COL_WHITE);
    renderer_draw_rect(SCREEN_TOP, 100, 44, 56, 4, COL_DARK_GRAY);
    int e_hp = (enemy_hp * 56) / enemy_max_hp;
    if (e_hp < 0) e_hp = 0;
    renderer_draw_rect(SCREEN_TOP, 100, 44, e_hp, 4, COL_GREEN);

    // Dialogue Bubble
    if (state == UT_STATE_TAUNT || state == UT_STATE_INTRO) {
        renderer_draw_rect(SCREEN_TOP, 142, 35, 100, 36, COL_WHITE);
        renderer_draw_rect_outline(SCREEN_TOP, 142, 35, 100, 36, COL_BLACK, 2);
        if (cur_enemy == UT_ENEMY_FROGGIT) {
            renderer_draw_text(SCREEN_TOP, 146, 40, "Ribbit, ribbit.\n(Life is tough.)", COL_BLACK);
        } else {
            renderer_draw_text(SCREEN_TOP, 146, 40, "Justice shines\nbrightly today!", COL_BLACK);
        }
    }

    // ==========================================
    // BOTTOM SCREEN: Arena, HUD & Controls
    // ==========================================
    renderer_clear(SCREEN_BOT, COL_BLACK);

    // Player HUD
    renderer_draw_text(SCREEN_BOT, 14, 18, "FRISK   LV 1", COL_WHITE);
    renderer_draw_text(SCREEN_BOT, 120, 18, "HP", COL_YELLOW);

    renderer_draw_rect(SCREEN_BOT, 142, 18, 40, 10, COL_RED);
    int hp_bar = (player_hp * 40) / player_max_hp;
    if (hp_bar < 0) hp_bar = 0;
    renderer_draw_rect(SCREEN_BOT, 142, 18, hp_bar, 10, COL_YELLOW);

    char hp_str[24];
    siprintf(hp_str, "%d / %d", player_hp, player_max_hp);
    renderer_draw_text(SCREEN_BOT, 190, 18, hp_str, COL_WHITE);

    // Box or Menu Area
    if (state == UT_STATE_BULLET_HELL) {
        renderer_draw_rect(SCREEN_BOT, UT_BOX_X, UT_BOX_Y, UT_BOX_W, UT_BOX_H, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, UT_BOX_X, UT_BOX_Y, UT_BOX_W, UT_BOX_H, COL_WHITE, 3);

        // Pellets
        for (int i = 0; i < UT_MAX_PELLETS; i++) {
            if (!pellets[i].active) continue;
            renderer_draw_sprite(SCREEN_BOT, &spr_bullet_pellet, (int)pellets[i].x - 4, (int)pellets[i].y - 4);
        }

        // Soul
        if (soul_invuln == 0 || (soul_invuln / 3) % 2 == 0) {
            if (cur_enemy == UT_ENEMY_CLOVER) {
                renderer_draw_sprite(SCREEN_BOT, &spr_soul_yellow, (int)soul_x - 8, (int)soul_y - 8);
            } else {
                renderer_draw_sprite(SCREEN_BOT, &spr_soul, (int)soul_x - 8, (int)soul_y - 8);
            }
        }

    } else if (state == UT_STATE_ATTACK_METER) {
        renderer_draw_rect(SCREEN_BOT, UT_BOX_X, UT_BOX_Y, UT_BOX_W, UT_BOX_H, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, UT_BOX_X, UT_BOX_Y, UT_BOX_W, UT_BOX_H, COL_WHITE, 2);

        renderer_draw_text(SCREEN_BOT, UT_BOX_X + 16, UT_BOX_Y + 12, "* PRESS [A] TO STRIKE!", COL_YELLOW);
        renderer_draw_rect(SCREEN_BOT, UT_BOX_X + 20, UT_BOX_Y + 40, 110, 16, RGB15(1, 1, 4));
        renderer_draw_rect_outline(SCREEN_BOT, UT_BOX_X + 20, UT_BOX_Y + 40, 110, 16, COL_WHITE, 1);
        renderer_draw_rect(SCREEN_BOT, (int)meter_x, UT_BOX_Y + 38, 4, 20, COL_WHITE);

    } else if (state == UT_STATE_ACT_MENU) {
        renderer_draw_rect(SCREEN_BOT, UT_BOX_X, UT_BOX_Y, UT_BOX_W, UT_BOX_H, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, UT_BOX_X, UT_BOX_Y, UT_BOX_W, UT_BOX_H, COL_WHITE, 2);

        renderer_draw_text(SCREEN_BOT, UT_BOX_X + 20, UT_BOX_Y + 18, "* Check", (act_sel == 0) ? COL_YELLOW : COL_WHITE);
        renderer_draw_text(SCREEN_BOT, UT_BOX_X + 20, UT_BOX_Y + 42, "* Compliment", (act_sel == 1) ? COL_YELLOW : COL_WHITE);
        renderer_draw_char(SCREEN_BOT, UT_BOX_X + 8, UT_BOX_Y + 18 + act_sel * 24, '*', COL_YELLOW);

    } else if (state == UT_STATE_ITEM_MENU) {
        renderer_draw_rect(SCREEN_BOT, UT_BOX_X, UT_BOX_Y, UT_BOX_W, UT_BOX_H, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, UT_BOX_X, UT_BOX_Y, UT_BOX_W, UT_BOX_H, COL_WHITE, 2);

        renderer_draw_text(SCREEN_BOT, UT_BOX_X + 20, UT_BOX_Y + 24, "* Monster Candy (+10 HP)", COL_YELLOW);
        renderer_draw_char(SCREEN_BOT, UT_BOX_X + 8, UT_BOX_Y + 24, '*', COL_YELLOW);

    } else if (state == UT_STATE_MERCY_MENU) {
        renderer_draw_rect(SCREEN_BOT, UT_BOX_X, UT_BOX_Y, UT_BOX_W, UT_BOX_H, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, UT_BOX_X, UT_BOX_Y, UT_BOX_W, UT_BOX_H, COL_WHITE, 2);

        renderer_draw_text(SCREEN_BOT, UT_BOX_X + 20, UT_BOX_Y + 24, "* Spare", enemy_spareable ? COL_YELLOW : COL_WHITE);
        renderer_draw_char(SCREEN_BOT, UT_BOX_X + 8, UT_BOX_Y + 24, '*', COL_YELLOW);

    } else if (state == UT_STATE_VICTORY) {
        renderer_draw_rect(SCREEN_BOT, UT_BOX_X, UT_BOX_Y, UT_BOX_W, UT_BOX_H, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, UT_BOX_X, UT_BOX_Y, UT_BOX_W, UT_BOX_H, COL_WHITE, 2);

        renderer_draw_text(SCREEN_BOT, UT_BOX_X + 16, UT_BOX_Y + 20, "* YOU WON!", COL_YELLOW);
        renderer_draw_text(SCREEN_BOT, UT_BOX_X + 16, UT_BOX_Y + 42, "* Got 0 EXP and 10 Gold.", COL_WHITE);
        renderer_draw_text(SCREEN_BOT, UT_BOX_X + 16, UT_BOX_Y + 62, "* Press [A] to continue.", COL_GRAY);

    } else {
        renderer_draw_rect(SCREEN_BOT, UT_BOX_X, UT_BOX_Y, UT_BOX_W, UT_BOX_H, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, UT_BOX_X, UT_BOX_Y, UT_BOX_W, UT_BOX_H, COL_WHITE, 2);

        renderer_draw_text(SCREEN_BOT, UT_BOX_X + 14, UT_BOX_Y + 18, "* Froggit hopped close!", COL_WHITE);
        renderer_draw_text(SCREEN_BOT, UT_BOX_X + 14, UT_BOX_Y + 38, "* Life is tough for this frog.", COL_WHITE);
    }

    // 4 Bottom Command Buttons: FIGHT, ACT, ITEM, MERCY
    const char *btn_labels[4] = { "FIGHT", "ACT", "ITEM", "MERCY" };
    for (int b = 0; b < 4; b++) {
        int bx = 16 + b * 58;
        int by = 160;
        bool is_sel = (b == cur_btn && state == UT_STATE_MENU);

        renderer_draw_rect(SCREEN_BOT, bx, by, 52, 22, is_sel ? RGB15(31, 16, 0) : RGB15(8, 4, 0));
        renderer_draw_rect_outline(SCREEN_BOT, bx, by, 52, 22, is_sel ? COL_YELLOW : COL_ORANGE, is_sel ? 2 : 1);
        renderer_draw_text(SCREEN_BOT, bx + 10, by + 6, btn_labels[b], is_sel ? COL_YELLOW : COL_ORANGE);
    }

    if (state == UT_STATE_GAMEOVER) {
        renderer_draw_rect(SCREEN_BOT, 20, 20, 216, 150, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, 20, 20, 216, 150, COL_RED, 2);

        renderer_draw_text_scaled(SCREEN_BOT, 60, 35, "GAME OVER", COL_RED, 2);
        renderer_draw_text(SCREEN_BOT, 40, 80, "* It cannot end now!", COL_WHITE);
        renderer_draw_text(SCREEN_BOT, 40, 96, "* Stay determined...", COL_YELLOW);
        renderer_draw_text(SCREEN_BOT, 40, 130, "* Press [A] to try again.", COL_WHITE);
    }
}
