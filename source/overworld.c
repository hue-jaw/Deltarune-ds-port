#include "overworld.h"
#include "renderer.h"
#include "sound_manager.h"
#include "dialogue.h"
#include <string.h>

#define TRAIL_LEN 48

typedef struct {
    int x, y;
    int dir;
    bool moving;
} Step;

static RoomId current_room = ROOM_LIGHT_SCHOOL;
static int kris_x = 40, kris_y = 100;
static int kris_dir = 0; // 0=down, 1=left, 2=right, 3=up
static int walk_frame = 0;
static int walk_timer = 0;
static bool is_moving = false;

static Step trail[TRAIL_LEN];

static int star_anim_timer = 0;
static int battle_trigger_type = -1;

// Dialogue scripts for overworld interactions
static const DialogueEntry toriel_dialogue[] = {
    { "Kris! It is time for school!\nDon't forget to get chalk from\nthe supply closet!", &spr_face_toriel_warm, VOICE_TORIEL }
};

static const DialogueEntry closet_warning[] = {
    { "The supply closet is dark...\nToo dark to see anything.", NULL, VOICE_DEFAULT },
    { "Susie: Hey, Kris...\nIs it just me, or does this\ncloset look... weird?", &spr_face_susie_neutral, VOICE_SUSIE }
};

static const DialogueEntry ralsei_intro[] = {
    { "Ralsei: Welcome, heroes of light!\nI am Ralsei, Prince of the Dark.", &spr_face_ralsei_neutral, VOICE_RALSEI },
    { "Ralsei: The Prophecy foretells of\na Human, a Monster, and a Prince\nwho will seal the Dark Fountain!", &spr_face_ralsei_smile, VOICE_RALSEI },
    { "Susie: ...Whatever. Just tell us\nhow to get out of here.", &spr_face_susie_angry, VOICE_SUSIE }
};

static const DialogueEntry save_text[] = {
    { "* At times, you may find it\n  helpful to take a moment...\n* (HP was fully restored.)", NULL, VOICE_DEFAULT }
};

static const DialogueEntry lancer_encounter[] = {
    { "Lancer: HO HO HO!\nFools! You've stepped right into\nmy trap!", &spr_face_lancer_smile, VOICE_DEFAULT },
    { "Susie: Who the heck are you supposed\nto be? A bowling ball?", &spr_face_susie_angry, VOICE_SUSIE },
    { "Lancer: I am LANCER! The bad guy!\nPrepare to be crushed, clowns!", &spr_face_lancer_smile, VOICE_DEFAULT }
};

static const DialogueEntry jevil_door_dialogue[] = {
    { "* Through the ancient locked door,\n  a strange voice echoes...", NULL, VOICE_DEFAULT },
    { "Jevil: UEE HEE HEE!\nA VISITOR! A VISITOR!\nSO LONELY, LONELY I HAVE BEEN!", NULL, VOICE_DEFAULT },
    { "Jevil: PLAY A GAME WITH ME!\nCHAOS, CHAOS!", NULL, VOICE_DEFAULT }
};

static void on_lancer_dialogue_finished(void) {
    battle_trigger_type = 1; // Lancer Boss Battle
}

static void on_jevil_dialogue_finished(void) {
    battle_trigger_type = 2; // Jevil Boss Battle
}

void overworld_init(RoomId initial_room) {
    current_room = initial_room;
    kris_x = 40;
    kris_y = 110;
    kris_dir = 2; // facing right
    walk_frame = 0;
    walk_timer = 0;
    is_moving = false;
    star_anim_timer = 0;
    battle_trigger_type = -1;

    for (int i = 0; i < TRAIL_LEN; i++) {
        trail[i].x = kris_x;
        trail[i].y = kris_y;
        trail[i].dir = kris_dir;
        trail[i].moving = false;
    }

    if (current_room == ROOM_LIGHT_SCHOOL) {
        sound_stop_bgm();
    } else if (current_room == ROOM_DARK_CLIFFS || current_room == ROOM_CASTLE_TOWN) {
        sound_play_bgm(&bgm_dark_world);
    } else if (current_room == ROOM_FIELD) {
        sound_play_bgm(&bgm_dark_world);
    }
}

