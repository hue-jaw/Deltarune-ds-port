#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <nds.h>

typedef enum {
    LAUNCH_NONE = 0,
    LAUNCH_DELTARUNE_CH1,
    LAUNCH_UNDERTALE_RUINS,
    LAUNCH_SANS_FIGHT,
    LAUNCH_UNDERTALE_YELLOW,
    LAUNCH_TS_UNDERSWAP,
    LAUNCH_SD_CUSTOM
} LauncherAction;

void launcher_init(void);
LauncherAction launcher_update(u32 keys_down, u32 keys_held, touchPosition touch);
void launcher_render(void);

const char *launcher_get_selected_game_path(void);
const char *launcher_get_selected_game_name(void);

#endif
