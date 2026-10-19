#ifndef POKEBW2_BATTLE_BTL_MAIN_H
#define POKEBW2_BATTLE_BTL_MAIN_H

#include "types.h"
#include "battle/btl_calc.h"
#include "battle/btl_rec.h"
#include "battle/btl_setup.h"
#include "constants/battle.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// What IsAdjacentOpponent looks up for a position
struct AdjacentOpponentData {
    u16 count1;
    u16 count2;
    u8 list1[3];
    u8 list2[3];
};

// The battle positions, 0 to 5. Even positions are on one side and odd positions on the other
#define BTL_POS_MAX 6

// Layout reconstructed from the party accessors in the game code.
struct BattleParty {
    BattleMon *mons[6];
    u8 count;
    u8 unk19[3];
};

// The parties of the four clients and every BattleMon by mon ID, 0xe8 bytes as the main module holds two
struct BtlPokeCon {
    BtlMainModule *mainModule;
    BattleParty parties[4];
    // The parties the clients' Pokémon come from
    PokeParty *srcParties[4];
    BattleMon *mons[24];
    u32 unkE4;
};

// A buffer of the main module that func_ov167_0219e314 allocates
typedef struct {
    u8 unk00[8];
    s8 unk08;
    u8 unk09[0xb];
} BtlMainUnk478;

// A step of the main module's setup or cleanup, run until it returns TRUE
typedef BOOL (*BtlMainSeqFunc)(u32 *state, BtlMainModule *mainModule);

typedef struct {
    BtlMainSeqFunc func;
    BtlMainSeqFunc nextFunc;
    BtlMainModule *mainModule;
    u32 state;
} BtlMainSeq;

// What the clients of a link battle exchange before it starts
struct BtlMainSyncData {
    MATHRandContext32 rand;
    u16 unk18;
    u16 unk1A;
    u16 unk1C;
    u8 unk1E;
    u8 unk1F_0 : 4;
    u8 unk1F_4 : 4;
};

// A client's trainer, 0x28 bytes
typedef struct {
    PlayerInfo *playerInfo;
    StrBuf *name;
    u16 unk08;
    u16 unk0A;
    u32 unk0C;
    u16 unk10[4];
    PMSData unk18;
    PMSData unk20;
} BtlTrainerData;

// A choice the player is offered in a Pokestar Studios scene
typedef struct {
    // -1 for none
    s16 msgId;
    s16 msgId2;
    s16 unk04;
    s16 unk06;
    s16 points;
    s16 unk0A;
} BtlStudioChoice;

// One entry of a Pokestar Studios movie's script
typedef struct {
    s16 unk00;
    s16 unk02;
    s16 unk04;
    s16 unk06;
    s16 unk08;
    s16 unk0A;
    // The messages of two kinds, by step, which the cut-ins' message command picks between
    s16 messages[2][10];
    // The script event each step waits for, -1 for none
    s16 events[10];
    BtlStudioChoice choices[4];
} BtlStudioScene;

typedef struct {
    s16 turn;
    s16 value;
} BtlStudioTurnEvent;

// The rules of a scripted battle, which seem to be the Pokestar Studios movies
typedef struct {
    s16 movie;
    u8 unk02[2];
    s16 unk04;
    s16 unk06;
    // Which of the studio's message files the movie uses
    s16 unk08;
    s16 turnLimit;
    s16 unk0C;
    s16 unk0E;
    s16 rule;
    s16 species1;
    s16 species2;
    s16 unk16;
    u8 unk18[0xc];
    // btlv_effect.c passes it to the field
    s16 unk24;
    u8 unk26[0xa];
    BtlStudioTurnEvent turnEvents[3];
    u8 unk3C[0x6c];
    // Messages by turn
    s16 unkA8[20];
    s16 unkD0[20];
    BtlStudioScene scenes[];
} BtlScriptedRules;

