#include "cutscene.h"
#include "renderer.h"
#include "sound_manager.h"
#include "dialogue.h"
#include <math.h>

static CutsceneId current_id = CUTSCENE_INTRO_SCHOOL;
static int step = 0;
static int timer = 0;
static bool finished = false;

// Fall animation variables
static float fall_y_kris = 20.0f;
static float fall_y_susie = 0.0f;
static float fall_spd = 2.0f;

// Dialogue scripts for cutscenes
static const DialogueEntry intro_dialogue_1[] = {
    { "Toriel: Kris! Wake up, dear!\nYou are going to be late for school!", &spr_face_toriel_warm, VOICE_TORIEL },
    { "Toriel: I will drive you there.\nCome along now!", &spr_face_toriel_warm, VOICE_TORIEL }
};

static const DialogueEntry intro_dialogue_2[] = {
    { "* You arrive at Hometown School.\n* Miss Alphys is looking for chalk.", NULL, VOICE_DEFAULT },
    { "Alphys: U-um, does anyone have any\nchalk left? We can't start class\nwithout it...", NULL, VOICE_DEFAULT },
    { "* The door bangs open violently.\n* Susie enters the classroom.", NULL, VOICE_DEFAULT },
    { "Susie: ...Am I late?", &spr_face_susie_neutral, VOICE_SUSIE },
    { "Alphys: N-no! Susie! Perfect timing!\nCould you and Kris go get chalk\nfrom the supply closet?", NULL, VOICE_DEFAULT }
};

static const DialogueEntry closet_fall_dialogue[] = {
    { "Susie: ...Hey, Kris.\nIs it just me, or is it getting\nDARKER in here?", &spr_face_susie_neutral, VOICE_SUSIE },
    { "Susie: There's no back to this closet!\nIt's just... endless blackness!", &spr_face_susie_angry, VOICE_SUSIE },
    { "* The floor suddenly collapses beneath\n  your feet!", NULL, VOICE_DEFAULT }
};

static const DialogueEntry landing_dialogue[] = {
    { "* You land with a soft crash onto\n  a mound of dark dust.", NULL, VOICE_DEFAULT },
    { "Susie: Ugh... My head...\nKris?! Where the heck are we?!", &spr_face_susie_angry, VOICE_SUSIE },
    { "Susie: Wait... Look at your clothes!\nAnd look at ME!\nWhat happened to us?!", &spr_face_susie_neutral, VOICE_SUSIE },
    { "Susie: Whatever this place is...\nWe're getting out of here.\nLead the way, Kris.", &spr_face_susie_smile, VOICE_SUSIE }
};

static const DialogueEntry legend_dialogue[] = {
    { "* Once upon a time, a LEGEND was\n  whispered among shadows.", NULL, VOICE_DEFAULT },
    { "* It was a LEGEND of HOPE.\n* It was a LEGEND of DREAMS.\n* It was a LEGEND of LIGHT and DARK.", NULL, VOICE_DEFAULT },
    { "* For millenia, LIGHT and DARK lived\n  in balance, bringing peace.", NULL, VOICE_DEFAULT },
    { "* But if harmony were to shatter...\n  a terrible calamity would arise.", NULL, VOICE_DEFAULT },
    { "* Three HEROES appear at WORLDS' edge:\n  A HUMAN, A MONSTER,\n  And a PRINCE FROM THE DARK.", NULL, VOICE_DEFAULT },
    { "Ralsei: Only then may the fountains\nbe sealed, and peace restored!\nHeroes, I am honored to join you!", &spr_face_ralsei_smile, VOICE_RALSEI }
};

static void on_intro_1_finished(void) {
    step = 1;
    timer = 0;
    dialogue_start(intro_dialogue_2, 5, SCREEN_BOT, NULL);
}

static void on_closet_dialogue_finished(void) {
    step = 2; // Begin tumbling fall animation!
    timer = 0;
    sound_play_sfx(&snd_hurt);
}

static void on_landing_finished(void) {
    finished = true;
}

static void on_legend_finished(void) {
    finished = true;
}

void cutscene_init(CutsceneId id) {
    current_id = id;
    step = 0;
    timer = 0;
    finished = false;
    fall_y_kris = 20.0f;
    fall_y_susie = -20.0f;
    fall_spd = 2.0f;

    if (id == CUTSCENE_INTRO_SCHOOL) {
        sound_stop_bgm();
        dialogue_start(intro_dialogue_1, 2, SCREEN_BOT, on_intro_1_finished);
    } else if (id == CUTSCENE_DARK_FALL) {
        sound_stop_bgm();
        dialogue_start(closet_fall_dialogue, 3, SCREEN_BOT, on_closet_dialogue_finished);
    } else if (id == CUTSCENE_LEGEND) {
        sound_play_bgm(&bgm_dark_world);
        dialogue_start(legend_dialogue, 6, SCREEN_BOT, on_legend_finished);
    }
}

bool cutscene_is_finished(void) {
    return finished;
}

