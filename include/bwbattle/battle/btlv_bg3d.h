#ifndef POKEBW2_BATTLE_BTLV_BG3D_H
#define POKEBW2_BATTLE_BTLV_BG3D_H

#include "types.h"
#include "nnsys/g2d.h"

// bwbattle: the effects' background (BG 3 in BW2, loaded from archive 94 by BtlvEffvm_CmdLoadBg) drawn as a textured
// quad in the 3D scene. Platinum's battle keeps the 2D BG layers and BG palettes for its textbox and menus, so the
// background that BW2 puts on BG 3 behind the 3D scene (BG 0) is drawn in it instead: at the back, or in front when
// the effect gives BG 3 a priority above BG 0's. Its tiles and screen come through BW2's BG system, which stays
// uncreated (bg_sys.c), its palettes from the view's palette fade (the main BG buffer), its scroll from the BG
// offsets BW2 sets, and its lines' offsets from BW2's wave (btlv_bg.c).

void BtlvBg3d_LoadChars(const void *chars, u32 size, u32 offset);
void BtlvBg3d_LoadScreen(const NNSG2dScreenData *screen);
void BtlvBg3d_SetEnabled(BOOL enabled);
void BtlvBg3d_SetPriority(u8 priority);
void BtlvBg3d_SetOffset(int x, int y);
// BW2's alpha blend of BG 3: the second weight (0 to 16), with which the front background lets the scene under it
// through; 0 turns it off
void BtlvBg3d_SetBlend(int weight);
// A wave's offsets, line by line: line y is shifted by table[(y + *shift + 1) % lines], across with 192 lines and up
// and down with 256, as BW2's H-blank task does. NULL ends it
void BtlvBg3d_SetWave(const s16 *table, u32 lines, const s16 *shift);
int BtlvBg3d_GetOffsetX(void);
int BtlvBg3d_GetOffsetY(void);
// Draws the background behind the scene (front FALSE, before it) or in front of it (TRUE, after it), when it is shown
// and its priority puts it there
void BtlvBg3d_Draw(BOOL front);
// Drops the background, at an effect's end
void BtlvBg3d_Unload(void);
void BtlvBg3d_Free(void);

#endif // POKEBW2_BATTLE_BTLV_BG3D_H
