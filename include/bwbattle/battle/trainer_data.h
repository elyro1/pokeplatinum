#ifndef POKEBW2_BATTLE_TRAINER_DATA_H
#define POKEBW2_BATTLE_TRAINER_DATA_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "struct_decls.h"

// The trainers' records, parties and messages, and the trainer classes' tables (tr_tool.c). Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except the structs, TRAINER_PARAM_*, PARTY_*,
// TRAINER_ID_BATTLE_INST_START, TRAINER_SPRITE_BACK and TRAINER_CLASS_SPECIAL_COUNT

// What each party entry holds besides the Pokémon itself, OR'ed together, as tools/data/trainer_data.py packs it
#define PARTY_MOVES 1
#define PARTY_ITEMS 2

// The first ID of the Battle Institute's and similar facilities' trainers, whose records and messages are kept apart
#define TRAINER_ID_BATTLE_INST_START 0x500

// A flag of GetTrSpriteBaseDatID: the back sprite rather than the front one
#define TRAINER_SPRITE_BACK 1

// The number of classes with a battle BGM or pedestal of their own, and what GetTrainerClassSpecialId returns for
// the others
#define TRAINER_CLASS_SPECIAL_COUNT 29

// A trainer's record in ARCID_TRDATA, packed from data/trainers/ by tools/data/trainer_data.py
typedef struct {
    u8 partyKind; // PARTY_*
    u8 trainerClass;
    u8 battleStyle; // BTL_STYLE_*
    u8 pokeCount;
    u16 items[4];
    u32 aiFlags;
    u8 heals : 1;
    // The prize money's multiplier
    u8 money;
    u16 rewardItem;
} TrainerData;

// A field of a trainer's record, for TrainerData_GetParam
enum {
    TRAINER_PARAM_PARTY_KIND,
    TRAINER_PARAM_CLASS,
    TRAINER_PARAM_BATTLE_STYLE,
    TRAINER_PARAM_POKE_COUNT,
    TRAINER_PARAM_ITEM_1,
    TRAINER_PARAM_ITEM_2,
    TRAINER_PARAM_ITEM_3,
    TRAINER_PARAM_ITEM_4,
    TRAINER_PARAM_AI_FLAGS,
    TRAINER_PARAM_HEALS,
    TRAINER_PARAM_MONEY,
    TRAINER_PARAM_REWARD_ITEM,
};

// A Pokémon of a trainer's party in ARCID_TRPOKE, in one of four layouts by the party's kind
typedef struct {
    // 0 to 255 for IVs of 0 to 31
    u8 difficulty;
    // The sex in the low nibble (1 male, 2 female, 0 either) and the ability in the high one (1 or 2 the first or
    // second, 3 the hidden one, 0 either)
    u8 genderAbility;
    u16 level;
    u16 species;
    u16 form;
} TrainerPoke;

typedef struct {
    TrainerPoke base;
    u16 moves[4];
} TrainerPokeMoves;

typedef struct {
    TrainerPoke base;
    u16 item;
} TrainerPokeItem;

typedef struct {
    TrainerPoke base;
    u16 item;
    u16 moves[4];
} TrainerPokeItemMoves;

// A field of a trainer's record (TRAINER_PARAM_*)
u32 TrainerData_GetParam(int trainerId, u32 param);
BOOL TrainerMsg_CheckExists(int trainerId, u32 msgId, HeapID heapId);
// Loads the trainer's message of the type, or clears strbuf if it has none. Battle institution trainers only have types
// 0 and 1, and any other type leaves strbuf as it was
void TrainerMsg_Load(int trainerId, u32 msgId, StrBuf *strbuf, HeapID heapId);
// 0 for a male class, 1 for a female one
u8 TrainerClass_GetSex(u32 trainerClass);
// The class's sprite in ARCID_TRSPRITE_FRONT, or with TRAINER_SPRITE_BACK in flags in ARCID_TRSPRITE_BACK
u8 GetTrSpriteBaseDatID(u32 trainerClass, u32 flags);
// The BGM played when the class's trainers spot the player
u16 GetTrainerClassEyeBGMID(u32 trainerClass);
// The class's index among the classes with a battle BGM or pedestal of their own, or TRAINER_CLASS_SPECIAL_COUNT
u8 GetTrainerClassSpecialId(u32 trainerClass);
u32 GetTrainerClassBGMGroupId(u32 trainerClass);
u32 GetTrainerClassBattlePedestal(u32 trainerClass);
// The class's own battle background, or bg if it has none
u32 CheckOverridenTrainerBattleBG(u32 trainerClass, u32 bg);
void TrainerUtil_LoadTrainer(GameData *gameData, int trainerId, BtlSetupTrainer *trainer, HeapID heapId);
void TrainerUtil_LoadParty(int trainerId, PokeParty *party, HeapID heapId);

#endif // POKEBW2_BATTLE_TRAINER_DATA_H