void cutscene_update(u32 keys_down, u32 keys_held) {
    timer++;

    if (dialogue_is_active()) {
        dialogue_update(keys_down, keys_held);
        return;
    }

    if (current_id == CUTSCENE_INTRO_SCHOOL) {
        if (step == 1) {
            // Classroom walk to closet
            if (timer > 20) {
                // Transition to Dark Fall
                current_id = CUTSCENE_DARK_FALL;
                step = 0;
                timer = 0;
                dialogue_start(closet_fall_dialogue, 3, SCREEN_BOT, on_closet_dialogue_finished);
            }
        }
    } else if (current_id == CUTSCENE_DARK_FALL) {
        if (step == 2) {
            // Tumbling fall physics
            fall_spd += 0.1f;
            fall_y_kris += fall_spd;
            fall_y_susie += fall_spd * 0.9f;

            if (timer % 8 == 0) {
                sound_play_sfx(&snd_graze);
            }

            if (fall_y_kris > 220.0f) {
                // Landed in the Dark World!
                step = 3;
                timer = 0;
                sound_play_sfx(&snd_damage);
                sound_play_bgm(&bgm_dark_world);
                dialogue_start(landing_dialogue, 4, SCREEN_BOT, on_landing_finished);
            }
        }
    }
}

void cutscene_render(void) {
    if (current_id == CUTSCENE_INTRO_SCHOOL) {
        // Light World Classroom
        renderer_draw_rect(SCREEN_TOP, 0, 0, SCREEN_W, 60, RGB15(16, 20, 22));
        renderer_draw_rect(SCREEN_TOP, 0, 60, SCREEN_W, 132, RGB15(22, 17, 12));

        // Chalkboard & desks
        renderer_draw_rect(SCREEN_TOP, 20, 20, 60, 36, RGB15(4, 12, 6));
        renderer_draw_text(SCREEN_TOP, 28, 30, "CLASS", COL_WHITE);
        renderer_draw_rect(SCREEN_TOP, 95, 80, 32, 20, RGB15(16, 10, 6));

        // Kris and Susie standing in classroom
        renderer_draw_sprite(SCREEN_TOP, &spr_kris_lw[0][0], 100, 110);
        renderer_draw_sprite(SCREEN_TOP, &spr_susie_dw[0][0], 130, 106);

        renderer_clear(SCREEN_BOT, COL_BLACK);
        if (dialogue_is_active()) dialogue_render();

    } else if (current_id == CUTSCENE_DARK_FALL) {
        // Deep abyss falling sequence!
        renderer_draw_gradient_v(SCREEN_TOP, 0, 0, SCREEN_W, SCREEN_H, RGB15(0, 0, 1), RGB15(3, 0, 7));
        renderer_draw_gradient_v(SCREEN_BOT, 0, 0, SCREEN_W, SCREEN_H, RGB15(3, 0, 7), RGB15(0, 0, 0));

        // Upward rushing speed lines
        for (int i = 0; i < 20; i++) {
            int line_x = (i * 37) % SCREEN_W;
            int line_y = (i * 29 - timer * 8) % SCREEN_H;
            if (line_y < 0) line_y += SCREEN_H;
            renderer_draw_rect(SCREEN_TOP, line_x, line_y, 2, 24, RGB15(14, 10, 24));
            renderer_draw_rect(SCREEN_BOT, line_x, line_y, 2, 24, RGB15(14, 10, 24));
        }

        if (step == 2) {
            // Tumbling sprites!
            int k_rot = (timer / 6) % 4;
            int s_rot = (timer / 8) % 4;
            renderer_draw_sprite(SCREEN_TOP, &spr_kris_dw[k_rot][0], 80, (int)fall_y_kris);
            renderer_draw_sprite(SCREEN_TOP, &spr_susie_dw[s_rot][0], 140, (int)fall_y_susie);
        } else if (step == 3) {
            // Grounded in Dark World
            renderer_draw_rect(SCREEN_TOP, 0, 130, SCREEN_W, 62, RGB15(10, 6, 18));
            renderer_draw_sprite(SCREEN_TOP, &spr_kris_dw[0][0], 80, 105);
            renderer_draw_sprite(SCREEN_TOP, &spr_susie_dw[0][0], 140, 100);
        }

        if (dialogue_is_active()) dialogue_render();

    } else if (current_id == CUTSCENE_LEGEND) {
        // The Legend of Delta Rune
        renderer_clear(SCREEN_TOP, COL_BLACK);
        renderer_clear(SCREEN_BOT, COL_BLACK);

        // Draw glowing Delta Rune Crest on Top Screen:
        // Winged orb on top, three triangles below
        int cx = 128;
        int cy = 60;
        int pulse = (timer / 10) % 4;
        u16 crest_col = (pulse == 0 || pulse == 2) ? COL_WHITE : COL_LIGHT_GRAY;

        // Top winged orb
        renderer_draw_rect(SCREEN_TOP, cx - 8, cy - 8, 16, 16, crest_col);
        renderer_draw_rect(SCREEN_TOP, cx - 28, cy - 4, 16, 8, crest_col); // Left wing
        renderer_draw_rect(SCREEN_TOP, cx + 12, cy - 4, 16, 8, crest_col); // Right wing

        // Three triangles below
        // Left triangle
        renderer_draw_rect(SCREEN_TOP, cx - 24, cy + 24, 14, 14, crest_col);
        // Right triangle
        renderer_draw_rect(SCREEN_TOP, cx + 10, cy + 24, 14, 14, crest_col);
        // Center inverted triangle
        renderer_draw_rect(SCREEN_TOP, cx - 7, cy + 34, 14, 14, crest_col);

        renderer_draw_text(SCREEN_TOP, 54, 120, "THE LEGEND OF DELTA RUNE", COL_GOLD);

        if (dialogue_is_active()) dialogue_render();
    }
}
