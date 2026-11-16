#ifndef ASSETS_AUDIO_H
#define ASSETS_AUDIO_H

#include <nds.h>

typedef struct {
    const s8 *data;
    u32 size;
    u16 freq;
} AudioSample;

extern const AudioSample snd_text_default;
extern const AudioSample snd_text_susie;
extern const AudioSample snd_text_ralsei;
extern const AudioSample snd_text_toriel;
extern const AudioSample snd_menu_switch;
extern const AudioSample snd_menu_confirm;
extern const AudioSample snd_attack;
extern const AudioSample snd_damage;
extern const AudioSample snd_hurt;
extern const AudioSample snd_graze;
extern const AudioSample snd_spell;
extern const AudioSample snd_door;
extern const AudioSample snd_bell;
extern const AudioSample snd_jevil_chaos;
extern const AudioSample snd_jevil_anything;

extern const AudioSample bgm_dark_world;
extern const AudioSample bgm_rude_buster;
extern const AudioSample bgm_jevil;

#endif
