#include "port/battle_timing.h"

#include "port/sim_config_prj.h"

static BOOL sActive;
static u32 sVisualEpoch = 1;
static BOOL sSmooth;
static u32 sFrame = 1;
static BOOL sTick = TRUE;
static BOOL sAnimationFix;
static BOOL sUiFix;
static BattleTimingDomain sDomain;

static void ReadPolicy(void)
{
    const SIM_Config_prj_type *config = SIM_Config_prj_GetConfig();
    BOOL animationFix = config->enable60fps && config->battlePacing != BATTLE_PACING_QUICK;
    BOOL uiFix = config->enable60fps && config->battleUiSpeedFix;
    BOOL smooth = animationFix && config->battlePacing == BATTLE_PACING_SMOOTH;
    if (smooth != sSmooth || animationFix != sAnimationFix || uiFix != sUiFix) {
        sVisualEpoch++;
    }
    sSmooth = smooth;
    if (animationFix != sAnimationFix || uiFix != sUiFix) {
        // A live change keeps task state and starts a fresh reference tick.
        sTick = TRUE;
    }
    sAnimationFix = animationFix;
    sUiFix = uiFix;
}

void BattleTiming_SetActive(BOOL active)
{
    sVisualEpoch++;
    sActive = active;
    sTick = TRUE;
    ReadPolicy();
}

BOOL BattleTiming_IsActive(void)
{
    return sActive;
}

void BattleTiming_BeginFrame(void)
{
    sFrame++;
    sTick = !sTick;
    ReadPolicy();
}

u32 BattleTiming_GetFrame(void)
{
    return sFrame;
}

u32 BattleTiming_GetVisualEpoch(void)
{
    return sVisualEpoch;
}

BOOL BattleTiming_ShouldSmoothAnimations(void)
{
    return sActive && sSmooth;
}

BOOL BattleTiming_ShouldTick(BattleTimingDomain domain)
{
    if (!sActive || sTick) {
        return TRUE;
    }
    switch (domain) {
    case BATTLE_TIMING_ANIMATION:
        return !sAnimationFix;
    case BATTLE_TIMING_UI:
        return !sUiFix;
    default:
        return TRUE;
    }
}

BattleTimingDomain BattleTiming_GetDomain(void)
{
    return sActive ? sDomain : BATTLE_TIMING_NONE;
}

BattleTimingDomain BattleTiming_SetDomain(BattleTimingDomain domain)
{
    BattleTimingDomain previous = sDomain;
    sDomain = sActive ? domain : BATTLE_TIMING_NONE;
    return previous;
}
