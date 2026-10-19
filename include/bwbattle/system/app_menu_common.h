#ifndef POKEBW2_SYSTEM_APP_MENU_COMMON_H
#define POKEBW2_SYSTEM_APP_MENU_COMMON_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The graphics that the game's menus share (app_menu_common.c, a guessed name): the archive's id, and the files of
// its icons. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except
// AppMenuCommon_LoadBarScreen and AppMenuCommon_GetStatusIcon.
//
// Most icons come as four files: a palette and a character file at fixed indices, then a cell and an animation file
// for each of the three OBJ VRAM mappings (32, 64 and 128 KB), picked by `mapping`.

// The status icons, as AppMenuCommon_GetStatusIcon returns them
#define APP_STATUS_ICON_PARALYSIS 1
#define APP_STATUS_ICON_FREEZE 2
#define APP_STATUS_ICON_SLEEP 3
#define APP_STATUS_ICON_POISON 4
#define APP_STATUS_ICON_BURN 5
#define APP_STATUS_ICON_FAINTED 6
#define APP_STATUS_ICON_NONE 8

BOOL func_0202d7d8(void);
void func_0202d7dc(void);
u32 getUINarcIdx(void);

// The type and contest category icons: the palette, each icon's palette within it, the icons, cells and animations
u32 func_0202d7e4(void);
u8 func_0202d7e8(u8 type);
u32 func_0202d7f4(u8 type);
u32 func_0202d7f8(u32 mapping);
u32 func_0202d7fc(u32 mapping);
u8 func_0202d800(u8 index);
u32 func_0202d80c(u8 index);

// The touch bar's icons: their palette, characters, and cells and animations for an OBJ VRAM mapping mode
u32 func_0202d810(void);
u32 func_0202d814(void);
u32 func_0202d818(u32 mapping);
u32 func_0202d81c(u32 mapping);
// The touch bar's BG: its palette, characters and screen
u32 func_0202d820(void);
u32 func_0202d824(void);
u32 func_0202d828(void);
u32 func_0202d82c(void);

// Loads the menu bar's screen into the bottom three rows of bg's screen buffer, with its characters starting at
// charBase and in palette `palette`
void AppMenuCommon_LoadBarScreen(ArcTool *arc, u8 bg, HeapID heapId, u32 charBase, u32 palette);

u32 func_0202d890(void);
u32 func_0202d894(void);
u32 func_0202d898(u32 mapping);
u32 func_0202d89c(u32 mapping);
u32 func_0202d8a0(void);
u32 func_0202d8a4(void);
u32 func_0202d8a8(u32 mapping);
u32 func_0202d8ac(u32 mapping);
u32 func_0202d8b0(void);
u32 func_0202d8b4(void);
u32 func_0202d8b8(u32 mapping);
u32 func_0202d8bc(u32 mapping);

// The status icon (APP_STATUS_ICON_*) of a party Pokémon: fainted, its status condition, or none
u32 AppMenuCommon_GetStatusIcon(PartyPkm *pkm);

u32 func_0202d90c(u32 mapping);
u32 func_0202d910(u32 mapping);
u32 func_0202d914(u32 mapping);
u32 func_0202d918(u32 mapping);
// The files of a Poké Ball's icon: its palette and characters, where ball 0 counts as 4, then its cells and
// animations, the same for every ball
u32 func_0202d91c(u32 ball);
u32 func_0202d928(u32 ball);
u32 func_0202d934(u32 ball, u32 mapping);
u32 func_0202d93c(u32 ball, u32 mapping);
u32 func_0202d944(void);
u32 func_0202d948(u32 mapping);
u32 func_0202d94c(u32 mapping);
u32 func_0202d950(u32 mapping);
u32 func_0202d954(void);
u32 func_0202d958(u32 mapping);
u32 func_0202d95c(u32 mapping);
u32 func_0202d960(u32 mapping);
u32 func_0202d964(void);
u32 func_0202d968(u32 mapping);
u32 func_0202d96c(u32 mapping);
u32 func_0202d970(u32 mapping);

#endif // POKEBW2_SYSTEM_APP_MENU_COMMON_H
