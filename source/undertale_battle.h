#ifndef UNDERTALE_BATTLE_H
#define UNDERTALE_BATTLE_H

#include <nds.h>

typedef enum {
    UT_ENEMY_FROGGIT = 0,
    UT_ENEMY_CLOVER   = 1,
    UT_ENEMY_SWAP     = 2
} UtEnemyType;

void undertale_battle_init(UtEnemyType enemy_type);
void undertale_battle_update(u32 keys_down, u32 keys_held, touchPosition touch);
void undertale_battle_render(void);
bool undertale_battle_is_finished(void);

#endif
