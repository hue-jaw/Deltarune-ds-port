#include "sans_fight.h"
#include "renderer.h"
#include "sound_manager.h"
#include "assets_gfx.h"
#include "assets_undertale.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define SANS_MAX_BONES 32
#define SANS_MAX_BLASTERS 4

typedef enum {
    SANS_STATE_INTRO,
    SANS_STATE_MENU,
    SANS_STATE_ACT_MENU,
    SANS_STATE_ITEM_MENU,
    SANS_STATE_MERCY_MENU,
    SANS_STATE_ATTACK_METER,
    SANS_STATE_SANS_DODGE,
    SANS_STATE_DIALOGUE,
    SANS_STATE_BULLET_HELL,
    SANS_STATE_GAMEOVER,
    SANS_STATE_VICTORY
} SansState;

typedef struct {
    float x, y;
    float vx, vy;
    int h; // height
    bool active;
} Bone;

typedef struct {
    float x, y;
    int timer; // 0..60
    int dir;   // 0=down, 1=left, 2=right
    bool active;
    bool fired;
} Blaster;

#define UT_ARENA_X 48
#define UT_ARENA_Y 40
#define UT_ARENA_W 160
#define UT_ARENA_H 90

static SansState state = SANS_STATE_INTRO;
static int state_timer = 0;
static int turn_count = 0;
static bool finished = false;

// Player stats
static int player_hp = 92;
static int player_max_hp = 92;
static int player_kr = 0;
static int cur_btn = 0; // 0=FIGHT, 1=ACT, 2=ITEM, 3=MERCY
static int act_sel = 0;

// Soul physics (Red vs Blue mode)
static bool blue_soul = false;
static float soul_x = UT_ARENA_X + 80;
static float soul_y = UT_ARENA_Y + 70;
static float soul_vy = 0.0f;
static bool on_ground = true;
static int soul_invuln = 0;

// Bones & Blasters
static Bone bones[SANS_MAX_BONES];
static Blaster blasters[SANS_MAX_BLASTERS];
static int attack_wave_timer = 0;
static float sans_offset_x = 0.0f;
static bool sans_dodged = false;
static float meter_x = 220.0f;

static const char *sans_dialogues[] = {
    "it's a beautiful day outside.\nbirds are singing,\nflowers are blooming...\non days like these, kids like you...\nSHOULD BE BURNING IN HELL.",
    "what? you think i'm just\ngonna stand there and take it?",
    "always wondered why people\nnever use their strongest\nattack first.",
    "geez, you really like\nswinging that thing, huh?",
    "you're gonna have a\nBAD TIME."
};

static void clear_attacks(void) {
    for (int i = 0; i < SANS_MAX_BONES; i++) bones[i].active = false;
    for (int i = 0; i < SANS_MAX_BLASTERS; i++) blasters[i].active = false;
}

static void spawn_bone(float x, float y, float vx, int h) {
    for (int i = 0; i < SANS_MAX_BONES; i++) {
        if (!bones[i].active) {
            bones[i].x = x;
            bones[i].y = y;
            bones[i].vx = vx;
            bones[i].vy = 0;
            bones[i].h = h;
            bones[i].active = true;
            return;
        }
    }
}

static void spawn_blaster(float x, float y, int dir) {
    for (int i = 0; i < SANS_MAX_BLASTERS; i++) {
        if (!blasters[i].active) {
            blasters[i].x = x;
            blasters[i].y = y;
            blasters[i].dir = dir;
            blasters[i].timer = 0;
            blasters[i].active = true;
            blasters[i].fired = false;
            return;
        }
    }
}

void sans_fight_init(void) {
    state = SANS_STATE_INTRO;
    state_timer = 0;
    turn_count = 0;
    finished = false;
    player_hp = 92;
    player_max_hp = 92;
    player_kr = 0;
    cur_btn = 0;
    blue_soul = false;
    sans_offset_x = 0;
    sans_dodged = false;

    clear_attacks();
    sound_play_bgm(&bgm_jevil); // Fast-paced battle theme
}

bool sans_fight_is_finished(void) {
    return finished;
}

