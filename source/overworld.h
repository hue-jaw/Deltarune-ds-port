#ifndef OVERWORLD_H
#define OVERWORLD_H

#include <nds.h>

typedef enum {
    ROOM_LIGHT_SCHOOL = 0,
    ROOM_DARK_CLIFFS  = 1,
    ROOM_CASTLE_TOWN  = 2,
    ROOM_FIELD        = 3
} RoomId;

void overworld_init(RoomId initial_room);
void overworld_update(u32 keys_down, u32 keys_held);
void overworld_render(void);

// External callbacks/flags
bool overworld_check_battle_trigger(int *out_enemy_type);
void overworld_return_from_battle(void);

#endif
