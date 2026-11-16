#ifndef DIALOGUE_H
#define DIALOGUE_H

#include <nds.h>
#include "assets_gfx.h"
#include "sound_manager.h"

typedef struct {
    const char *text;
    const GfxImage *portrait;
    int voice;
} DialogueEntry;

typedef void (*DialogueCallback)(void);

void dialogue_init(void);
void dialogue_start(const DialogueEntry *entries, int count, int screen, DialogueCallback on_finish);
void dialogue_update(u32 keys_down, u32 keys_held);
void dialogue_render(void);
bool dialogue_is_active(void);

#endif