bool overworld_check_battle_trigger(int *out_enemy_type) {
    if (battle_trigger_type >= 0) {
        if (out_enemy_type) *out_enemy_type = battle_trigger_type;
        battle_trigger_type = -1;
        return true;
    }
    return false;
}

void overworld_return_from_battle(void) {
    if (current_room == ROOM_LIGHT_SCHOOL) {
        sound_stop_bgm();
    } else {
        sound_play_bgm(&bgm_dark_world);
    }
}

static void update_trail(int x, int y, int dir, bool moving) {
    for (int i = TRAIL_LEN - 1; i > 0; i--) {
        trail[i] = trail[i - 1];
    }
    trail[0].x = x;
    trail[0].y = y;
    trail[0].dir = dir;
    trail[0].moving = moving;
}

void overworld_update(u32 keys_down, u32 keys_held) {
    star_anim_timer++;

    if (dialogue_is_active()) {
        dialogue_update(keys_down, keys_held);
        return;
    }

    int speed = (keys_held & KEY_B) ? 3 : 2;
    int dx = 0, dy = 0;

    if (keys_held & KEY_UP)    { dy -= speed; kris_dir = 3; }
    if (keys_held & KEY_DOWN)  { dy += speed; kris_dir = 0; }
    if (keys_held & KEY_LEFT)  { dx -= speed; kris_dir = 1; }
    if (keys_held & KEY_RIGHT) { dx += speed; kris_dir = 2; }

    is_moving = (dx != 0 || dy != 0);

    if (is_moving) {
        kris_x += dx;
        kris_y += dy;

        // Clamp to room bounds
        if (kris_x < 16) kris_x = 16;
        if (kris_x > 230) kris_x = 230;
        if (kris_y < 50) kris_y = 50;
        if (kris_y > 150) kris_y = 150;

        walk_timer++;
        if (walk_timer >= 6) {
            walk_timer = 0;
            walk_frame = (walk_frame + 1) % 4;
        }
        update_trail(kris_x, kris_y, kris_dir, true);
    } else {
        walk_frame = 0;
        walk_timer = 0;
        update_trail(kris_x, kris_y, kris_dir, false);
    }

    // Check interaction button (A)
    if (keys_down & KEY_A) {
        // Room 0: School
        if (current_room == ROOM_LIGHT_SCHOOL) {
            // Toriel at x=40, y=70
            if (kris_x < 70 && kris_y < 90) {
                dialogue_start(toriel_dialogue, 1, SCREEN_BOT, NULL);
            }
            // Supply closet door at right (x > 210)
            else if (kris_x > 200) {
                dialogue_start(closet_warning, 2, SCREEN_BOT, NULL);
            }
        }
        // Room 2: Castle Town
        else if (current_room == ROOM_CASTLE_TOWN) {
            // Ralsei at x=140, y=90
            if (kris_x > 110 && kris_x < 170 && kris_y < 120) {
                dialogue_start(ralsei_intro, 3, SCREEN_BOT, NULL);
            }
            // Save Star at x=70, y=100
            else if (kris_x > 50 && kris_x < 90 && kris_y > 80 && kris_y < 120) {
                sound_play_sfx(&snd_bell);
                dialogue_start(save_text, 1, SCREEN_BOT, NULL);
            }
        }
        // Room 3: Field of Hopes and Dreams
        else if (current_room == ROOM_FIELD) {
            // Save Star at x=60, y=100
            if (kris_x > 40 && kris_x < 80 && kris_y > 80 && kris_y < 120) {
                sound_play_sfx(&snd_bell);
                dialogue_start(save_text, 1, SCREEN_BOT, NULL);
            }
            // Lancer encounter at x=200, y=100
            else if (kris_x > 180 && kris_y > 70 && kris_y < 130) {
                sound_play_sfx(&snd_bell);
                dialogue_start(lancer_encounter, 3, SCREEN_BOT, on_lancer_dialogue_finished);
            }
            // Secret Jevil door at x=30, y=55
            else if (kris_x < 50 && kris_y < 75) {
                sound_play_sfx(&snd_jevil_chaos);
                dialogue_start(jevil_door_dialogue, 3, SCREEN_BOT, on_jevil_dialogue_finished);
            }
        }
    }

    // Room transition triggers
    if (current_room == ROOM_LIGHT_SCHOOL && kris_x >= 225) {
        sound_play_sfx(&snd_door);
        current_room = ROOM_DARK_CLIFFS;
        kris_x = 24;
        sound_play_bgm(&bgm_dark_world);
    } else if (current_room == ROOM_DARK_CLIFFS && kris_x >= 225) {
        current_room = ROOM_CASTLE_TOWN;
        kris_x = 24;
    } else if (current_room == ROOM_CASTLE_TOWN && kris_x >= 225) {
        sound_play_sfx(&snd_door);
        current_room = ROOM_FIELD;
        kris_x = 24;
    }

    // Patrolling Rudinn battle trigger in Field
    if (current_room == ROOM_FIELD && kris_x > 130 && kris_x < 155 && kris_y > 80 && kris_y < 120) {
        sound_play_sfx(&snd_hurt);
        battle_trigger_type = 0; // Rudinn battle
    }
}

