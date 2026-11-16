#ifndef SANS_FIGHT_H
#define SANS_FIGHT_H

#include <nds.h>

void sans_fight_init(void);
void sans_fight_update(u32 keys_down, u32 keys_held, touchPosition touch);
void sans_fight_render(void);
bool sans_fight_is_finished(void);

#endif
