#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <nds.h>
#include "assets_audio.h"

#define VOICE_DEFAULT 0
#define VOICE_SUSIE   1
#define VOICE_RALSEI  2
#define VOICE_TORIEL  3
#define VOICE_NONE    4

void sound_manager_init(void);
void sound_play_bgm(const AudioSample *bgm);
void sound_stop_bgm(void);
void sound_play_sfx(const AudioSample *sfx);
void sound_play_sfx_vol(const AudioSample *sfx, int vol);
void sound_play_voice(int voice_type);

#endif
