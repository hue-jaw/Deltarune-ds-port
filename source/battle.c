#include "battle.h"
#include "renderer.h"
#include "sound_manager.h"
#include "assets_gfx.h"
#include "assets_audio.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BULLETS 48

typedef enum {
    B_STATE_INTRO,
    B_STATE_MENU_BTN,
    B_STATE_TARGET_SELECT,
    B_STATE_ACT_SELECT,
    B_STATE_ITEM_SELECT,
    B_STATE_ATTACK_METER,
    B_STATE_EXECUTE_ACTIONS,
    B_STATE_ENEMY_TAUNT,
    B_STATE_BULLET_HELL,
    B_STATE_VICTORY,
    B_STATE_GAMEOVER
} BattleState;

typedef struct {
    float x, y;
    float vx, vy;
    int type; // 0=spade, 1=diamond, 2=scythe, 3=pellet
    bool active;
    bool grazed;
    float angle;
    float rot_speed;
} Bullet;

typedef struct {
    const char *name;
    int max_hp;
    int hp;
    int max_tp;
    int act_selected; // 0=none, 1=fight, 2=act, 3=item, 4=spare, 5=defend
    int anim_timer;
    int anim_state; // 0=idle, 1=attack, 2=defend, 3=spell, 4=hurt
} PartyMember;

typedef struct {
    const char *name;
    int max_hp;
    int hp;
    int mercy; // 0..100
    int anim_timer;
    bool is_hurt;
    const char *taunt_text;
} Enemy;

static BattleEnemyType cur_enemy_type = BATTLE_ENEMY_RUDINN;
static BattleState state = B_STATE_INTRO;
static int state_timer = 0;
static bool battle_finished = false;

// Party members
static PartyMember party[3];
static int active_party_idx = 0;
static int cur_btn_idx = 0; // 0=FIGHT, 1=ACT, 2=ITEM, 3=SPARE, 4=DEFEND
static int sub_menu_idx = 0;

// Enemy
static Enemy enemy;
static int kround_bow_count = 0;

// Attack timing bar
static float meter_pos_x = 240.0f;
static float meter_speed = 4.2f;
static int attack_damage_dealt = 0;
static int damage_display_timer = 0;
static int damage_display_val = 0;

// TP Gauge (0..100)
static int tension_points = 24;

// Bullet hell arena
#define ARENA_X 72
#define ARENA_Y 44
#define ARENA_W 112
#define ARENA_H 84

static float soul_x = 128.0f;
static float soul_y = 86.0f;
static int soul_invuln = 0;
static Bullet bullets[MAX_BULLETS];
static int wave_timer = 0;
static int graze_effect_timer = 0;

static void clear_bullets(void) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }
}

static void spawn_bullet(float x, float y, float vx, float vy, int type) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x = x;
            bullets[i].y = y;
            bullets[i].vx = vx;
            bullets[i].vy = vy;
            bullets[i].type = type;
            bullets[i].active = true;
            bullets[i].grazed = false;
            bullets[i].angle = 0;
            bullets[i].rot_speed = 0.1f;
            return;
        }
    }
}

void battle_init(BattleEnemyType enemy_type) {
    cur_enemy_type = enemy_type;
    state = B_STATE_INTRO;
    state_timer = 0;
    battle_finished = false;
    kround_bow_count = 0;

    // Party stats
    party[0].name = "Kris";
    party[0].max_hp = 90;
    party[0].hp = 90;
    party[0].anim_state = 0;
    party[0].anim_timer = 0;

    party[1].name = "Susie";
    party[1].max_hp = 110;
    party[1].hp = 110;
    party[1].anim_state = 0;
    party[1].anim_timer = 0;

    party[2].name = "Ralsei";
    party[2].max_hp = 70;
    party[2].hp = 70;
    party[2].anim_state = 0;
    party[2].anim_timer = 0;

    active_party_idx = 0;
    cur_btn_idx = 0;
    sub_menu_idx = 0;
    tension_points = 20;

    // Enemy stats
    if (enemy_type == BATTLE_ENEMY_RUDINN) {
        enemy.name = "Rudinn";
        enemy.max_hp = 120;
        enemy.hp = 120;
        enemy.mercy = 0;
        enemy.taunt_text = "I'm just doing\nmy job, okay?";
        sound_play_bgm(&bgm_rude_buster);
    } else if (enemy_type == BATTLE_ENEMY_LANCER) {
        enemy.name = "Lancer";
        enemy.max_hp = 240;
        enemy.hp = 240;
        enemy.mercy = 0;
        enemy.taunt_text = "Ho ho ho!\nTaste my sweet\njustice!";
        sound_play_bgm(&bgm_rude_buster);
    } else if (enemy_type == BATTLE_ENEMY_KROUND) {
        enemy.name = "K. Round";
        enemy.max_hp = 380;
        enemy.hp = 380;
        enemy.mercy = 0;
        enemy.taunt_text = "* It adjusts its\n  crown proudly.";
        sound_play_bgm(&bgm_rude_buster);
    } else if (enemy_type == BATTLE_ENEMY_KING) {
        enemy.name = "Chaos King";
        enemy.max_hp = 600;
        enemy.hp = 600;
        enemy.mercy = 0;
        enemy.taunt_text = "To my people,\nI am a HERO!\nTo you, your END!";
        sound_play_bgm(&bgm_rude_buster);
    } else { // JEVIL
        enemy.name = "Jevil";
        enemy.max_hp = 700;
        enemy.hp = 700;
        enemy.mercy = 0;
        enemy.taunt_text = "CHAOS, CHAOS!\nLET IT RAIN!";
        sound_play_bgm(&bgm_jevil);
        sound_play_sfx(&snd_jevil_chaos);
    }

    enemy.is_hurt = false;
    clear_bullets();
}

