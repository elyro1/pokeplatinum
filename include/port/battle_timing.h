#ifndef POKEPLATINUM_PORT_BATTLE_TIMING_H
#define POKEPLATINUM_PORT_BATTLE_TIMING_H

#include <nitro/types.h>

// Ownership follows callbacks and their child tasks, never task priority.
typedef enum BattleTimingDomain {
    BATTLE_TIMING_NONE,
    BATTLE_TIMING_ANIMATION,
    BATTLE_TIMING_UI,
} BattleTimingDomain;

#ifdef SDK_PORT
void BattleTiming_SetActive(BOOL active);
BOOL BattleTiming_IsActive(void);
void BattleTiming_BeginFrame(void);
u32 BattleTiming_GetFrame(void);
u32 BattleTiming_GetVisualEpoch(void);
BOOL BattleTiming_ShouldSmoothAnimations(void);
BOOL BattleTiming_ShouldTick(BattleTimingDomain domain);
BattleTimingDomain BattleTiming_GetDomain(void);
BattleTimingDomain BattleTiming_SetDomain(BattleTimingDomain domain);
#else
static inline void BattleTiming_SetActive(BOOL active)
{
    (void)active;
}
static inline BOOL BattleTiming_IsActive(void)
{
    return FALSE;
}
static inline void BattleTiming_BeginFrame(void)
{
}
static inline u32 BattleTiming_GetFrame(void)
{
    return 0;
}
static inline u32 BattleTiming_GetVisualEpoch(void)
{
    return 0;
}
static inline BOOL BattleTiming_ShouldSmoothAnimations(void)
{
    return FALSE;
}
static inline BOOL BattleTiming_ShouldTick(BattleTimingDomain domain)
{
    (void)domain;
    return TRUE;
}
static inline BattleTimingDomain BattleTiming_GetDomain(void)
{
    return BATTLE_TIMING_NONE;
}
static inline BattleTimingDomain BattleTiming_SetDomain(BattleTimingDomain domain)
{
    (void)domain;
    return BATTLE_TIMING_NONE;
}
#endif

#endif
