#ifndef POKEBW2_PML_PERSONAL_H
#define POKEBW2_PML_PERSONAL_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Species data

#define PERSONAL_TYPE_1 6
#define PERSONAL_TYPE_2 7
#define PERSONAL_ABILITY_1 26
#define PERSONAL_ABILITY_2 27
#define PERSONAL_ABILITY_HIDDEN 28
#define PERSONAL_SEX_RATIO 20
#define PERSONAL_HATCH_CYCLES 21
#define PERSONAL_FRIENDSHIP 22
#define PERSONAL_GROWTH_RATE 23
// The offset of the forms' sprites (not from swan)
#define PERSONAL_FORM_SPRITE_OFFSET 31
#define PERSONAL_FORM_COUNT 32
// Whether the forms only change the palette, as Arceus's do (not from swan)
// Whether the sprite may be flipped
#define PERSONAL_SPRITE_FLIP 34
#define PERSONAL_PALETTE_FORMS 35
// Not from swan: a flag that keeps the summary screen's sprite from bouncing, and the weight
#define PERSONAL_NO_BOUNCE 16
#define PERSONAL_WEIGHT 38
// Not from swan: the first of the four words of TM and HM flags, the free move tutors' flags, and the first of the
// four words of the paid move tutors' flags
#define PERSONAL_MACHINES_1 39
#define PERSONAL_TUTORS 43
#define PERSONAL_TUTOR_PAID_1 44

// A species' data, the files of ARCID_PERSONAL. PML_PersonalGetParam reads its fields by number
typedef struct {
    u8 hp;
    u8 attack;
    u8 defense;
    u8 speed;
    u8 spAttack;
    u8 spDefense;
    u8 type1;
    u8 type2;
    u8 catchRate;
    u8 stage;
    u16 evHP : 2;
    u16 evAttack : 2;
    u16 evDefense : 2;
    u16 evSpeed : 2;
    u16 evSpAttack : 2;
    u16 evSpDefense : 2;
    u16 noBounce : 1;
    u16 items[3];
    u8 sexRatio;
    u8 hatchCycles;
    u8 friendship;
    u8 growthRate;
    u8 eggGroup1;
    u8 eggGroup2;
    u8 abilities[3];
    u8 fleeRate;
    // The file of the first other form, or 0, and the offset of the forms' sprites
    u16 formFile;
    u16 formSpriteOffset;
    u8 formCount;
    u8 color : 6;
    u8 spriteFlip : 1;
    u8 paletteForms : 1;
    u16 baseExp;
    u16 height;
    u16 weight;
    u32 machines[4];
    u32 tutors[5];
} PersonalData;

// Opens the personal archives, once
void PML_PersonalArcInit(HeapID heapId);
void PML_PersonalFetchBW2(u16 species, u16 form, PersonalData *dest);
void PML_PersonalFetchBW1(u16 species, u16 form, PersonalData *dest);
u32 PML_PersonalGetParamSingle(u16 species, u16 form, u32 param);
// The same from Black and White's personal data, which the musical's sprites still follow
u32 PML_PersonalGetParamSingleBW1(u16 species, u16 form, u32 param);
void *PML_PersonalLoad(u16 species, u16 form, HeapID heapId);
u32 PML_PersonalGetParam(void *personal, u32 param);
void PML_PersonalFree(void *personal);
// The moves a species learns by level, pairs of move and level that end with two 0xffff
void PML_LearnsetLvUpLoad(u16 species, u16 form, void *dest);
// The experience a Pokémon of the species needs for the level
u32 PML_UtilGetPkmLvExp(u16 species, u16 form, u16 level);
// The regional Pokédex numbers, by national number, and, if count is not NULL, how many species have one. The caller
// frees the table
u16 *PML_PersonalLoadRegionalDexTable(HeapID heapId, u16 *count);

ArcTool *loadEvolutionFile(HeapID heapId);
BOOL getEvolutionInfoParam(ArcTool *evoFile, u16 species, u16 form, u16 index);

#endif // POKEBW2_PML_PERSONAL_H
