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

    // Let all three shard move tutors teach compatible moves without requiring or consuming any shards.
    // FALSE restores their original shard prices; displayed costs and tutor greetings follow this setting.
    config->freeMoveTutors = TRUE;

    // Make the Route 212 tutor offer all three shard tutors' move lists, including the post-game Survival Area moves.
    // FALSE restores each tutor's original list; compatibility checks and the separate freeMoveTutors setting still apply.
    config->mainStoryMoveTutors = TRUE;

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

    // Plain trade evolutions also trigger on level-up at or above this level; 0 restores trade-only evolution.
    // Held-item trades use levelUpHeldItemEvolutions instead. Values above 100 cannot trigger.
    config->tradeEvolutionLevel = 40;

    // Evolve on the next level-up holding the required item, including trade items, with no day/night restriction.
    // The item is consumed on successful evolution. FALSE restores trade and time-of-day requirements.
    config->levelUpHeldItemEvolutions = TRUE;

    // Encounter Spiritomb immediately after inserting an Odd Keystone, skipping Underground conversations.
    // Already-inserted Keystones also work; each successful encounter still uses one Keystone.
    config->instantSpiritomb = TRUE;

    // Replace Feebas's Beauty level-up requirement with using a Water Stone.
    // FALSE restores the original Beauty evolution and disables the Water Stone alternative.
    config->feebasWaterStoneEvolution = TRUE;

    // Skip field poison damage and its visual/survival effects; poison status and battle damage remain.
    config->disableOverworldPoisonDamage = TRUE;

    // Set the total shiny rolls for newly generated eligible Pokemon: 1 keeps original odds, and each extra roll has a 1/8192 chance.
    // 0 also keeps original odds; extra rolls preserve existing Pokemon, nature, gender, ability, Wurmple's evolution branch, and shiny locks.
    config->shinyRolls = 1;

    // Multiply positive friendship gains after existing bonuses; friendship losses are unchanged.
    // 0 or 1 keeps original gains. Friendship still caps at 255; initial friendship and egg cycles are unaffected.
    config->friendshipGainMultiplier = 2;

    // Divide the normal steps per hatch check by this value, rounding down to at least one step; 5 reduces 255 steps to 51.
    // 0 or 1 keeps normal timing, ability bonuses still apply, and zeroEggCycles eggs hatch at the next check instead of completing normal cycles.
    config->eggHatchSpeedMultiplier = 5;

    // Multiply the final catch rate after ball, HP, status, and Safari modifiers; 255 or higher guarantees capture.
    // 0 or 1 keeps original rates; Master Balls, guaranteed catches, and trainer capture restrictions still apply.
    config->catchRateMultiplier = 2;
}

void GameplayConfig_Init(void)
{
    GameplayConfig_LoadDefaults(&sGameplayConfig);
}

GameplayConfig *GameplayConfig_Get(void)
{
    return &sGameplayConfig;
}