void sans_fight_update(u32 keys_down, u32 keys_held, touchPosition touch) {
    state_timer++;
    if (soul_invuln > 0) soul_invuln--;

    // Karma poison drain
    if (player_kr > 0 && (state_timer % 15 == 0)) {
        player_kr--;
        if (player_hp > 1) player_hp--;
    }

    switch (state) {
        case SANS_STATE_INTRO:
            if (state_timer > 45 || (keys_down & KEY_A)) {
                state = SANS_STATE_DIALOGUE;
                state_timer = 0;
            }
            break;

        case SANS_STATE_DIALOGUE:
            if (state_timer > 80 || (keys_down & KEY_A)) {
                state = SANS_STATE_MENU;
                state_timer = 0;
            }
            break;

        case SANS_STATE_MENU:
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
                    state = SANS_STATE_ATTACK_METER;
                    meter_x = 220.0f;
                } else if (cur_btn == 1) { // ACT
                    state = SANS_STATE_ACT_MENU;
                    act_sel = 0;
                } else if (cur_btn == 2) { // ITEM
                    state = SANS_STATE_ITEM_MENU;
                } else if (cur_btn == 3) { // MERCY
                    state = SANS_STATE_MERCY_MENU;
                }
            }
            break;

        case SANS_STATE_ACT_MENU:
            if (keys_down & (KEY_UP | KEY_DOWN)) {
                act_sel = 1 - act_sel;
                sound_play_sfx(&snd_menu_switch);
            }
            if (keys_down & KEY_B) {
                state = SANS_STATE_MENU;
                sound_play_sfx(&snd_menu_switch);
            }
            if (keys_down & KEY_A) {
                sound_play_sfx(&snd_text_default);
                state = SANS_STATE_BULLET_HELL;
                state_timer = 0;
                attack_wave_timer = 0;
                clear_attacks();
                blue_soul = true;
                soul_x = UT_ARENA_X + 80;
                soul_y = UT_ARENA_Y + UT_ARENA_H - 14;
            }
            break;

        case SANS_STATE_ITEM_MENU:
            if (keys_down & KEY_B) {
                state = SANS_STATE_MENU;
                sound_play_sfx(&snd_menu_switch);
            }
            if (keys_down & KEY_A) {
                sound_play_sfx(&snd_bell);
                player_hp += 45;
                if (player_hp > player_max_hp) player_hp = player_max_hp;
                player_kr = 0;
                state = SANS_STATE_BULLET_HELL;
                state_timer = 0;
                attack_wave_timer = 0;
                clear_attacks();
                blue_soul = true;
                soul_x = UT_ARENA_X + 80;
                soul_y = UT_ARENA_Y + UT_ARENA_H - 14;
            }
            break;

        case SANS_STATE_MERCY_MENU:
            if (keys_down & KEY_B) {
                state = SANS_STATE_MENU;
                sound_play_sfx(&snd_menu_switch);
            }
            if (keys_down & KEY_A) {
                sound_play_sfx(&snd_text_default);
                state = SANS_STATE_BULLET_HELL;
                state_timer = 0;
                attack_wave_timer = 0;
                clear_attacks();
                blue_soul = true;
                soul_x = UT_ARENA_X + 80;
                soul_y = UT_ARENA_Y + UT_ARENA_H - 14;
            }
            break;

        case SANS_STATE_ATTACK_METER:
            meter_x -= 5.0f;
            if (keys_down & KEY_A) {
                sound_play_sfx(&snd_attack);
                sans_dodged = true;
                sans_offset_x = 35.0f;
                state = SANS_STATE_SANS_DODGE;
                state_timer = 0;
            } else if (meter_x < 40.0f) {
                sans_dodged = true;
                state = SANS_STATE_SANS_DODGE;
                state_timer = 0;
            }
            break;

        case SANS_STATE_SANS_DODGE:
            if (state_timer > 45) {
                sans_dodged = false;
                sans_offset_x = 0.0f;
                state = SANS_STATE_BULLET_HELL;
                state_timer = 0;
                attack_wave_timer = 0;
                turn_count++;
                clear_attacks();

                // Alternate between Blue Gravity Soul and Gaster Blasters
                blue_soul = (turn_count % 2 == 1);
                soul_x = UT_ARENA_X + 80;
                soul_y = UT_ARENA_Y + UT_ARENA_H - 14;
                soul_vy = 0.0f;
            }
            break;

        case SANS_STATE_BULLET_HELL:
            attack_wave_timer++;

            // Soul movement & Gravity physics
            if (blue_soul) {
                // Gravity pulling down
                soul_vy += 0.28f;
                soul_y += soul_vy;

                float floor_y = UT_ARENA_Y + UT_ARENA_H - 12;
                if (soul_y >= floor_y) {
                    soul_y = floor_y;
                    soul_vy = 0.0f;
                    on_ground = true;
                } else {
                    on_ground = false;
                }

                // JUMP!
                if ((keys_down & (KEY_UP | KEY_A)) && on_ground) {
                    soul_vy = -4.6f;
                    on_ground = false;
                    sound_play_sfx(&snd_menu_switch);
                }

                // Horizontal movement
                if (keys_held & KEY_LEFT) soul_x -= 2.2f;
                if (keys_held & KEY_RIGHT) soul_x += 2.2f;
            } else {
                // Red Soul: 8-way free movement
                float spd = 2.0f;
                if (keys_held & KEY_UP) soul_y -= spd;
                if (keys_held & KEY_DOWN) soul_y += spd;
                if (keys_held & KEY_LEFT) soul_x -= spd;
                if (keys_held & KEY_RIGHT) soul_x += spd;
            }

            // Clamp soul in arena
            if (soul_x < UT_ARENA_X + 8) soul_x = UT_ARENA_X + 8;
            if (soul_x > UT_ARENA_X + UT_ARENA_W - 8) soul_x = UT_ARENA_X + UT_ARENA_W - 8;
            if (soul_y < UT_ARENA_Y + 8) soul_y = UT_ARENA_Y + 8;
            if (soul_y > UT_ARENA_Y + UT_ARENA_H - 8) soul_y = UT_ARENA_Y + UT_ARENA_H - 8;

            // Attack waves
            if (blue_soul) {
                // Bone sliding wave: jump over moving bones!
                if (attack_wave_timer % 38 == 0 && attack_wave_timer < 280) {
                    int bh = 14 + (rand() % 16);
                    spawn_bone(UT_ARENA_X + UT_ARENA_W + 10, UT_ARENA_Y + UT_ARENA_H - bh, -2.4f, bh);
                }
            } else {
                // Gaster Blasters wave!
                if (attack_wave_timer == 30) {
                    spawn_blaster(UT_ARENA_X + 30, UT_ARENA_Y + 12, 0);
                } else if (attack_wave_timer == 90) {
                    spawn_blaster(UT_ARENA_X + 110, UT_ARENA_Y + 12, 0);
                } else if (attack_wave_timer == 160) {
                    spawn_blaster(UT_ARENA_X + 14, UT_ARENA_Y + 45, 2);
                } else if (attack_wave_timer == 220) {
                    spawn_blaster(UT_ARENA_X + UT_ARENA_W - 14, UT_ARENA_Y + 65, 1);
                }
            }

            // Update bones
            for (int i = 0; i < SANS_MAX_BONES; i++) {
                if (!bones[i].active) continue;
                bones[i].x += bones[i].vx;

                if (bones[i].x < UT_ARENA_X - 10) {
                    bones[i].active = false;
                    continue;
                }

                // Collision check with bone
                if (soul_x >= bones[i].x - 6 && soul_x <= bones[i].x + 6 &&
                    soul_y >= bones[i].y && soul_y <= bones[i].y + bones[i].h) {
                    if (soul_invuln == 0) {
                        soul_invuln = 12;
                        player_hp -= 4;
                        player_kr += 6;
                        sound_play_sfx(&snd_hurt);
                        if (player_hp <= 0) {
                            player_hp = 0;
                            state = SANS_STATE_GAMEOVER;
                            state_timer = 0;
                        }
                    }
                }
            }

            // Update blasters
            for (int i = 0; i < SANS_MAX_BLASTERS; i++) {
                if (!blasters[i].active) continue;
                blasters[i].timer++;

                if (blasters[i].timer == 28 && !blasters[i].fired) {
                    blasters[i].fired = true;
                    sound_play_sfx(&snd_damage); // Blast laser sound!
                }

                // Laser beam hit check when firing
                if (blasters[i].fired && blasters[i].timer < 44) {
                    bool hit = false;
                    if (blasters[i].dir == 0) { // Vertical beam downwards
                        if (fabsf(soul_x - blasters[i].x) < 14.0f) hit = true;
                    } else if (blasters[i].dir == 2) { // Horizontal beam right
                        if (fabsf(soul_y - blasters[i].y) < 14.0f) hit = true;
                    } else if (blasters[i].dir == 1) { // Horizontal beam left
                        if (fabsf(soul_y - blasters[i].y) < 14.0f) hit = true;
                    }

                    if (hit && soul_invuln == 0) {
                        soul_invuln = 10;
                        player_hp -= 6;
                        player_kr += 8;
                        sound_play_sfx(&snd_hurt);
                        if (player_hp <= 0) {
                            player_hp = 0;
                            state = SANS_STATE_GAMEOVER;
                            state_timer = 0;
                        }
                    }
                }

                if (blasters[i].timer > 50) {
                    blasters[i].active = false;
                }
            }

            if (attack_wave_timer > 310) {
                clear_attacks();
                state = SANS_STATE_MENU;
                state_timer = 0;
            }
            break;

        case SANS_STATE_GAMEOVER:
            if (keys_down & KEY_A) {
                sans_fight_init();
            }
            break;

        case SANS_STATE_VICTORY:
            if (keys_down & KEY_A) {
                finished = true;
            }
            break;
    }
}