bool battle_is_finished(void) {
    return battle_finished;
}

static void advance_party_turn(void) {
    active_party_idx++;
    cur_btn_idx = 0;
    sub_menu_idx = 0;
    if (active_party_idx >= 3) {
        // All party actions queued, start execution!
        state = B_STATE_ATTACK_METER;
        meter_pos_x = 240.0f;
    } else {
        state = B_STATE_MENU_BTN;
    }
}

void battle_update(u32 keys_down, u32 keys_held, touchPosition touch) {
    state_timer++;
    if (soul_invuln > 0) soul_invuln--;
    if (damage_display_timer > 0) damage_display_timer--;

    for (int i = 0; i < 3; i++) {
        party[i].anim_timer++;
    }

    switch (state) {
        case B_STATE_INTRO:
            if (state_timer > 30) {
                state = B_STATE_MENU_BTN;
                state_timer = 0;
            }
            break;

        case B_STATE_MENU_BTN:
            if (keys_down & KEY_TOUCH) {
                if (touch.py >= 125 && touch.py <= 165) {
                    int clicked_btn = (touch.px - 28) / 40;
                    if (clicked_btn >= 0 && clicked_btn < 5) {
                        cur_btn_idx = clicked_btn;
                        sound_play_sfx(&snd_menu_confirm);
                        keys_down |= KEY_A;
                    }
                }
            }

            if (keys_down & KEY_LEFT) {
                cur_btn_idx = (cur_btn_idx + 4) % 5;
                sound_play_sfx(&snd_menu_switch);
            }
            if (keys_down & KEY_RIGHT) {
                cur_btn_idx = (cur_btn_idx + 1) % 5;
                sound_play_sfx(&snd_menu_switch);
            }

            if (keys_down & KEY_A) {
                sound_play_sfx(&snd_menu_confirm);
                if (cur_btn_idx == 0) { // FIGHT
                    party[active_party_idx].act_selected = 1;
                    advance_party_turn();
                } else if (cur_btn_idx == 1) { // ACT
                    state = B_STATE_ACT_SELECT;
                    sub_menu_idx = 0;
                } else if (cur_btn_idx == 2) { // ITEM
                    state = B_STATE_ITEM_SELECT;
                    sub_menu_idx = 0;
                } else if (cur_btn_idx == 3) { // SPARE
                    party[active_party_idx].act_selected = 4;
                    if (enemy.mercy >= 100) {
                        state = B_STATE_VICTORY;
                        state_timer = 0;
                    } else {
                        advance_party_turn();
                    }
                } else if (cur_btn_idx == 4) { // DEFEND
                    party[active_party_idx].act_selected = 5;
                    party[active_party_idx].anim_state = 2; // Defend pose
                    tension_points += 16;
                    if (tension_points > 100) tension_points = 100;
                    advance_party_turn();
                }
            }
            break;

        case B_STATE_TARGET_SELECT:
            advance_party_turn();
            break;

        case B_STATE_ACT_SELECT:
            if (keys_down & KEY_UP || keys_down & KEY_DOWN) {
                sub_menu_idx = 1 - sub_menu_idx;
                sound_play_sfx(&snd_menu_switch);
            }
            if (keys_down & KEY_B) {
                state = B_STATE_MENU_BTN;
                sound_play_sfx(&snd_menu_switch);
            }
            if (keys_down & KEY_A) {
                sound_play_sfx(&snd_spell);
                party[active_party_idx].act_selected = 2;

                if (active_party_idx == 0) { // Kris
                    if (cur_enemy_type == BATTLE_ENEMY_KROUND) {
                        // Bowing loosens K.Round's crown!
                        kround_bow_count++;
                        enemy.mercy += 35;
                        if (enemy.mercy >= 100) enemy.mercy = 100;
                    } else if (cur_enemy_type == BATTLE_ENEMY_JEVIL) {
                        // Pirouette!
                        tension_points += 30;
                        if (tension_points > 100) tension_points = 100;
                        enemy.mercy += 20;
                        sound_play_sfx(&snd_jevil_anything);
                    } else if (cur_enemy_type == BATTLE_ENEMY_KING) {
                        // Courage
                        tension_points += 25;
                        if (tension_points > 100) tension_points = 100;
                    } else {
                        enemy.mercy += 50;
                    }
                } else if (active_party_idx == 1) { // Susie
                    if (tension_points >= 40) {
                        tension_points -= 40;
                        enemy.hp -= 95;
                    }
                } else if (active_party_idx == 2) { // Ralsei
                    if (tension_points >= 32) {
                        tension_points -= 32;
                        for (int p = 0; p < 3; p++) {
                            party[p].hp += 45;
                            if (party[p].hp > party[p].max_hp) party[p].hp = party[p].max_hp;
                        }
                    }
                    if (enemy.mercy >= 50) enemy.mercy = 100;
                }
                advance_party_turn();
            }
            break;

        case B_STATE_ITEM_SELECT:
            if (keys_down & KEY_B) {
                state = B_STATE_MENU_BTN;
                sound_play_sfx(&snd_menu_switch);
            }
            if (keys_down & KEY_A) {
                sound_play_sfx(&snd_bell);
                party[active_party_idx].act_selected = 3;
                party[active_party_idx].hp += 40;
                if (party[active_party_idx].hp > party[active_party_idx].max_hp) {
                    party[active_party_idx].hp = party[active_party_idx].max_hp;
                }
                advance_party_turn();
            }
            break;

        case B_STATE_ATTACK_METER:
            meter_pos_x -= meter_speed;
            if (keys_down & KEY_A) {
                float diff = meter_pos_x - 70.0f;
                if (diff < 0) diff = -diff;
                if (diff < 10.0f) {
                    attack_damage_dealt = 115 + (rand() % 35);
                    sound_play_sfx(&snd_attack);
                } else if (diff < 35.0f) {
                    attack_damage_dealt = 75 + (rand() % 20);
                    sound_play_sfx(&snd_attack);
                } else {
                    attack_damage_dealt = 40 + (rand() % 15);
                    sound_play_sfx(&snd_attack);
                }

                damage_display_val = attack_damage_dealt;
                damage_display_timer = 45;
                enemy.hp -= attack_damage_dealt;
                enemy.is_hurt = true;
                sound_play_sfx(&snd_damage);

                state = B_STATE_EXECUTE_ACTIONS;
                state_timer = 0;
            } else if (meter_pos_x < 50.0f) {
                attack_damage_dealt = 0;
                damage_display_val = 0;
                damage_display_timer = 30;
                state = B_STATE_EXECUTE_ACTIONS;
                state_timer = 0;
            }
            break;

        case B_STATE_EXECUTE_ACTIONS:
            party[0].anim_state = 1;
            if (party[1].act_selected == 1) party[1].anim_state = 1;
            if (party[2].act_selected == 2) party[2].anim_state = 3;

            if (state_timer > 50) {
                enemy.is_hurt = false;
                party[0].anim_state = 0;
                party[1].anim_state = 0;
                party[2].anim_state = 0;

                if (enemy.hp <= 0 || enemy.mercy >= 100) {
                    state = B_STATE_VICTORY;
                    state_timer = 0;
                } else {
                    state = B_STATE_ENEMY_TAUNT;
                    state_timer = 0;
                }
            }
            break;

        case B_STATE_ENEMY_TAUNT:
            if (state_timer > 60 || (keys_down & KEY_A)) {
                state = B_STATE_BULLET_HELL;
                state_timer = 0;
                wave_timer = 0;
                soul_x = ARENA_X + (ARENA_W / 2);
                soul_y = ARENA_Y + (ARENA_H / 2);
                soul_invuln = 20;
                clear_bullets();
            }
            break;

        case B_STATE_BULLET_HELL:
            wave_timer++;

            // Soul movement (8-directional)
            {
                float soul_spd = 2.0f;
                if (keys_held & KEY_UP)    soul_y -= soul_spd;
                if (keys_held & KEY_DOWN)  soul_y += soul_spd;
                if (keys_held & KEY_LEFT)  soul_x -= soul_spd;
                if (keys_held & KEY_RIGHT) soul_x += soul_spd;

                if (soul_x < ARENA_X + 9) soul_x = ARENA_X + 9;
                if (soul_x > ARENA_X + ARENA_W - 9) soul_x = ARENA_X + ARENA_W - 9;
                if (soul_y < ARENA_Y + 9) soul_y = ARENA_Y + 9;
                if (soul_y > ARENA_Y + ARENA_H - 9) soul_y = ARENA_Y + ARENA_H - 9;
            }

            // Bullet spawning patterns based on enemy
            if (cur_enemy_type == BATTLE_ENEMY_RUDINN) {
                if (wave_timer % 32 == 0 && wave_timer < 280) {
                    float bx = ARENA_X + (rand() % ARENA_W);
                    float by = ARENA_Y + 4;
                    float dx = soul_x - bx;
                    float dy = soul_y - by;
                    float dist = 1.0f;
                    if (dx*dx + dy*dy > 1.0f) dist = 2.2f / sqrtf(dx*dx + dy*dy);
                    spawn_bullet(bx, by, dx * dist, dy * dist, 1);
                }
            } else if (cur_enemy_type == BATTLE_ENEMY_LANCER) {
                if (wave_timer % 24 == 0 && wave_timer < 300) {
                    float side = (rand() % 2 == 0) ? ARENA_X + 4 : ARENA_X + ARENA_W - 4;
                    float vy = ((rand() % 100) - 50) / 40.0f;
                    float vx = (side < ARENA_X + 50) ? 1.8f : -1.8f;
                    spawn_bullet(side, ARENA_Y + 10 + (rand() % (ARENA_H - 20)), vx, vy, 0);
                }
            } else if (cur_enemy_type == BATTLE_ENEMY_KROUND) {
                // K. Round: Checker shrapnel and diamond stomps
                if (wave_timer % 28 == 0 && wave_timer < 310) {
                    float bx = ARENA_X + 10 + (rand() % (ARENA_W - 20));
                    spawn_bullet(bx, ARENA_Y + 4, 0.0f, 2.0f, 1);
                    spawn_bullet(bx, ARENA_Y + 4, 1.2f, 1.6f, 3);
                    spawn_bullet(bx, ARENA_Y + 4, -1.2f, 1.6f, 3);
                }
            } else if (cur_enemy_type == BATTLE_ENEMY_KING) {
                // Chaos King: Spade waves and chain strikes
                if (wave_timer % 18 == 0 && wave_timer < 320) {
                    float ang = wave_timer * 0.12f;
                    float vx = cosf(ang) * 2.1f;
                    float vy = sinf(ang) * 2.1f;
                    spawn_bullet(ARENA_X + ARENA_W / 2, ARENA_Y + 10, vx, vy, 0);
                }
            } else { // Jevil
                // Jevil: Devilsknife scythes & chaos bursts
                if (wave_timer % 20 == 0 && wave_timer < 320) {
                    float ang = (wave_timer * 0.15f);
                    float vx = cosf(ang) * 2.2f;
                    float vy = sinf(ang) * 2.2f;
                    int b_type = (wave_timer % 40 == 0) ? 2 : 1;
                    spawn_bullet(ARENA_X + ARENA_W / 2, ARENA_Y + 8, vx, vy, b_type);
                }
            }

            // Update bullets & collision check
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!bullets[i].active) continue;
                bullets[i].x += bullets[i].vx;
                bullets[i].y += bullets[i].vy;

                if (bullets[i].x < ARENA_X - 10 || bullets[i].x > ARENA_X + ARENA_W + 10 ||
                    bullets[i].y < ARENA_Y - 10 || bullets[i].y > ARENA_Y + ARENA_H + 10) {
                    bullets[i].active = false;
                    continue;
                }

                float dx = bullets[i].x - soul_x;
                float dy = bullets[i].y - soul_y;
                float dist_sq = dx * dx + dy * dy;

                if (dist_sq < 196.0f && !bullets[i].grazed) {
                    bullets[i].grazed = true;
                    tension_points += 2;
                    if (tension_points > 100) tension_points = 100;
                    graze_effect_timer = 10;
                    sound_play_sfx(&snd_graze);
                }

                if (dist_sq < 36.0f && soul_invuln == 0) {
                    soul_invuln = 40;
                    sound_play_sfx(&snd_hurt);
                    int target_p = (active_party_idx < 3) ? active_party_idx : 0;
                    party[target_p].hp -= 18;
                    party[target_p].anim_state = 4;
                    if (party[target_p].hp <= 0) party[target_p].hp = 0;

                    if (party[0].hp == 0 && party[1].hp == 0 && party[2].hp == 0) {
                        state = B_STATE_GAMEOVER;
                        state_timer = 0;
                    }
                }
            }

            if (wave_timer > 320) {
                clear_bullets();
                active_party_idx = 0;
                cur_btn_idx = 0;
                for (int p = 0; p < 3; p++) party[p].anim_state = 0;
                state = B_STATE_MENU_BTN;
            }
            break;

        case B_STATE_VICTORY:
            if (keys_down & KEY_A) {
                battle_finished = true;
            }
            break;

        case B_STATE_GAMEOVER:
            if (keys_down & KEY_A) {
                battle_init(cur_enemy_type);
            }
            break;
    }
}