struct BtlMainModule {
    BtlSetup *setup;
    BtlvCore *viewCore;
    BtlServer *server;
    BtlServer *unk0C;
    BtlClient *clients[4];
    BtlTrainerData trainers[4];
    PlayerInfo *unkC0;
    u8 unkC4[4];
    BtlPokeCon pokeCons[2];
    PokeParty *unk298[4];
    PokeParty *unk2A8[4];
    PokeParty *unk2B8;
    BtlField *field;
    PartyPkm *unk2C0;
    void *unk2C4;
    BtlRecReader recReader;
    void *unk3E0[4];
    MATHRandContext32 rand;
    // What the clients of a link battle agree on before it starts
    BtlMainSyncData syncData;
    u8 posClientIds[6];
    u8 unk42e[2];
    // The prize money, which func_ov167_0219ca78 doubles and finalizes
    u32 prizeMoney;
    u32 money;
    u32 unk438;
    u32 unk43C;
    u16 unk440;
    u16 unk442;
    u32 result;
    BtlClientIDList unk448;
    BtlMainSeq seq;
    s32 unk460;
    BOOL (*mainFunc)(BtlMainModule *mainModule);
    u16 heapId;
    u8 unk46A;
    u8 clientCount;
    u8 playerClientId;
    u8 unk46D;
    u8 unk46E;
    u8 unk46F;
    u8 unk470;
    u8 unk471;
    u8 unk472;
    u8 unk473_0 : 1;
    u8 unk473_1 : 1;
    u8 unk473_2 : 1;
    u8 unk473_3 : 1;
    u8 unk473_4 : 1;
    u8 unk473_5 : 1;
    u8 unk473_6 : 1;
    u8 unk473_7 : 1;
    BtlScriptedRules *unk474;
    BtlMainUnk478 *unk478;
    PokewoodCutin *cutin;
};

// Swan's names for these two take the main module, whose first field points to the BtlSetup
u32 BtlSetup_GetBattleStyle(BtlMainModule *mainModule);
u32 func_ov167_0219bd88(BtlMainModule *mainModule);
u8 func_ov167_0219bd98(BtlMainModule *mainModule);
u32 BtlSetup_GetBattleType(BtlMainModule *mainModule);
u8 func_ov167_0219bedc(BtlMainModule *mainModule);
u8 func_ov167_0219bee4(BtlMainModule *mainModule);
BOOL func_ov167_0219beec(BtlMainModule *mainModule);
u16 func_ov167_0219bf00(BtlMainModule *mainModule);
u16 func_ov167_0219bf08(BtlMainModule *mainModule);
u16 func_ov167_0219bf14(BtlMainModule *mainModule);
u32 BtlSetup_IsBattleType(BtlMainModule *mainModule, u32 flag);
u8 func_ov167_0219c980(BtlMainModule *mainModule);
void func_ov167_0219c9fc(BtlMainModule *mainModule, u8 pos);
void func_ov167_0219ca48(BtlMainModule *mainModule, u32 result);
u32 func_ov167_0219ca58(BtlMainModule *mainModule);
void func_ov167_0219cac0(BtlMainModule *mainModule, u32 money);
void func_ov167_0219cb10(BtlMainModule *mainModule);
void func_ov167_0219dad0(BtlMainModule *mainModule, u32 record);
BOOL func_ov167_0219df10(BtlMainModule *mainModule);
BOOL func_ov167_0219df50(BtlMainModule *mainModule);
u32 func_ov167_0219c988(BtlMainModule *mainModule);
BOOL IsSwitchMode(BtlMainModule *mainModule);
u32 GetValidPosMax(BtlMainModule *mainModule);
u32 func_ov167_0219be8c(BtlMainModule *mainModule);
BOOL func_ov167_0219bebc(BtlMainModule *mainModule, u32 pos);
u32 GetRunMode(BtlMainModule *mainModule);
BtlFieldSituation *GetFieldEffectData(BtlMainModule *mainModule);

