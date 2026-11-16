#ifndef CUSTOM_LOADER_H
#define CUSTOM_LOADER_H

#include <nds.h>

void custom_loader_init(const char *game_name, const char *game_path);
bool custom_loader_update(u32 keys_down, u32 keys_held);
void custom_loader_render(void);

#endif
