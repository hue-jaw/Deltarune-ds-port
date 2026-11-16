#ifndef BATTLE_H
#define BATTLE_H

#include <nds.h>

typedef enum {
    BATTLE_ENEMY_RUDINN = 0,
    BATTLE_ENEMY_LANCER = 1,
    BATTLE_ENEMY_JEVIL  = 2
} BattleEnemyType;

void battle_init(BattleEnemyType enemy_type);
void battle_update(u32 keys_down, u32 keys_held, touchPosition touch);
void battle_render(void);
bool battle_is_finished(void);

#endif
