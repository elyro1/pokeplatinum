#ifndef POKEBW2_BATTLE_BTL_CALC_H
#define POKEBW2_BATTLE_BTL_CALC_H

// Overlay 167's btl_calc.c: the battle's random numbers, type matchups, stat stage ratios, condition makers and target
// selection. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

#include "types.h"
#include "battle/btl_pokeparam.h"
#include "gfl/heap.h"
#include "nitro/math.h"
#include "pml/waza.h"
#include "struct_decls.h"

// Up to four client IDs without repeats, the unused ones 4
typedef struct {
    u32 count;
    u8 clientIds[4];
} BtlClientIDList;

void func_ov167_021bd054(const MATHRandContext32 *rand, HeapID heapId);
void func_ov167_021bd090(const MATHRandContext32 *rand);
void func_ov167_021bd0a8(void);
u32 BattleRandom(u32 max);
u16 GetBoostFromStatStage(u16 value, u8 stage);
u8 func_ov167_021bd11c(u32 value, u32 stage);
BOOL func_ov167_021bd144(u8 critStage);
u32 GetTypeEffectiveness(u8 attackType, u8 defenseType);
s32 func_ov167_021bd1b0(u8 attackType, PokeTypePair defenseTypes);
u32 GetTypeEffectivenessMultiplier(u32 effectiveness1, u32 effectiveness2);
u32 CalcBaseDamage(u32 power, u32 attack, u32 level, u32 defense);
u32 TypeEffectivenessPowerMod(u32 damage, u32 effectiveness);
u8 GetTypeWeaknesses(u8 attackType, u8 *defenseTypes);
u32 func_ov167_021bd2e8(s32 value);
BOOL RollEffectChance(u32 chance);
s32 func_ov167_021bd31c(s32 value, s32 minimum);
u32 fixed_round(u32 value, u32 ratio);
u32 GetRatioOverZero(u32 value, u32 ratio);
u32 MultiplyValueByRatio(u32 value, u32 ratio);
u32 DivideMaxHp(BattleMon *mon, u32 divisor);
u32 DivideMaxHPZeroCheck(BattleMon *mon, u32 divisor);
u32 func_ov167_021bd3a0(u32 min, u32 max);
u8 func_ov167_021bd3b8(u8 hits);
u32 func_ov167_021bd3e8(BattleMon *mon, u32 weather);
fx32 WeatherPowerMod(u32 weather, u32 moveType);
void func_ov167_021bd484(MoveConditionParam param, BattleMon *mon, BattleCondition *out);
BattleCondition func_ov167_021bd52c(u32 status);
u32 func_ov167_021bd624(BattleMon *mon);
BOOL IsBasicStatus(s32 condition);
BattleCondition func_ov167_021bd58c(u32 turns);
BattleCondition func_ov167_021bd5b0(u32 turns);
void func_ov167_021bd5d4(s32 condition, BattleMon *mon, BattleCondition *out);
u16 func_ov167_021bd658(const u16 *excluded, u16 count);
BOOL func_ov167_021bd6a4(const u16 *moves, u32 count, u16 move);
u32 CalcBaseExpGain(BattleMon *mon, s32 levelDiff);
BOOL func_ov167_021bd718(u32 value);
u32 func_ov167_021bd728(u32 value);
u8 GetNumMonsOnField(u32 battleStyle, u8 count);
BOOL func_ov167_021bd760(u32 trainerClass);
BOOL func_ov167_021bd774(u32 trainerClass);
BOOL func_ov167_021bd788(u16 trainerClass);
u32 func_ov167_021bd7b0(BtlSetup *setup);
u32 func_ov167_021bd7e4(BtlSetupTrainer *trainer, PokeParty *party);
u32 func_ov167_021bd820(u32 index, BattleParty *party);
void func_ov167_021bd874(HeapID heapId);
void func_ov167_021bd880(void);
u32 ItemGetParam(u16 item, u32 param);
u32 func_ov167_021bd894(const u8 *values);
void func_ov167_021bd8b8(u32 packed, u8 *values);
u32 func_ov167_021bd8d0(BattleMon *mon);
u8 func_ov167_021bd8e4(BtlMainModule *mainModule, BtlPokeCon *pokeCon, BattleMon *mon, u16 move);
u8 DecideMoveTargetAutoForClient(BtlMainModule *mainModule, BtlPokeCon *pokeCon, BattleMon *mon, u16 move,
                                 MATHRandContext32 *saved);
void func_ov167_021bda58(BtlClientIDList *list);
void func_ov167_021bda6c(BtlClientIDList *list, u8 clientId);
u32 func_ov167_021bda94(BtlClientIDList *list);
u32 func_ov167_021bda98(BtlClientIDList *list, u8 clientId, u32 value);

#endif // POKEBW2_BATTLE_BTL_CALC_H
