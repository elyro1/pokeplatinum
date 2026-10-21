#include <assert.h>
#include <stdio.h>

// Include the item implementation to exercise its internal EV update calculation.
#include "../../src/item_use_pokemon.c"

int main(void)
{
    GameplayConfig_Init();
    GameplayConfig *config = GameplayConfig_Get();
    assert(config->prohibitBattleItems);
    assert(config->forceSetBattleStyle);
    assert(config->unlimitedVitaminEVs);

    assert(CalculateEVUpdate(100, 0, 10) == 110);
    assert(CalculateEVUpdate(249, 0, 10) == 255);
    assert(CalculateEVUpdate(255, 0, 10) == EV_UNCHANGED);
    assert(CalculateEVUpdate(100, 405, 10) == 105);
    assert(CalculateEVUpdate(100, 410, 10) == EV_UNCHANGED);
    assert(CalculateEVUpdate(5, 0, -10) == 0);
    config->unlimitedVitaminEVs = FALSE;
    assert(CalculateEVUpdate(95, 0, 10) == 100);
    assert(CalculateEVUpdate(100, 0, 10) == EV_UNCHANGED);
    assert(CalculateEVUpdate(200, 0, 10) == EV_UNCHANGED);
    assert(CalculateEVUpdate(200, 0, -10) == 190);

    config->bpPricePercent = 100;
    assert(GameplayConfig_GetBPPrice(ITEM_RARE_CANDY, 48) == 1);
    assert(GameplayConfig_GetBPPrice(ITEM_PROTEIN, 1) == 1);
    assert(GameplayConfig_GetBPPrice(ITEM_CHOICE_BAND, 48) == 48);
    config->bpPricePercent = 50;
    assert(GameplayConfig_GetBPPrice(ITEM_CHOICE_BAND, 48) == 24);
    assert(GameplayConfig_GetBPPrice(ITEM_PROTEIN, 1) == 1);
    assert(GameplayConfig_GetBPPrice(ITEM_RARE_CANDY, 48) == 1);
    config->rareCandyBPPrice = 7;
    assert(GameplayConfig_GetBPPrice(ITEM_RARE_CANDY, 48) == 7);
    config->bpPricePercent = 0;
    assert(GameplayConfig_GetBPPrice(ITEM_CHOICE_BAND, 48) == 0);
    assert(GameplayConfig_GetBPPrice(ITEM_RARE_CANDY, 48) == 7);
    config->rareCandyBPPrice = 0;
    assert(GameplayConfig_GetBPPrice(ITEM_RARE_CANDY, 48) == 0);
    config->bpPricePercent = 65535;
    assert(GameplayConfig_GetBPPrice(ITEM_CHOICE_BAND, 65535) == 65535);
    puts("Gameplay config BP price and vitamin EV checks passed.");
    return 0;
}
