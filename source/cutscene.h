#ifndef CUTSCENE_H
#define CUTSCENE_H

#include <nds.h>

typedef enum {
    CUTSCENE_INTRO_SCHOOL = 0,
    CUTSCENE_DARK_FALL    = 1,
    CUTSCENE_LEGEND       = 2
} CutsceneId;

void cutscene_init(CutsceneId id);
void cutscene_update(u32 keys_down, u32 keys_held);
void cutscene_render(void);
bool cutscene_is_finished(void);

#endif