// The position at index on the same side as pos
u8 GetPosOnSameSide(u8 pos, u8 index);
BOOL DoesClientExist(BtlMainModule *mainModule, u8 clientId);
u8 GetClientSide(BtlMainModule *mainModule, u8 clientId);
BOOL AreClientsOnOppositeSides(BtlMainModule *mainModule, u8 clientId1, u8 clientId2);
u8 BattlePosToClientID(BtlMainModule *mainModule, u8 pos);
u8 MonIDToClientID(u8 monId);
u8 func_ov167_0219c458(BtlMainModule *mainModule, u8 clientId, u8 slot);
s32 func_ov167_0219d140(BtlPokeCon *pokeCon, u8 clientId, u8 monId);
u8 MonIDToBattlePos(BtlMainModule *mainModule, BtlPokeCon *pokeCon, u8 monId);
u8 func_ov167_0219bfe4(BtlMainModule *mainModule, u16 posFlags, u8 *out);
u8 func_ov167_0219c4bc(BtlMainModule *mainModule, u8 pos, u8 index);
u8 func_ov167_0219c5a4(BtlMainModule *mainModule, BtlPokeCon *pokeCon, u16 posFlags, u8 *monIds);
u8 func_ov167_0219c648(u8 monId);
u8 func_ov167_0219c658(BtlMainModule *mainModule, u8 pos);
u8 GetPlayerClientID(BtlMainModule *mainModule);
u8 func_ov167_0219c86c(BtlMainModule *mainModule);
u8 func_ov167_0219c87c(BtlMainModule *mainModule, u8 clientId);
BOOL IsAllyClientID(u8 clientId1, u8 clientId2);
u8 func_ov167_0219c8b8(BtlMainModule *mainModule, u8 other);
u32 func_ov167_0219c8d0(BtlMainModule *mainModule, u8 clientId, u8 other);
void BattleClient_SubItem(BtlMainModule *mainModule, u8 clientId, u16 item);
void BattleClient_AddItem(BtlMainModule *mainModule, u8 clientId, u16 item);
void ChangeFriendshipWhenFainted(BtlMainModule *mainModule, BattleMon *mon, BOOL reason);
void ChangeFriendship(BtlMainModule *mainModule, BattleMon *mon, u32 reason);
u8 func_ov167_0219c650(BtlMainModule *mainModule, u8 pos);
BattleMon *func_ov167_0219d188(BtlPokeCon *pokeCon, u8 pos);
BattleMon *GetClientMonData(BtlPokeCon *pokeCon, u8 clientId, u8 monId);
BattleMon *GetPokeParam(BtlPokeCon *pokeCon, u8 monId);
const BattleMon *GetPokeParamConst(const BtlPokeCon *pokeCon, u8 monId);
// Underlying battle-type-dependent count used by GetClientBattlerCount.
u8 func_ov167_0219a180(BtlMainModule *mainModule, u8 clientId);
// How many of the client's Pokemon are in battle at once. They come first in the party, so this is also the index of
// the first Pokemon waiting to switch in
u8 GetClientBattlerCount(BtlMainModule *mainModule, u8 clientId);
BOOL IsAllyMonID(u8 monId1, u8 monId2);
// The side of the battle a mon ID is on: IDs from 12 are the second side's
typedef enum { BTL_SIDE_1ST, BTL_SIDE_2ND } BtlSide;
u8 GetSideFromMonID(u8 monId);
u8 GetSideFromOpposingMonID(u8 monId);
u8 func_ov167_0219d338(u8 side);
// Returns the opponent-position lookup entry for a battle position.
const AdjacentOpponentData *func_ov167_0219d2bc(u8 pos);
// Whether pos2 is an opponent next to pos1 in a triple battle
BOOL IsAdjacentOpponent(u8 pos1, u8 pos2);
BattleParty *GetClientParty(BtlPokeCon *pokeCon, u32 clientId);
BattleParty *GetPartyData(BtlPokeCon *pokeCon, u32 clientId);

s32 FindPartyMon(const BattleParty *party, BattleMon *mon);
s32 GetPartyPkmnEligibleForBattle(PokeParty *party);
void AddBattleMonToParty(BattleParty *party, BattleMon *mon);
void func_ov167_0219d434(BattleParty *party);
void func_ov167_0219d454(BattleParty *party);
u8 func_ov167_0219d4b8(BattleParty *party, s32 index);
BattleMon *func_ov167_0219d4e4(BattleParty *party, u8 index);
void func_ov167_0219d504(BattleParty *party, u32 first, u32 second);
void func_ov167_0219d518(BattleParty *party, u8 index);
void func_ov167_0219d544(BattleParty *party, u32 pos, BattleMon **oldOut, BattleMon **newOut);
s32 func_ov167_0219d5b0(const BattleParty *party, u32 monId);
BattleMon *func_ov167_0219d5dc(const BattleParty *party);
u8 func_ov167_0219d38c(u32 pos);
u32 func_ov167_0219d3a4(u32 pos);
BattleMon *GetBattleMonFromParty(BattleParty *party, u8 index);
u8 GetNumMonsInParty(BattleParty *party);
u8 GetAlivePartyCount(BattleParty *party);

