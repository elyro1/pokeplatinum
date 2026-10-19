#ifndef POKEBW2_BATTLE_BTLV_TIMER_H
#define POKEBW2_BATTLE_BTLV_TIMER_H

// Overlay 168's btlv_timer.c (named by its string), the two countdowns of a timed battle on the upper screen: the
// battle's time and the time to choose a command, as minutes and seconds. The names are ours; swan has none for this
// file

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

BtlvTimer *BtlvTimer_Create(HeapID heapId);
void BtlvTimer_Delete(BtlvTimer *timer);
// Creates the digits of each countdown whose limit isn't 0, at most 5999 seconds (99:59), starts both and hides them
void BtlvTimer_Start(BtlvTimer *timer, s32 battleLimit, s32 commandLimit);
// Shows or hides a countdown, restarting it from its limit when both visible and restart are TRUE
void BtlvTimer_SetVisible(BtlvTimer *timer, int which, BOOL visible, BOOL restart);
// Whether a countdown has run out
BOOL BtlvTimer_IsTimeUp(BtlvTimer *timer, int which);

#endif // POKEBW2_BATTLE_BTLV_TIMER_H
