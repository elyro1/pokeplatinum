#include "port/gameplay_config.h"

static GameplayConfig sGameplayConfig;

void GameplayConfig_LoadDefaults(GameplayConfig *config)
{
    // Allow HM moves to be replaced when learning another move, including after leveling up in battle.
    // FALSE restores HM replacement protection; field-use requirements and HM item reuse stay unchanged.
    config->forgettableHMs = TRUE;

    // Let the Pastoria City Move Maniac relearn moves without requiring or consuming a Heart Scale.
    // FALSE restores the Heart Scale requirement and payment; other move tutors are unaffected.
    config->freeMoveRelearning = TRUE;

    // Stop counting Great Marsh steps and disable the 500-step exit, while retaining the entry fee and Safari Ball limit.
    // FALSE resumes step counting from the saved counter and restores the 500-step exit.
    config->unlimitedGreatMarshSteps = TRUE;

    // Create eggs with zero remaining hatch cycles and set existing party eggs to zero at their next hatch check.
    // FALSE uses normal cycles for new eggs but does not restore cycles already removed from existing eggs.
    config->zeroEggCycles = TRUE;

    // Make unexpired, slathered Honey Trees encounter-ready immediately when checked again, retaining encounter tables and failure chances.
    // FALSE restores the six-hour wait; Honey costs and the 24-hour expiration remain unchanged in either mode.
    config->instantHoneyTrees = TRUE;

    // Keep a TM in the Bag after successfully teaching its move; selling or discarding it still removes it normally.
    // FALSE consumes TMs after teaching, while HMs remain reusable in either mode.
    config->infiniteTMs = TRUE;

    // Set the total shiny rolls for newly generated eligible Pokemon: 1 keeps original odds, and each extra roll has a 1/8192 chance.
    // 0 also keeps original odds; extra rolls preserve existing Pokemon, nature, gender, ability, Wurmple's evolution branch, and shiny locks.
    config->shinyRolls = 1;

    // Divide the normal steps per hatch check by this value, rounding down to at least one step; 5 reduces 255 steps to 51.
    // 0 or 1 keeps normal timing, ability bonuses still apply, and zeroEggCycles eggs hatch at the next check instead of completing normal cycles.
    config->eggHatchSpeedMultiplier = 5;
}

void GameplayConfig_Init(void)
{
    GameplayConfig_LoadDefaults(&sGameplayConfig);
}

GameplayConfig *GameplayConfig_Get(void)
{
    return &sGameplayConfig;
}