BOOL func_ov167_0219a004(BtlSetup *setup);
void func_ov167_0219a034(BtlMainModule *mainModule, BtlSetup *setup);
void func_ov167_0219a0c4(BtlMainModule *mainModule, BtlSetup *setup);
void func_ov167_0219bd40(BtlMainModule *mainModule);
BOOL func_ov167_0219bd5c(BtlMainModule *mainModule);
s32 func_ov167_0219bde0(BtlMainModule *mainModule);
void func_ov167_0219bdf0(BtlMainModule *mainModule);
BOOL func_ov167_0219bdfc(BtlMainModule *mainModule);
void *func_ov167_0219be48(BtlMainModule *mainModule);
PlayerInfo *func_ov167_0219bf68(BtlMainModule *mainModule);
BOOL func_ov167_0219bf70(BtlMainModule *mainModule, BattleMon *mon);
u32 func_ov167_0219bf88(BtlMainModule *mainModule);
GameData *func_ov167_0219bf98(BtlMainModule *mainModule);
void func_ov167_0219bfa0(BtlMainModule *mainModule, u8 clientId, BattleMon *mon);
u8 func_ov167_0219c054(BtlMainModule *mainModule, u8 type, u8 pos, u8 *out);
u8 func_ov167_0219c0a0(BtlMainModule *mainModule, u8 type, u8 pos, u8 *out);
u8 func_ov167_0219c158(BtlMainModule *mainModule, u8 type, u8 pos, u8 *out);
u32 func_ov167_0219c3e4(BtlMainModule *mainModule, u8 clientId);
BOOL func_ov167_0219c43c(BtlMainModule *mainModule, u8 side);
u8 func_ov167_0219c48c(u32 battleStyle, u8 pos, u8 index);
u8 func_ov167_0219c4c8(u32 battleStyle, u8 pos);
u8 func_ov167_0219c51c(BtlMainModule *mainModule, u8 pos);
u8 func_ov167_0219c62c(BtlMainModule *mainModule, BtlPokeCon *pokeCon, u8 monId);
void func_ov167_0219c694(BtlMainModule *mainModule, u8 pos, u8 *clientId, u8 *index);
u8 func_ov167_0219c6dc(BtlMainModule *mainModule, u8 pos);
u8 func_ov167_0219c744(BtlMainModule *mainModule, u8 index);
BOOL func_ov167_0219c7a4(BtlMainModule *mainModule);
PokeParty *func_ov167_0219c7c8(BtlMainModule *mainModule, u8 clientId);
PokeParty *func_ov167_0219c7e8(BtlMainModule *mainModule, u8 clientId);
u8 func_ov167_0219c82c(BtlMainModule *mainModule, u8 clientId);
u8 func_ov167_0219c850(BtlMainModule *mainModule);
BagSave *func_ov167_0219c954(BtlMainModule *mainModule);
void *func_ov167_0219c95c(BtlMainModule *mainModule);
BOOL func_ov167_0219c964(BtlMainModule *mainModule);
u32 func_ov167_0219c99c(BtlMainModule *mainModule);
u8 func_ov167_0219c9b0(BtlMainModule *mainModule);
u32 func_ov167_0219c9c0(BtlMainModule *mainModule);
void func_ov167_0219c9d4(BtlMainModule *mainModule);
u32 func_ov167_0219c9e0(BtlMainModule *mainModule);
u32 ReturnZero(BtlMainModule *mainModule, u32 flag);
void func_ov167_0219ca60(BtlMainModule *mainModule);
u32 func_ov167_0219ca78(BtlMainModule *mainModule);
u32 func_ov167_0219caec(BtlMainModule *mainModule);
u32 func_ov167_0219cb20(BtlMainModule *mainModule);
void func_ov167_0219cb7c(BtlMainModule *mainModule);
void func_ov167_0219cc34(BtlMainModule *mainModule, u8 monId);
BattleMon *GetIllusionDisguise(BtlMainModule *mainModule, BtlPokeCon *pokeCon, BattleMon *mon);
void func_ov167_0219ccbc(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u32 unkE4);
void func_ov167_0219cd00(BtlPokeCon *pokeCon);
void func_ov167_0219cd3c(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u8 clientId);
void func_ov167_0219cf50(BtlPokeCon *pokeCon);
void func_ov167_0219cf78(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u8 clientId, BOOL keepBaseForm);
void func_ov167_0219d07c(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u8 clientId);
PokeParty *func_ov167_0219d138(BtlPokeCon *pokeCon, u8 clientId);
BattleMon *func_ov167_0219d180(BtlPokeCon *pokeCon, u8 pos);
BattleMon *func_ov167_0219d1e8(BtlPokeCon *pokeCon, u8 clientId, u8 index);
BOOL func_ov167_0219d228(BtlMainModule *mainModule, u32 index, void **out);
u8 func_ov167_0219d258(BtlMainModule *mainModule, u8 clientId);
u8 func_ov167_0219d29c(BtlMainModule *mainModule, u8 clientId);
BOOL func_ov167_0219d2cc(u8 pos);
BOOL func_ov167_0219d2dc(u8 pos, u8 *out);
u8 func_ov167_0219d3bc(u8 pos);
u32 func_ov167_0219d3e0(BtlMainModule *mainModule);
u8 func_ov167_0219d3f8(BtlPokeCon *pokeCon, u8 clientId);
void func_ov167_0219d404(BtlMainModule *mainModule, u8 clientId, u8 value);
void func_ov167_0219d604(BtlMainModule *mainModule, BattleParty *party, u8 clientId);
void func_ov167_0219d6ac(BtlMainModule *mainModule);
void func_ov167_0219d6dc(BtlMainModule *mainModule);
void func_ov167_0219d72c(BtlTrainerData *trainer, HeapID heapId, PlayerInfo *src);
BOOL func_ov167_0219d888(BtlMainModule *mainModule, u8 clientId);
u16 func_ov167_0219d89c(BtlMainModule *mainModule, u8 clientId, u8 index);
StrBuf *func_ov167_0219d8c4(BtlMainModule *mainModule, u8 clientId, u32 *trainerClass);
u32 func_ov167_0219d8d4(BtlMainModule *mainModule, u8 clientId);
u16 func_ov167_0219d91c(BtlMainModule *mainModule, u8 clientId);
u32 func_ov167_0219d938(BtlMainModule *mainModule, u8 clientId);
PMSData *func_ov167_0219d944(BtlMainModule *mainModule, u8 clientId, u32 which);
PlayerInfo *func_ov167_0219d97c(BtlMainModule *mainModule, u8 clientId);
PlayerInfo *func_ov167_0219d998(BtlMainModule *mainModule);
BtlField *func_ov167_0219d9a8(BtlMainModule *mainModule);
void func_ov167_0219d9b0(BtlMainModule *mainModule);
void func_ov167_0219d9e8(BtlMainModule *mainModule);
void func_ov167_0219da44(BtlMainModule *mainModule, u8 clientId, const PokeParty *party);
PokeParty *func_ov167_0219da94(BtlMainModule *mainModule, u8 clientId, u32 useSecond);
void func_ov167_0219dae8(BtlMainModule *mainModule, u32 record, u32 value);
void *func_ov167_0219db00(BtlMainModule *mainModule);
BOOL func_ov167_0219db08(BtlMainModule *mainModule);
BOOL func_ov167_0219db28(BtlMainModule *mainModule);
void func_ov167_0219db64(BtlMainModule *mainModule, BattleMon *mon);
void func_ov167_0219db7c(BtlMainModule *mainModule, BattleMon *mon, u16 item);
void func_ov167_0219dc00(BtlMainModule *mainModule, BattleMon *mon);
void func_ov167_0219dc10(BtlMainModule *mainModule);
BOOL func_ov167_0219de6c(BtlMainModule *mainModule);
u32 func_ov167_0219de84(BtlMainModule *mainModule);
u16 func_ov167_0219defc(BtlMainModule *mainModule);
u16 func_ov167_0219df08(BtlMainModule *mainModule);
void func_ov167_0219df80(BtlMainModule *mainModule, u8 clientId, const PokeParty *party);
void func_ov167_0219dfa8(BtlMainModule *mainModule, u8 clientId, const PlayerInfo *src);
void func_ov167_0219dfd0(BtlMainModule *mainModule, u8 clientId);
void func_ov167_0219dff8(BtlMainModule *mainModule);
u8 func_ov167_0219e048(u8 playerClientId, u8 clientId);
void func_ov167_0219e074(BtlMainModule *mainModule, u32 arg1);
void func_ov167_0219e130(BtlMainModule *mainModule);
BtlServerFlow *func_ov167_0219e158(BtlMainModule *mainModule);
void func_ov167_0219e164(BtlSetup *setup);
void func_ov167_0219e1b0(BtlMainModule *mainModule);
u32 func_ov167_0219e300(BtlMainModule *mainModule);
BtlSetup *func_ov167_0219e30c(BtlMainModule *mainModule);
BtlSetup *func_ov167_0219e310(BtlMainModule *mainModule);
void func_ov167_0219e314(BtlMainModule *mainModule, u8 arg1);
void func_ov167_0219e378(BtlMainModule *mainModule);
BtlScriptedRules *func_ov167_0219e39c(BtlMainModule *mainModule);
void *func_ov167_0219e3ac(BtlMainModule *mainModule);
PokewoodCutin *func_ov167_0219e3bc(BtlMainModule *mainModule);
void func_ov167_0219e3c8(void *data);