void battle_render(void) {
    // ==========================================
    // TOP SCREEN: Battle Arena, Party & Enemy
    // ==========================================
    renderer_draw_gradient_v(SCREEN_TOP, 0, 0, SCREEN_W, 70, RGB15(1, 0, 4), RGB15(4, 1, 9));
    for (int ty = 70; ty < SCREEN_H; ty += 48) {
        for (int tx = 0; tx < SCREEN_W; tx += 48) {
            renderer_draw_sprite_sub(SCREEN_TOP, &bg_battle_tile, tx, ty, 0, 0, 48, 48);
        }
    }

    // Party Members on the left
    int kris_frame = (party[0].anim_timer / 15) % 2;
    int susie_frame = (party[1].anim_timer / 15) % 2;
    int ralsei_frame = (party[2].anim_timer / 15) % 2;

    if (party[0].anim_state == 1) {
        renderer_draw_sprite(SCREEN_TOP, &spr_kris_b_attack[1], 44, 58);
    } else if (party[0].anim_state == 2) {
        renderer_draw_sprite(SCREEN_TOP, &spr_kris_b_defend, 36, 60);
    } else if (party[0].anim_state == 4) {
        renderer_draw_sprite(SCREEN_TOP, &spr_kris_b_hurt, 32, 60);
    } else {
        renderer_draw_sprite(SCREEN_TOP, &spr_kris_b_idle[kris_frame], 36, 60);
    }

    if (party[1].anim_state == 1) {
        renderer_draw_sprite(SCREEN_TOP, &spr_susie_b_attack[1], 36, 96);
    } else if (party[1].anim_state == 2) {
        renderer_draw_sprite(SCREEN_TOP, &spr_susie_b_defend, 28, 100);
    } else if (party[1].anim_state == 4) {
        renderer_draw_sprite(SCREEN_TOP, &spr_susie_b_hurt, 24, 100);
    } else {
        renderer_draw_sprite(SCREEN_TOP, &spr_susie_b_idle[susie_frame], 28, 100);
    }

    if (party[2].anim_state == 3) {
        renderer_draw_sprite(SCREEN_TOP, &spr_ralsei_b_spell[1], 26, 138);
    } else if (party[2].anim_state == 2) {
        renderer_draw_sprite(SCREEN_TOP, &spr_ralsei_b_defend, 20, 140);
    } else if (party[2].anim_state == 4) {
        renderer_draw_sprite(SCREEN_TOP, &spr_ralsei_b_hurt, 16, 140);
    } else {
        renderer_draw_sprite(SCREEN_TOP, &spr_ralsei_b_idle[ralsei_frame], 20, 140);
    }

    if (state == B_STATE_MENU_BTN || state == B_STATE_ACT_SELECT || state == B_STATE_ITEM_SELECT) {
        int arr_y = 70 + active_party_idx * 40;
        renderer_draw_char(SCREEN_TOP, 14, arr_y, '>', COL_YELLOW);
    }

    // Render Boss / Enemy on the right
    int enemy_x = 175;
    int enemy_y = 65;

    if (cur_enemy_type == BATTLE_ENEMY_RUDINN) {
        int e_bob = (state_timer / 12) % 2;
        if (enemy.is_hurt) {
            renderer_draw_sprite_tint(SCREEN_TOP, &spr_rudinn_b_hurt, enemy_x, enemy_y, COL_WHITE);
        } else {
            renderer_draw_sprite(SCREEN_TOP, &spr_rudinn_b_idle[e_bob], enemy_x, enemy_y);
        }
    } else if (cur_enemy_type == BATTLE_ENEMY_LANCER) {
        int e_bob = (state_timer / 12) % 2;
        if (enemy.is_hurt) {
            renderer_draw_sprite_tint(SCREEN_TOP, &spr_lancer_b_hurt, enemy_x, enemy_y, COL_WHITE);
        } else {
            renderer_draw_sprite(SCREEN_TOP, &spr_lancer_b_idle[e_bob], enemy_x, enemy_y);
        }
    } else if (cur_enemy_type == BATTLE_ENEMY_KROUND) {
        int kr_frame = (state_timer / 10) % 4;
        if (enemy.is_hurt) {
            renderer_draw_sprite_tint(SCREEN_TOP, &spr_kround_b_bow, enemy_x, enemy_y - 10, COL_WHITE);
        } else {
            renderer_draw_sprite(SCREEN_TOP, &spr_kround_b_idle[kr_frame], enemy_x, enemy_y - 10);
        }
    } else if (cur_enemy_type == BATTLE_ENEMY_KING) {
        int king_frame = (state_timer / 10) % 5;
        if (enemy.is_hurt) {
            renderer_draw_sprite_tint(SCREEN_TOP, &spr_king_b_hurt, enemy_x - 10, enemy_y - 10, COL_WHITE);
        } else {
            renderer_draw_sprite(SCREEN_TOP, &spr_king_b_idle[king_frame], enemy_x - 10, enemy_y - 10);
        }
    } else { // Jevil
        int jv_frame = (state_timer / 8) % 4;
        if (enemy.is_hurt) {
            renderer_draw_sprite_tint(SCREEN_TOP, &spr_jevil_b_hurt, enemy_x, enemy_y, COL_WHITE);
        } else {
            renderer_draw_sprite(SCREEN_TOP, &spr_jevil_b_idle[jv_frame], enemy_x, enemy_y);
        }
    }

    // Enemy Name & HP / Mercy Bar on top
    renderer_draw_text(SCREEN_TOP, 155, 24, enemy.name, COL_WHITE);
    renderer_draw_rect(SCREEN_TOP, 155, 42, 80, 6, COL_DARK_GRAY);
    int hp_bar_w = (enemy.hp * 80) / enemy.max_hp;
    if (hp_bar_w < 0) hp_bar_w = 0;
    renderer_draw_rect(SCREEN_TOP, 155, 42, hp_bar_w, 6, COL_GREEN);

    if (enemy.mercy > 0) {
        renderer_draw_rect(SCREEN_TOP, 155, 50, (enemy.mercy * 80) / 100, 3, COL_YELLOW);
    }

    // Damage number pop-up
    if (damage_display_timer > 0) {
        char dmg_str[16];
        siprintf(dmg_str, "%d", damage_display_val);
        renderer_draw_text_scaled(SCREEN_TOP, enemy_x + 10, enemy_y - 20, dmg_str, COL_RED, 2);
    }

    // Enemy Speech Bubble during Taunt
    if (state == B_STATE_ENEMY_TAUNT && enemy.taunt_text) {
        renderer_draw_rect(SCREEN_TOP, 125, 6, 122, 48, COL_WHITE);
        renderer_draw_rect_outline(SCREEN_TOP, 125, 6, 122, 48, COL_BLACK, 2);
        renderer_draw_text(SCREEN_TOP, 130, 12, enemy.taunt_text, COL_BLACK);
    }

    // ==========================================
    // BOTTOM SCREEN: HUD, Command Menu & Arena
    // ==========================================
    renderer_clear(SCREEN_BOT, COL_BLACK);

    // Left Side: TP Gauge
    renderer_draw_rect(SCREEN_BOT, 4, 10, 18, 140, RGB15(2, 2, 4));
    renderer_draw_rect_outline(SCREEN_BOT, 4, 10, 18, 140, COL_WHITE, 1);
    renderer_draw_text(SCREEN_BOT, 7, 14, "T", COL_ORANGE);
    renderer_draw_text(SCREEN_BOT, 7, 30, "P", COL_ORANGE);

    int tp_h = (tension_points * 95) / 100;
    renderer_draw_rect(SCREEN_BOT, 6, 148 - tp_h, 14, tp_h, COL_ORANGE);

    char tp_str[8];
    siprintf(tp_str, "%d%%", tension_points);
    renderer_draw_text(SCREEN_BOT, 4, 154, tp_str, COL_WHITE);

    // Bottom Party Cards
    // Kris
    renderer_draw_rect(SCREEN_BOT, 28, 166, 68, 22, RGB15(1, 2, 4));
    renderer_draw_rect_outline(SCREEN_BOT, 28, 166, 68, 22, (active_party_idx == 0) ? COL_YELLOW : COL_CYAN, 1);
    char kris_hp[16];
    siprintf(kris_hp, "%d/%d", party[0].hp, party[0].max_hp);
    renderer_draw_text(SCREEN_BOT, 32, 169, "KRIS", COL_CYAN);
    renderer_draw_text(SCREEN_BOT, 60, 169, kris_hp, COL_WHITE);

    // Susie
    renderer_draw_rect(SCREEN_BOT, 100, 166, 68, 22, RGB15(3, 1, 3));
    renderer_draw_rect_outline(SCREEN_BOT, 100, 166, 68, 22, (active_party_idx == 1) ? COL_YELLOW : COL_MAGENTA, 1);
    char susie_hp[16];
    siprintf(susie_hp, "%d/%d", party[1].hp, party[1].max_hp);
    renderer_draw_text(SCREEN_BOT, 104, 169, "SUSIE", COL_MAGENTA);
    renderer_draw_text(SCREEN_BOT, 134, 169, susie_hp, COL_WHITE);

    // Ralsei
    renderer_draw_rect(SCREEN_BOT, 172, 166, 68, 22, RGB15(1, 3, 1));
    renderer_draw_rect_outline(SCREEN_BOT, 172, 166, 68, 22, (active_party_idx == 2) ? COL_YELLOW : COL_GREEN, 1);
    char ralsei_hp[16];
    siprintf(ralsei_hp, "%d/%d", party[2].hp, party[2].max_hp);
    renderer_draw_text(SCREEN_BOT, 176, 169, "RALSEI", COL_GREEN);
    renderer_draw_text(SCREEN_BOT, 208, 169, ralsei_hp, COL_WHITE);

    // Command Area or Bullet Arena
    if (state == B_STATE_MENU_BTN || state == B_STATE_INTRO) {
        renderer_draw_rect(SCREEN_BOT, 28, 12, 220, 110, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, 28, 12, 220, 110, COL_WHITE, 2);

        char prompt_str[64];
        siprintf(prompt_str, "* What will %s do?", party[active_party_idx].name);
        renderer_draw_text(SCREEN_BOT, 42, 28, prompt_str, COL_WHITE);

        for (int b = 0; b < 5; b++) {
            int bx = 28 + b * 44;
            int by = 130;
            int btn_state = (b == cur_btn_idx) ? 1 : 0;
            renderer_draw_sprite(SCREEN_BOT, &spr_btn[b][btn_state], bx, by);
        }

    } else if (state == B_STATE_ACT_SELECT) {
        renderer_draw_rect(SCREEN_BOT, 28, 12, 220, 145, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, 28, 12, 220, 145, COL_WHITE, 2);

        renderer_draw_text(SCREEN_BOT, 42, 24, "* SELECT AN ACTION:", COL_YELLOW);
        if (active_party_idx == 0) { // Kris
            if (cur_enemy_type == BATTLE_ENEMY_KROUND) {
                renderer_draw_text(SCREEN_BOT, 54, 52, "* Deep Bow (Loosens the Crown)", (sub_menu_idx == 0) ? COL_YELLOW : COL_WHITE);
                renderer_draw_text(SCREEN_BOT, 54, 76, "* Flatter (Increases Mercy)", (sub_menu_idx == 1) ? COL_YELLOW : COL_WHITE);
            } else if (cur_enemy_type == BATTLE_ENEMY_KING) {
                renderer_draw_text(SCREEN_BOT, 54, 52, "* Courage (Party Defense + TP)", (sub_menu_idx == 0) ? COL_YELLOW : COL_WHITE);
                renderer_draw_text(SCREEN_BOT, 54, 76, "* Dual Heal (Heals Party)", (sub_menu_idx == 1) ? COL_YELLOW : COL_WHITE);
            } else if (cur_enemy_type == BATTLE_ENEMY_JEVIL) {
                renderer_draw_text(SCREEN_BOT, 54, 52, "* Pirouette (Random Chaos Effect)", (sub_menu_idx == 0) ? COL_YELLOW : COL_WHITE);
                renderer_draw_text(SCREEN_BOT, 54, 76, "* Hypnosis (Tires Jevil out)", (sub_menu_idx == 1) ? COL_YELLOW : COL_WHITE);
            } else {
                renderer_draw_text(SCREEN_BOT, 54, 52, "* Check (Inspect enemy stats)", (sub_menu_idx == 0) ? COL_YELLOW : COL_WHITE);
                renderer_draw_text(SCREEN_BOT, 54, 76, "* Compliment (+50% Mercy)", (sub_menu_idx == 1) ? COL_YELLOW : COL_WHITE);
            }
        } else if (active_party_idx == 1) { // Susie
            renderer_draw_text(SCREEN_BOT, 54, 52, "* Rude Buster (40% TP - Heavy Damage)", (sub_menu_idx == 0) ? COL_YELLOW : COL_WHITE);
            renderer_draw_text(SCREEN_BOT, 54, 76, "* Warn (Enemy dodges attacks)", (sub_menu_idx == 1) ? COL_YELLOW : COL_WHITE);
        } else { // Ralsei
            renderer_draw_text(SCREEN_BOT, 54, 52, "* Heal Prayer (32% TP - +45 HP)", (sub_menu_idx == 0) ? COL_YELLOW : COL_WHITE);
            renderer_draw_text(SCREEN_BOT, 54, 76, "* Pacify (16% TP - Spares tired foe)", (sub_menu_idx == 1) ? COL_YELLOW : COL_WHITE);
        }
        renderer_draw_char(SCREEN_BOT, 42, 52 + sub_menu_idx * 24, '*', COL_YELLOW);

    } else if (state == B_STATE_ITEM_SELECT) {
        renderer_draw_rect(SCREEN_BOT, 28, 12, 220, 145, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, 28, 12, 220, 145, COL_WHITE, 2);

        renderer_draw_text(SCREEN_BOT, 42, 24, "* SELECT AN ITEM:", COL_YELLOW);
        renderer_draw_text(SCREEN_BOT, 54, 52, "* Dark Candy (+40 HP)", COL_YELLOW);
        renderer_draw_text(SCREEN_BOT, 54, 76, "* Choco Diamond (+80 HP)", COL_GRAY);
        renderer_draw_char(SCREEN_BOT, 42, 52, '*', COL_YELLOW);

    } else if (state == B_STATE_ATTACK_METER) {
        renderer_draw_rect(SCREEN_BOT, 28, 12, 220, 145, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, 28, 12, 220, 145, COL_WHITE, 2);

        renderer_draw_text(SCREEN_BOT, 42, 24, "* PRESS [A] TO STRIKE!", COL_YELLOW);

        renderer_draw_rect(SCREEN_BOT, 40, 75, 195, 20, RGB15(2, 2, 6));
        renderer_draw_rect_outline(SCREEN_BOT, 40, 75, 195, 20, COL_WHITE, 1);
        renderer_draw_rect(SCREEN_BOT, 70, 72, 3, 26, COL_CYAN);
        renderer_draw_rect(SCREEN_BOT, (int)meter_pos_x, 70, 6, 30, COL_WHITE);

    } else if (state == B_STATE_BULLET_HELL) {
        renderer_draw_rect(SCREEN_BOT, ARENA_X, ARENA_Y, ARENA_W, ARENA_H, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, ARENA_X, ARENA_Y, ARENA_W, ARENA_H, COL_WHITE, 3);

        if (soul_invuln == 0 || (soul_invuln / 4) % 2 == 0) {
            renderer_draw_sprite(SCREEN_BOT, &spr_soul, (int)soul_x - 8, (int)soul_y - 8);
        }

        if (graze_effect_timer > 0) {
            graze_effect_timer--;
            renderer_draw_sprite(SCREEN_BOT, &spr_soul_graze, (int)soul_x - 12, (int)soul_y - 12);
        }

        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!bullets[i].active) continue;
            int bx = (int)bullets[i].x;
            int by = (int)bullets[i].y;
            if (bullets[i].type == 0) {
                renderer_draw_sprite(SCREEN_BOT, &spr_bullet_spade, bx - 9, by - 8);
            } else if (bullets[i].type == 1) {
                renderer_draw_sprite(SCREEN_BOT, &spr_bullet_diamond, bx - 6, by - 6);
            } else if (bullets[i].type == 2) {
                renderer_draw_sprite(SCREEN_BOT, &spr_bullet_scythe, bx - 14, by - 14);
            } else {
                renderer_draw_sprite(SCREEN_BOT, &spr_bullet_pellet, bx - 4, by - 4);
            }
        }

    } else if (state == B_STATE_VICTORY) {
        renderer_draw_rect(SCREEN_BOT, 28, 12, 220, 145, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, 28, 12, 220, 145, COL_WHITE, 2);

        renderer_draw_text(SCREEN_BOT, 42, 32, "* YOU WON!", COL_YELLOW);
        renderer_draw_text(SCREEN_BOT, 42, 60, "* Got 0 EXP and 180 D$.", COL_WHITE);
        renderer_draw_text(SCREEN_BOT, 42, 90, "* Press [A] to continue.", COL_GRAY);

    } else if (state == B_STATE_GAMEOVER) {
        renderer_draw_rect(SCREEN_BOT, 28, 12, 220, 145, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, 28, 12, 220, 145, COL_RED, 2);

        renderer_draw_text(SCREEN_BOT, 42, 32, "* THE WORLD HAS BEEN", COL_RED);
        renderer_draw_text(SCREEN_BOT, 42, 52, "  SHROUDED IN DARKNESS...", COL_RED);
        renderer_draw_text(SCREEN_BOT, 42, 85, "* DON'T GIVE UP!", COL_WHITE);
        renderer_draw_text(SCREEN_BOT, 42, 110, "* Press [A] to try again.", COL_YELLOW);
    }
}
