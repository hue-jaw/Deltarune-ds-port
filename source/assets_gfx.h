#ifndef ASSETS_GFX_H
#define ASSETS_GFX_H

#include <nds.h>

typedef struct {
    u16 w, h;
    const u16 *data;
} GfxImage;

// Overworld walk sprites: [direction 0..3][frame 0..3]
extern const GfxImage spr_kris_dw[4][4];
extern const GfxImage spr_susie_dw[4][4];
extern const GfxImage spr_ralsei_dw[4][4];
extern const GfxImage spr_kris_lw[4][4];
extern const GfxImage spr_lancer_dw[4][2];

// Battle party sprites
extern const GfxImage spr_kris_b_idle[2];
extern const GfxImage spr_kris_b_attack[3];
extern const GfxImage spr_kris_b_defend;
extern const GfxImage spr_kris_b_hurt;

extern const GfxImage spr_susie_b_idle[2];
extern const GfxImage spr_susie_b_attack[3];
extern const GfxImage spr_susie_b_defend;
extern const GfxImage spr_susie_b_hurt;

extern const GfxImage spr_ralsei_b_idle[2];
extern const GfxImage spr_ralsei_b_spell[3];
extern const GfxImage spr_ralsei_b_defend;
extern const GfxImage spr_ralsei_b_hurt;

// Enemy sprites
// Rudinn
extern const GfxImage spr_rudinn_b_idle[2];
extern const GfxImage spr_rudinn_b_hurt;

// Lancer
extern const GfxImage spr_lancer_b_idle[2];
extern const GfxImage spr_lancer_b_hurt;

// K. Round (Checker King)
extern const GfxImage spr_kround_b_idle[4];
extern const GfxImage spr_kround_b_bow;

// King (Chaos King)
extern const GfxImage spr_king_b_idle[5];
extern const GfxImage spr_king_b_hurt;

// Jevil
extern const GfxImage spr_jevil_b_idle[4];
extern const GfxImage spr_jevil_b_attack;
extern const GfxImage spr_jevil_b_hurt;

// Soul & Bullets
extern const GfxImage spr_soul;
extern const GfxImage spr_soul_graze;
extern const GfxImage spr_bullet_spade;
extern const GfxImage spr_bullet_diamond;
extern const GfxImage spr_bullet_scythe;
extern const GfxImage spr_bullet_pellet;

// UI Buttons
extern const GfxImage spr_btn[5][3];

// Portraits
extern const GfxImage spr_face_susie_neutral;
extern const GfxImage spr_face_susie_smile;
extern const GfxImage spr_face_susie_angry;
extern const GfxImage spr_face_ralsei_neutral;
extern const GfxImage spr_face_ralsei_smile;
extern const GfxImage spr_face_ralsei_blush;
extern const GfxImage spr_face_lancer_smile;
extern const GfxImage spr_face_toriel_warm;
extern const GfxImage spr_face_king_furious;

// Backgrounds / patterns
extern const GfxImage bg_battle_tile;
extern const GfxImage bg_field_banner;

#endif