void sans_fight_render(void) {
    // ==========================================
    // TOP SCREEN: Sans Encounter & Speech
    // ==========================================
    renderer_clear(SCREEN_TOP, COL_BLACK);

    // Sans sprite
    int sx = 112 + (int)sans_offset_x;
    int sy = 60;

    if (sans_dodged) {
        renderer_draw_sprite(SCREEN_TOP, &spr_sans_dodge, sx, sy);
        renderer_draw_text_scaled(SCREEN_TOP, 116, sy - 20, "MISS", COL_LIGHT_GRAY, 1);
    } else {
        bool eye_glow = (state == SANS_STATE_BULLET_HELL && (state_timer / 4) % 2 == 0);
        if (eye_glow) {
            renderer_draw_sprite(SCREEN_TOP, &spr_sans_eye, sx, sy);
        } else {
            renderer_draw_sprite(SCREEN_TOP, &spr_sans_idle, sx, sy);
        }
    }

    // Speech bubble
    if (state == SANS_STATE_DIALOGUE || state == SANS_STATE_INTRO) {
        int d_idx = turn_count % 5;
        renderer_draw_rect(SCREEN_TOP, 140, 20, 110, 48, COL_WHITE);
        renderer_draw_rect_outline(SCREEN_TOP, 140, 20, 110, 48, COL_BLACK, 2);
        renderer_draw_text(SCREEN_TOP, 144, 24, sans_dialogues[d_idx], COL_BLACK);
    }

    // ==========================================
    // BOTTOM SCREEN: Arena, HUD & Controls
    // ==========================================
    renderer_clear(SCREEN_BOT, COL_BLACK);

    // Player HUD
    renderer_draw_text(SCREEN_BOT, 14, 18, "KRIS   LV 19", COL_WHITE);
    renderer_draw_text(SCREEN_BOT, 120, 18, "HP", COL_YELLOW);

    renderer_draw_rect(SCREEN_BOT, 142, 18, 50, 10, COL_RED);
    int hp_bar = (player_hp * 50) / player_max_hp;
    if (hp_bar < 0) hp_bar = 0;
    renderer_draw_rect(SCREEN_BOT, 142, 18, hp_bar, 10, COL_YELLOW);

    char hp_str[24];
    siprintf(hp_str, "%d / %d", player_hp, player_max_hp);
    renderer_draw_text(SCREEN_BOT, 198, 18, hp_str, COL_WHITE);

    if (player_kr > 0) {
        renderer_draw_text(SCREEN_BOT, 198, 28, "KR", COL_MAGENTA);
    }

    // Arena Box or Menu Box
    if (state == SANS_STATE_BULLET_HELL) {
        renderer_draw_rect(SCREEN_BOT, UT_ARENA_X, UT_ARENA_Y, UT_ARENA_W, UT_ARENA_H, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, UT_ARENA_X, UT_ARENA_Y, UT_ARENA_W, UT_ARENA_H, COL_WHITE, 3);

        // Render Bones
        for (int i = 0; i < SANS_MAX_BONES; i++) {
            if (!bones[i].active) continue;
            renderer_draw_rect(SCREEN_BOT, (int)bones[i].x - 3, (int)bones[i].y, 6, bones[i].h, COL_WHITE);
        }

        // Render Blasters & Laser Beams
        for (int i = 0; i < SANS_MAX_BLASTERS; i++) {
            if (!blasters[i].active) continue;
            int bx = (int)blasters[i].x;
            int by = (int)blasters[i].y;

            if (blasters[i].fired) {
                renderer_draw_sprite(SCREEN_BOT, &spr_gaster_blaster_fire, bx - 14, by - 14);

                // Giant laser beam
                if (blasters[i].dir == 0) { // Vertical downwards
                    renderer_draw_rect(SCREEN_BOT, bx - 10, by, 20, UT_ARENA_H, COL_WHITE);
                    renderer_draw_rect(SCREEN_BOT, bx - 6, by, 12, UT_ARENA_H, COL_CYAN);
                } else if (blasters[i].dir == 2) { // Horizontal right
                    renderer_draw_rect(SCREEN_BOT, bx, by - 10, UT_ARENA_W, 20, COL_WHITE);
                    renderer_draw_rect(SCREEN_BOT, bx, by - 6, UT_ARENA_W, 12, COL_CYAN);
                } else if (blasters[i].dir == 1) { // Horizontal left
                    renderer_draw_rect(SCREEN_BOT, UT_ARENA_X, by - 10, bx - UT_ARENA_X, 20, COL_WHITE);
                    renderer_draw_rect(SCREEN_BOT, UT_ARENA_X, by - 6, bx - UT_ARENA_X, 12, COL_CYAN);
                }
            } else {
                renderer_draw_sprite(SCREEN_BOT, &spr_gaster_blaster, bx - 14, by - 14);
            }
        }

        // Render Soul (Blue or Red)
        if (soul_invuln == 0 || (soul_invuln / 3) % 2 == 0) {
            if (blue_soul) {
                renderer_draw_sprite(SCREEN_BOT, &spr_soul_blue, (int)soul_x - 8, (int)soul_y - 8);
            } else {
                renderer_draw_sprite(SCREEN_BOT, &spr_soul, (int)soul_x - 8, (int)soul_y - 8);
            }
        }

    } else if (state == SANS_STATE_ATTACK_METER) {
        renderer_draw_rect(SCREEN_BOT, UT_ARENA_X, UT_ARENA_Y, UT_ARENA_W, UT_ARENA_H, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, UT_ARENA_X, UT_ARENA_Y, UT_ARENA_W, UT_ARENA_H, COL_WHITE, 2);

        renderer_draw_text(SCREEN_BOT, UT_ARENA_X + 16, UT_ARENA_Y + 12, "* PRESS [A] TO STRIKE!", COL_YELLOW);
        renderer_draw_rect(SCREEN_BOT, UT_ARENA_X + 20, UT_ARENA_Y + 45, 120, 16, RGB15(1, 1, 4));
        renderer_draw_rect_outline(SCREEN_BOT, UT_ARENA_X + 20, UT_ARENA_Y + 45, 120, 16, COL_WHITE, 1);
        renderer_draw_rect(SCREEN_BOT, (int)meter_x, UT_ARENA_Y + 42, 4, 22, COL_WHITE);

    } else if (state == SANS_STATE_ACT_MENU) {
        renderer_draw_rect(SCREEN_BOT, UT_ARENA_X, UT_ARENA_Y, UT_ARENA_W, UT_ARENA_H, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, UT_ARENA_X, UT_ARENA_Y, UT_ARENA_W, UT_ARENA_H, COL_WHITE, 2);

        renderer_draw_text(SCREEN_BOT, UT_ARENA_X + 20, UT_ARENA_Y + 20, "* Check", (act_sel == 0) ? COL_YELLOW : COL_WHITE);
        renderer_draw_text(SCREEN_BOT, UT_ARENA_X + 20, UT_ARENA_Y + 45, "* Insult", (act_sel == 1) ? COL_YELLOW : COL_WHITE);
        renderer_draw_char(SCREEN_BOT, UT_ARENA_X + 8, UT_ARENA_Y + 20 + act_sel * 25, '*', COL_YELLOW);

    } else if (state == SANS_STATE_ITEM_MENU) {
        renderer_draw_rect(SCREEN_BOT, UT_ARENA_X, UT_ARENA_Y, UT_ARENA_W, UT_ARENA_H, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, UT_ARENA_X, UT_ARENA_Y, UT_ARENA_W, UT_ARENA_H, COL_WHITE, 2);

        renderer_draw_text(SCREEN_BOT, UT_ARENA_X + 20, UT_ARENA_Y + 20, "* Butterscotch Pie (Full HP)", COL_YELLOW);
        renderer_draw_text(SCREEN_BOT, UT_ARENA_X + 20, UT_ARENA_Y + 45, "* Legendary Hero (+40 HP)", COL_WHITE);
        renderer_draw_char(SCREEN_BOT, UT_ARENA_X + 8, UT_ARENA_Y + 20, '*', COL_YELLOW);

    } else if (state == SANS_STATE_MERCY_MENU) {
        renderer_draw_rect(SCREEN_BOT, UT_ARENA_X, UT_ARENA_Y, UT_ARENA_W, UT_ARENA_H, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, UT_ARENA_X, UT_ARENA_Y, UT_ARENA_W, UT_ARENA_H, COL_WHITE, 2);

        renderer_draw_text(SCREEN_BOT, UT_ARENA_X + 20, UT_ARENA_Y + 30, "* Spare (sans is sparing you)", COL_YELLOW);
        renderer_draw_char(SCREEN_BOT, UT_ARENA_X + 8, UT_ARENA_Y + 30, '*', COL_YELLOW);

    } else {
        renderer_draw_rect(SCREEN_BOT, UT_ARENA_X, UT_ARENA_Y, UT_ARENA_W, UT_ARENA_H, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, UT_ARENA_X, UT_ARENA_Y, UT_ARENA_W, UT_ARENA_H, COL_WHITE, 2);

        renderer_draw_text(SCREEN_BOT, UT_ARENA_X + 14, UT_ARENA_Y + 16, "* The shadows gather.", COL_WHITE);
        renderer_draw_text(SCREEN_BOT, UT_ARENA_X + 14, UT_ARENA_Y + 36, "* You feel like you're gonna", COL_WHITE);
        renderer_draw_text(SCREEN_BOT, UT_ARENA_X + 14, UT_ARENA_Y + 52, "  have a bad time.", COL_WHITE);
    }

    // 4 Bottom Command Buttons: FIGHT, ACT, ITEM, MERCY
    const char *btn_labels[4] = { "FIGHT", "ACT", "ITEM", "MERCY" };
    for (int b = 0; b < 4; b++) {
        int bx = 16 + b * 58;
        int by = 160;
        bool is_sel = (b == cur_btn && state == SANS_STATE_MENU);

        renderer_draw_rect(SCREEN_BOT, bx, by, 52, 22, is_sel ? RGB15(31, 16, 0) : RGB15(8, 4, 0));
        renderer_draw_rect_outline(SCREEN_BOT, bx, by, 52, 22, is_sel ? COL_YELLOW : COL_ORANGE, is_sel ? 2 : 1);
        renderer_draw_text(SCREEN_BOT, bx + 10, by + 6, btn_labels[b], is_sel ? COL_YELLOW : COL_ORANGE);
    }

    if (state == SANS_STATE_GAMEOVER) {
        renderer_draw_rect(SCREEN_BOT, 20, 20, 216, 150, COL_BLACK);
        renderer_draw_rect_outline(SCREEN_BOT, 20, 20, 216, 150, COL_RED, 2);

        renderer_draw_text_scaled(SCREEN_BOT, 60, 35, "GAME OVER", COL_RED, 2);
        renderer_draw_text(SCREEN_BOT, 40, 80, "* geez kid, you really pushed", COL_WHITE);
        renderer_draw_text(SCREEN_BOT, 40, 96, "  your luck there.", COL_WHITE);
        renderer_draw_text(SCREEN_BOT, 40, 130, "* Press [A] to try again.", COL_YELLOW);
    }
}