static void draw_save_star(int screen, int x, int y, int timer) {
    int pulse = (timer / 8) % 4;
    int sz = 4 + (pulse == 1 || pulse == 3 ? 1 : 0);
    // Golden center and diamond points
    renderer_draw_rect(screen, x - sz, y - 1, sz * 2 + 1, 3, COL_YELLOW);
    renderer_draw_rect(screen, x - 1, y - sz, 3, sz * 2 + 1, COL_YELLOW);
    renderer_draw_rect(screen, x - 2, y - 2, 5, 5, COL_WHITE);
}

void overworld_render(void) {
    // ==========================================
    // TOP SCREEN: Overworld Environment & Party
    // ==========================================
    if (current_room == ROOM_LIGHT_SCHOOL) {
        // Light World classroom / hallway
        renderer_draw_rect(SCREEN_TOP, 0, 0, SCREEN_W, 60, RGB15(16, 20, 22)); // classroom wall
        renderer_draw_rect(SCREEN_TOP, 0, 60, SCREEN_W, 132, RGB15(22, 17, 12)); // wood floor

        // Lockers & chalkboard
        renderer_draw_rect(SCREEN_TOP, 20, 20, 60, 36, RGB15(4, 12, 6)); // chalkboard
        renderer_draw_rect_outline(SCREEN_TOP, 20, 20, 60, 36, RGB15(18, 12, 8), 2);
        renderer_draw_text(SCREEN_TOP, 28, 30, "CLASS", COL_WHITE);

        // Desks
        renderer_draw_rect(SCREEN_TOP, 95, 80, 32, 20, RGB15(16, 10, 6));
        renderer_draw_rect(SCREEN_TOP, 145, 80, 32, 20, RGB15(16, 10, 6));

        // Supply closet door on right
        renderer_draw_rect(SCREEN_TOP, 220, 40, 26, 65, RGB15(2, 2, 4));
        renderer_draw_rect_outline(SCREEN_TOP, 220, 40, 26, 65, RGB15(12, 12, 16), 2);
        renderer_draw_text(SCREEN_TOP, 224, 46, "CLOSET", COL_GRAY);

        // Toriel sprite / indicator at desk
        renderer_draw_rect(SCREEN_TOP, 40, 66, 18, 26, RGB15(24, 24, 26)); // Toriel body
        renderer_draw_rect(SCREEN_TOP, 42, 58, 14, 12, RGB15(30, 30, 30)); // Toriel head
        renderer_draw_text(SCREEN_TOP, 34, 42, "TORIEL", COL_YELLOW);

    } else if (current_room == ROOM_DARK_CLIFFS) {
        // Dark World Abyss / Cliffs
        renderer_draw_gradient_v(SCREEN_TOP, 0, 0, SCREEN_W, SCREEN_H, RGB15(1, 0, 3), RGB15(6, 2, 12));

        // Floating dark platforms
        renderer_draw_rect(SCREEN_TOP, 0, 130, SCREEN_W, 62, RGB15(10, 6, 18));
        renderer_draw_rect(SCREEN_TOP, 0, 126, SCREEN_W, 4, RGB15(18, 10, 28));

        // Floating ambient dust particles
        for (int p = 0; p < 16; p++) {
            int px = (p * 47 + star_anim_timer) % SCREEN_W;
            int py = (p * 31 + (star_anim_timer / 2)) % 120;
            renderer_set_pixel(SCREEN_TOP, px, py, RGB15(20, 16, 28));
        }

        renderer_draw_text(SCREEN_TOP, 50, 20, "?? ?? ?? - THE ABYSS", RGB15(16, 16, 24));

    } else if (current_room == ROOM_CASTLE_TOWN) {
        // Castle Town
        renderer_draw_gradient_v(SCREEN_TOP, 0, 0, SCREEN_W, SCREEN_H, RGB15(2, 0, 6), RGB15(12, 4, 16));

        // Distant Castle Spire silhouette
        renderer_draw_rect(SCREEN_TOP, 190, 20, 30, 80, RGB15(4, 2, 10));
        renderer_draw_rect(SCREEN_TOP, 200, 10, 10, 15, RGB15(4, 2, 10));

        // Ground cliffs
        renderer_draw_rect(SCREEN_TOP, 0, 120, SCREEN_W, 72, RGB15(12, 8, 20));
        renderer_draw_rect(SCREEN_TOP, 0, 116, SCREEN_W, 4, RGB15(24, 16, 31));

        // Save Star at x=70, y=100
        draw_save_star(SCREEN_TOP, 70, 100, star_anim_timer);

        // Ralsei standing as NPC if not recruited yet
        renderer_draw_sprite(SCREEN_TOP, &spr_ralsei_dw[0][0], 140, 85);
        renderer_draw_text(SCREEN_TOP, 130, 72, "RALSEI", COL_GREEN);

        // The Great Door at the right edge
        renderer_draw_rect(SCREEN_TOP, 236, 60, 20, 60, RGB15(31, 20, 4));

    } else if (current_room == ROOM_FIELD) {
        // Field of Hopes and Dreams!
        renderer_draw_sprite(SCREEN_TOP, &bg_field_banner, 0, 0);
        // Lower ground (deep indigo/purple turf)
        renderer_draw_rect(SCREEN_TOP, 0, 112, SCREEN_W, 80, RGB15(6, 4, 16));
        renderer_draw_rect(SCREEN_TOP, 0, 110, SCREEN_W, 2, RGB15(18, 12, 28));

        // Save Star at x=60, y=100
        draw_save_star(SCREEN_TOP, 60, 100, star_anim_timer);

        // Patrolling Rudinn at x=142, y=95
        int rudinn_bob = (star_anim_timer / 12) % 2;
        renderer_draw_sprite(SCREEN_TOP, &spr_rudinn_b_idle[rudinn_bob], 140, 88);
        renderer_draw_text(SCREEN_TOP, 136, 74, "! ENEMY !", COL_RED);

        // Lancer waiting at roadblock (x=205, y=95)
        renderer_draw_sprite(SCREEN_TOP, &spr_lancer_dw[1][0], 205, 88);
        renderer_draw_text(SCREEN_TOP, 198, 74, "LANCER", COL_CYAN);

        // Secret Door at (x=20, y=50)
        renderer_draw_rect(SCREEN_TOP, 20, 50, 20, 30, RGB15(2, 2, 4));
        renderer_draw_rect_outline(SCREEN_TOP, 20, 50, 20, 30, RGB15(18, 0, 24), 2);
        renderer_draw_text(SCREEN_TOP, 12, 38, "? JEVIL ?", COL_PURPLE);
    }

    // Render Party Members
    // In Dark World: draw followers (Susie & Ralsei) behind Kris!
    if (current_room != ROOM_LIGHT_SCHOOL) {
        // Ralsei follower (at trail[28])
        int r_idx = 28;
        if (r_idx < TRAIL_LEN) {
            int r_dir = trail[r_idx].dir;
            int r_frame = trail[r_idx].moving ? walk_frame : 0;
            renderer_draw_sprite(SCREEN_TOP, &spr_ralsei_dw[r_dir][r_frame], trail[r_idx].x - 10, trail[r_idx].y - 20);
        }

        // Susie follower (at trail[14])
        int s_idx = 14;
        if (s_idx < TRAIL_LEN) {
            int s_dir = trail[s_idx].dir;
            int s_frame = trail[s_idx].moving ? walk_frame : 0;
            renderer_draw_sprite(SCREEN_TOP, &spr_susie_dw[s_dir][s_frame], trail[s_idx].x - 13, trail[s_idx].y - 22);
        }

        // Kris (Leader in Dark World)
        renderer_draw_sprite(SCREEN_TOP, &spr_kris_dw[kris_dir][walk_frame], kris_x - 9, kris_y - 19);
    } else {
        // Kris in Light World school clothes
        renderer_draw_sprite(SCREEN_TOP, &spr_kris_lw[kris_dir][walk_frame], kris_x - 9, kris_y - 19);
    }

    // ==========================================
    // BOTTOM SCREEN: Party Status HUD & Dialogue
    // ==========================================
    renderer_clear(SCREEN_BOT, COL_BLACK);

    // Header bar
    renderer_draw_rect(SCREEN_BOT, 0, 0, SCREEN_W, 24, RGB15(3, 2, 8));
    renderer_draw_rect(SCREEN_BOT, 0, 23, SCREEN_W, 1, RGB15(12, 10, 20));

    const char *location_name = "HOMETOWN - SCHOOL";
    if (current_room == ROOM_DARK_CLIFFS)  location_name = "THE DARK WORLD - CLIFFS";
    if (current_room == ROOM_CASTLE_TOWN)  location_name = "CASTLE TOWN - GRAND FOUNTAIN";
    if (current_room == ROOM_FIELD)        location_name = "FIELD OF HOPES AND DREAMS";

    renderer_draw_text(SCREEN_BOT, 8, 4, location_name, COL_WHITE);
    renderer_draw_text(SCREEN_BOT, 200, 4, "$ 120", COL_GOLD);

    // Party Cards (Kris, Susie, Ralsei)
    // Kris
    renderer_draw_rect(SCREEN_BOT, 6, 32, 76, 76, RGB15(1, 2, 4));
    renderer_draw_rect_outline(SCREEN_BOT, 6, 32, 76, 76, COL_CYAN, 1);
    renderer_draw_text(SCREEN_BOT, 12, 36, "KRIS", COL_CYAN);
    renderer_draw_text(SCREEN_BOT, 12, 54, "LV 1", COL_WHITE);
    renderer_draw_text(SCREEN_BOT, 12, 70, "HP 90/90", COL_WHITE);
    renderer_draw_rect(SCREEN_BOT, 12, 90, 64, 6, COL_DARK_GRAY);
    renderer_draw_rect(SCREEN_BOT, 12, 90, 64, 6, COL_CYAN);

    // Susie
    renderer_draw_rect(SCREEN_BOT, 90, 32, 76, 76, RGB15(4, 1, 3));
    renderer_draw_rect_outline(SCREEN_BOT, 90, 32, 76, 76, COL_MAGENTA, 1);
    renderer_draw_text(SCREEN_BOT, 96, 36, "SUSIE", COL_MAGENTA);
    renderer_draw_text(SCREEN_BOT, 96, 54, "LV 1", COL_WHITE);
    renderer_draw_text(SCREEN_BOT, 96, 70, "HP 110/110", COL_WHITE);
    renderer_draw_rect(SCREEN_BOT, 96, 90, 64, 6, COL_DARK_GRAY);
    renderer_draw_rect(SCREEN_BOT, 96, 90, 64, 6, COL_MAGENTA);

    // Ralsei
    renderer_draw_rect(SCREEN_BOT, 174, 32, 76, 76, RGB15(1, 3, 1));
    renderer_draw_rect_outline(SCREEN_BOT, 174, 32, 76, 76, COL_GREEN, 1);
    renderer_draw_text(SCREEN_BOT, 180, 36, "RALSEI", COL_GREEN);
    renderer_draw_text(SCREEN_BOT, 180, 54, "LV 1", COL_WHITE);
    renderer_draw_text(SCREEN_BOT, 180, 70, "HP 70/70", COL_WHITE);
    renderer_draw_rect(SCREEN_BOT, 180, 90, 64, 6, COL_DARK_GRAY);
    renderer_draw_rect(SCREEN_BOT, 180, 90, 64, 6, COL_GREEN);

    // Overworld hint footer
    renderer_draw_rect(SCREEN_BOT, 0, 168, SCREEN_W, 24, RGB15(2, 2, 4));
    renderer_draw_text(SCREEN_BOT, 8, 172, "[D-PAD] Walk  [B] Run  [A] Interact", COL_GRAY);

    // Dialogue overlay if active
    if (dialogue_is_active()) {
        dialogue_render();
    }
}