BOOL func_ov167_021998c0(GameProc *proc, u32 *state, void *param, void *work);
BOOL func_ov167_02199c08(GameProc *proc, u32 *state, void *param, void *work);
BOOL func_ov167_02199ca0(BtlMainModule *mainModule);
BOOL func_ov167_02199cd4(GameProc *proc, u32 *state, void *param, void *work);
void func_ov167_02199ec0(BtlMainSeq *seq, BtlMainModule *mainModule, BtlSetup *setup);
void func_ov167_02199ff0(BtlMainSeq *seq, BtlMainModule *mainModule, BtlSetup *setup);
void func_ov167_0219a140(BtlMainModule *mainModule, u8 clientId);
void func_ov167_0219a1e8(BtlMainModule *mainModule, BtlSetup *setup);
void func_ov167_0219a228(BtlMainModule *mainModule, BtlSetup *setup);
void func_ov167_0219a25c(BtlMainModule *mainModule, BtlSetup *setup, u32 arg2);
BOOL func_ov167_0219a298(u32 *state, BtlMainModule *mainModule);
BOOL func_ov167_0219a3f4(u32 *state, BtlMainModule *mainModule);
BOOL func_ov167_0219a448(u32 *state, BtlMainModule *mainModule);
BOOL func_ov167_0219a5bc(u32 *state, BtlMainModule *mainModule);
BOOL func_ov167_0219a848(u32 *state, BtlMainModule *mainModule);
BOOL func_ov167_0219a9cc(u32 *state, BtlMainModule *mainModule);
BOOL func_ov167_0219ab44(u32 *state, BtlMainModule *mainModule);
BOOL func_ov167_0219abbc(u32 *state, BtlMainModule *mainModule);
BOOL func_ov167_0219ac80(u32 *state, BtlMainModule *mainModule);
BOOL func_ov167_0219ad10(u32 *state, BtlMainModule *mainModule);
BOOL func_ov167_0219ada0(BtlMainModule *mainModule, s32 *state);
BOOL func_ov167_0219af50(BtlMainModule *mainModule, s32 *state);
BOOL func_ov167_0219b160(BtlMainModule *mainModule, s32 *state);
BOOL func_ov167_0219b3a8(BtlMainModule *mainModule, s32 *state);
BOOL func_ov167_0219b4ac(BtlMainModule *mainModule, s32 *state);
BOOL func_ov167_0219b610(BtlMainModule *mainModule, s32 *state);
BOOL func_ov167_0219b868(BtlMainModule *mainModule, s32 *state);
BOOL func_ov167_0219b9d4(BtlMainModule *mainModule, s32 *state);
BOOL func_ov167_0219bb54(BtlMainModule *mainModule, s32 *state);
BOOL func_ov167_0219bbec(BtlMainModule *mainModule);
BOOL func_ov167_0219bc2c(BtlMainModule *mainModule);
BOOL func_ov167_0219bcd0(BtlMainModule *mainModule);
void func_ov167_0219d794(BtlTrainerData *trainer, const BtlSetupTrainer *src);
void func_ov167_0219d808(BtlTrainerData *trainer, const BtlCommTrainerData *src);

#endif // POKEBW2_BATTLE_BTL_MAIN_H
