#ifndef ASSETS_GFX_H
#define ASSETS_GFX_H

#include <nds.h>

typedef struct {
    u16 w, h;
    const u16 *data;
} GfxImage;

// Overworld walk sprites: [direction 0..3][frame 0..3]
// 0=Down, 1=Left, 2=Right, 3=Up
extern const GfxImage spr_kris_dw[4][4];
extern const GfxImage spr_susie_dw[4][4];
extern const GfxImage spr_ralsei_dw[4][4];
extern const GfxImage spr_kris_lw[4][4];
extern const GfxImage spr_lancer_dw[4][2];

// Battle player sprites
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
extern const GfxImage spr_lancer_b_idle[2];
extern const GfxImage spr_lancer_b_hurt;

extern const GfxImage spr_jevil_b_idle[2];
extern const GfxImage spr_jevil_b_attack;
extern const GfxImage spr_jevil_b_hurt;

extern const GfxImage spr_rudinn_b_idle[2];
extern const GfxImage spr_rudinn_b_hurt;

// Soul & Bullets
extern const GfxImage spr_soul;
extern const GfxImage spr_soul_graze;
extern const GfxImage spr_bullet_spade;
extern const GfxImage spr_bullet_diamond;
extern const GfxImage spr_bullet_scythe;
extern const GfxImage spr_bullet_pellet;

// UI Buttons (FIGHT=0, ACT=1, ITEM=2, SPARE=3, DEFEND=4), [state 0=norm, 1=high, 2=active]
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

// Backgrounds / patterns
extern const GfxImage bg_battle_tile;
extern const GfxImage bg_field_banner;

#endif
