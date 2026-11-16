#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

#include <nds.h>

typedef enum {
    TITLE_CHOICE_NONE = -1,
    TITLE_CHOICE_STORY = 0,
    TITLE_CHOICE_FIELD = 1,
    TITLE_CHOICE_LANCER = 2,
    TITLE_CHOICE_JEVIL = 3,
    TITLE_CHOICE_LEGEND = 4
} TitleChoice;

void title_screen_init(void);
TitleChoice title_screen_update(u32 keys_down, u32 keys_held, touchPosition touch);
void title_screen_render(void);

#endif
