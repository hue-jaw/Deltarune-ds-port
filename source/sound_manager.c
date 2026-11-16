#include "sound_manager.h"

static int current_bgm_channel = -1;
static const AudioSample *current_bgm_ptr = NULL;

void sound_manager_init(void) {
    soundEnable();
    current_bgm_channel = -1;
    current_bgm_ptr = NULL;
}

void sound_play_bgm(const AudioSample *bgm) {
    if (!bgm || !bgm->data) return;
    if (current_bgm_ptr == bgm && current_bgm_channel >= 0) return; // already playing this bgm

    if (current_bgm_channel >= 0) {
        soundKill(current_bgm_channel);
        current_bgm_channel = -1;
    }

    current_bgm_ptr = bgm;
    current_bgm_channel = soundPlaySample(bgm->data, SoundFormat_8Bit, bgm->size, bgm->freq, 110, 64, true, 0);
}

void sound_stop_bgm(void) {
    if (current_bgm_channel >= 0) {
        soundKill(current_bgm_channel);
        current_bgm_channel = -1;
    }
    current_bgm_ptr = NULL;
}

void sound_play_sfx(const AudioSample *sfx) {
    if (!sfx || !sfx->data) return;
    soundPlaySample(sfx->data, SoundFormat_8Bit, sfx->size, sfx->freq, 127, 64, false, 0);
}

void sound_play_sfx_vol(const AudioSample *sfx, int vol) {
    if (!sfx || !sfx->data) return;
    if (vol < 0) vol = 0;
    if (vol > 127) vol = 127;
    soundPlaySample(sfx->data, SoundFormat_8Bit, sfx->size, sfx->freq, (u8)vol, 64, false, 0);
}

void sound_play_voice(int voice_type) {
    switch (voice_type) {
        case VOICE_DEFAULT:
            sound_play_sfx(&snd_text_default);
            break;
        case VOICE_SUSIE:
            sound_play_sfx(&snd_text_susie);
            break;
        case VOICE_RALSEI:
            sound_play_sfx(&snd_text_ralsei);
            break;
        case VOICE_TORIEL:
            sound_play_sfx(&snd_text_toriel);
            break;
        case VOICE_NONE:
        default:
            break;
    }
}
