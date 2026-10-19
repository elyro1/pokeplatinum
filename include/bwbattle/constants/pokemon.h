#ifndef POKEBW2_CONSTANTS_POKEMON_H
#define POKEBW2_CONSTANTS_POKEMON_H

// Genders, as BATTLEMON_GENDER gives them. The AI scripts tell which is which
#define GENDER_MALE 0
#define GENDER_FEMALE 1
// Not from swan: a genderless Pokémon's, which its sprite files treat as male (pokegra.c)
#define GENDER_UNKNOWN 2

// SPECIES_EGG, the species ID of an egg, follows the national Pokédex (data/constants/species.txt)
#include "constants/species.h"

// Fields of a Pokémon, which PokeParty_GetParam and PokeParty_SetParam read and write. Names from swan's PkmField
#define PKM_PARAM_PID 0x0
// Not from swan: whether the party and the box data are decrypted, and the checksum
#define PKM_PARAM_PARTY_DECRYPTED 0x1
#define PKM_PARAM_BOX_DECRYPTED 0x2
// Whether the Pokémon's data is broken, making it a bad egg (our name)
#define PKM_PARAM_BAD_EGG 0x3
#define PKM_PARAM_CHECKSUM 0x4
#define PKM_PARAM_SPECIES 0x5
#define PKM_PARAM_ITEM 0x6
#define PKM_PARAM_ID 0x7
#define PKM_PARAM_EXP 0x8
// A friendship, or an egg's remaining steps
#define PKM_PARAM_HAPPINESS 0x9
#define PKM_PARAM_ABILITY 0xa
#define PKM_PARAM_MARKINGS 0xb
#define PKM_PARAM_REGION 0xc
// The effort values, HP to special defense
#define PKM_PARAM_EV_HP 0xd
#define PKM_PARAM_EV_ATTACK 0xe
#define PKM_PARAM_EV_DEFENSE 0xf
#define PKM_PARAM_EV_SPEED 0x10
#define PKM_PARAM_EV_SP_ATTACK 0x11
#define PKM_PARAM_EV_SP_DEFENSE 0x12
// The first ribbon of each group of ribbons, each followed by the field after the group
// Not from swan: the contest conditions, cool to sheen
#define PKM_PARAM_CONTEST_COOL 0x13
#define PKM_PARAM_CONTEST_BEAUTY 0x14
#define PKM_PARAM_CONTEST_CUTE 0x15
#define PKM_PARAM_CONTEST_SMART 0x16
#define PKM_PARAM_CONTEST_TOUGH 0x17
#define PKM_PARAM_CONTEST_SHEEN 0x18
#define PKM_PARAM_RIBBON_CHAMPION_SINNOH 0x19
#define PKM_PARAM_MOVE1 0x36
#define PKM_PARAM_MOVE2 0x37
#define PKM_PARAM_MOVE3 0x38
#define PKM_PARAM_MOVE4 0x39
#define PKM_PARAM_MOVE1_PP 0x3a
#define PKM_PARAM_MOVE2_PP 0x3b
#define PKM_PARAM_MOVE3_PP 0x3c
#define PKM_PARAM_MOVE4_PP 0x3d
#define PKM_PARAM_MOVE1_PP_UP 0x3e
#define PKM_PARAM_MOVE2_PP_UP 0x3f
#define PKM_PARAM_MOVE3_PP_UP 0x40
#define PKM_PARAM_MOVE4_PP_UP 0x41
#define PKM_PARAM_MOVE1_MAX_PP 0x42
#define PKM_PARAM_MOVE2_MAX_PP 0x43
#define PKM_PARAM_MOVE3_MAX_PP 0x44
#define PKM_PARAM_MOVE4_MAX_PP 0x45
// Not from swan: the individual values, HP to special defense
#define PKM_PARAM_IV_HP 0x46
#define PKM_PARAM_IV_ATTACK 0x47
#define PKM_PARAM_IV_DEFENSE 0x48
#define PKM_PARAM_IV_SPEED 0x49
#define PKM_PARAM_IV_SP_ATTACK 0x4a
#define PKM_PARAM_IV_SP_DEFENSE 0x4b
#define PKM_PARAM_IS_EGG 0x4c
#define PKM_PARAM_RIBBON_G3_COOL 0x4d
#define PKM_PARAM_FATEFUL_ENCOUNTER 0x6d
#define PKM_PARAM_SEX 0x6e
#define PKM_PARAM_FORM 0x6f
// Not from swan
#define PKM_PARAM_NATURE 0x70
// Whether the Pokémon has its hidden ability
#define PKM_PARAM_HIDDEN_ABILITY 0x71
#define PKM_PARAM_UNK72 0x72
// The nickname, copied to or from a StrBuf
#define PKM_PARAM_NICKNAME 0x73
// The nickname, copied to or from a u16 array
#define PKM_PARAM_NICKNAME_RAW 0x74
// Not from swan
#define PKM_PARAM_IS_NICKNAMED 0x75
#define PKM_PARAM_UNK76 0x76
// The game the Pokémon was caught in
#define PKM_PARAM_ORIGIN_GAME 0x77
#define PKM_PARAM_RIBBON_G4_COOL 0x78
#define PKM_PARAM_OT_NAME 0x8d
// The original trainer's name, copied to or from a u16 array (not from swan)
#define PKM_PARAM_OT_NAME_RAW 0x8e
// Not from swan: the dates the Pokémon was met as an egg and as itself, each two digits of the year, the month and
// the day
#define PKM_PARAM_EGG_YEAR 0x8f
#define PKM_PARAM_EGG_MONTH 0x90
#define PKM_PARAM_EGG_DAY 0x91
#define PKM_PARAM_MET_YEAR 0x92
#define PKM_PARAM_MET_MONTH 0x93
#define PKM_PARAM_MET_DAY 0x94
// Where the Pokémon was met as an egg and as itself, and the level (not from swan)
#define PKM_PARAM_EGG_LOCATION 0x95
#define PKM_PARAM_MET_LOCATION 0x96
#define PKM_PARAM_MET_LEVEL 0x99
#define PKM_PARAM_OT_GENDER 0x9a
#define PKM_PARAM_UNK9B 0x9b
#define PKM_PARAM_UNK9C 0x9c
#define PKM_PARAM_POKERUS 0x97
#define PKM_PARAM_POKEBALL 0x98
#define PKM_PARAM_STATUS 0x9d
#define PKM_PARAM_LEVEL 0x9e
#define PKM_PARAM_UNK9F 0x9f
#define PKM_PARAM_HP 0xa0
#define PKM_PARAM_MAX_HP 0xa1
#define PKM_PARAM_ATTACK 0xa2
#define PKM_PARAM_DEFENSE 0xa3
#define PKM_PARAM_SPEED 0xa4
#define PKM_PARAM_SP_ATTACK 0xa5
#define PKM_PARAM_SP_DEFENSE 0xa6
// Whether the Pokémon's name is followed by its sex, which isn't for Nidoran (our name)
#define PKM_PARAM_SHOW_SEX 0xad
// The mail the Pokémon holds, copied from a MailData
#define PKM_PARAM_MAIL 0xa7
#define PKM_PARAM_UNKA8 0xa8
// Whether there is a Pokémon in the slot
#define PKM_PARAM_SPECIES_VALID 0xa9
// Whether it is an egg or a bad egg, which PKM_PARAM_IS_EGG returns as well (not from swan)
#define PKM_PARAM_EGG_OR_BAD_EGG 0xaa
// The species, or SPECIES_EGG for an egg
#define PKM_PARAM_LEGAL_SPECIES 0xab
// The individual values packed in a word
#define PKM_PARAM_IVS_ALL 0xac
// FALSE for a Nidoran without a nickname, whose name already shows its sex
#define PKM_PARAM_NIDORAN_NICKNAME 0xad
#define PKM_PARAM_TYPE1 0xae
#define PKM_PARAM_TYPE2 0xaf
// Not from swan: the nickname, set from a StrBuf and from a u16 array without changing whether the Pokémon has a
// nickname
#define PKM_PARAM_NICKNAME_KEEP_FLAG 0xb0
#define PKM_PARAM_NICKNAME_RAW_KEEP_FLAG 0xb1
// Whether it is one of N's Pokémon
#define PKM_PARAM_N_POKEMON 0xb2
#define PKM_PARAM_POKESTAR_FAME 0xb3

// The numbers of ribbons in the three groups, which start at PKM_PARAM_RIBBON_CHAMPION_SINNOH,
// PKM_PARAM_RIBBON_G3_COOL and PKM_PARAM_RIBBON_G4_COOL
#define PKM_RIBBON_A_COUNT 29
#define PKM_RIBBON_G3_COUNT 32
#define PKM_RIBBON_G4_COUNT 21

// How a Pokémon evolves, the method of its species' evolution table. The evolution demo treats some differently: it
// removes the held item for the held item methods, makes a Shedinja, and doesn't let an evolution by item be stopped.
// The names are the usual ones for the values of the evolution tables, which data/evolutions bears out
#define EVO_METHOD_NONE 0
#define EVO_METHOD_FRIENDSHIP 1
#define EVO_METHOD_FRIENDSHIP_DAY 2
#define EVO_METHOD_FRIENDSHIP_NIGHT 3
#define EVO_METHOD_LEVEL 4
#define EVO_METHOD_TRADE 5
#define EVO_METHOD_TRADE_WITH_ITEM 6
// Traded for a Pokémon of the species in the parameter, as Karrablast and Shelmet
#define EVO_METHOD_TRADE_WITH_SPECIES 7
#define EVO_METHOD_ITEM 8
#define EVO_METHOD_LEVEL_ATK_GT_DEF 9
#define EVO_METHOD_LEVEL_ATK_EQ_DEF 10
#define EVO_METHOD_LEVEL_ATK_LT_DEF 11
// Wurmple's two evolutions, by its personality value
#define EVO_METHOD_LEVEL_PERSONALITY_LOW 12
#define EVO_METHOD_LEVEL_PERSONALITY_HIGH 13
// Evolving makes a Shedinja as well, with a free party slot and a Poké Ball
#define EVO_METHOD_SHEDINJA 14
// The Shedinja that EVO_METHOD_SHEDINJA's evolution makes
#define EVO_METHOD_SHEDINJA_MADE 15
#define EVO_METHOD_BEAUTY 16
#define EVO_METHOD_ITEM_MALE 17
#define EVO_METHOD_ITEM_FEMALE 18
#define EVO_METHOD_HELD_ITEM_DAY 19
#define EVO_METHOD_HELD_ITEM_NIGHT 20
#define EVO_METHOD_KNOWS_MOVE 21
#define EVO_METHOD_SPECIES_IN_PARTY 22
#define EVO_METHOD_LEVEL_MALE 23
#define EVO_METHOD_LEVEL_FEMALE 24
#define EVO_METHOD_LEVEL_MAGNETIC_FIELD 25
#define EVO_METHOD_LEVEL_MOSS_ROCK 26
#define EVO_METHOD_LEVEL_ICE_ROCK 27

// Growth rates of the species data, which pick an experience table. Names and order as pret's pokeplatinum, whose
// values this game keeps
#define GROWTH_MEDIUM_FAST 0
#define GROWTH_ERRATIC 1
#define GROWTH_FLUCTUATING 2
#define GROWTH_MEDIUM_SLOW 3
#define GROWTH_FAST 4
#define GROWTH_SLOW 5

// Egg groups of the species data, as pret's pokeplatinum names them
#define EGG_GROUP_NONE 0
#define EGG_GROUP_MONSTER 1
#define EGG_GROUP_WATER_1 2
#define EGG_GROUP_BUG 3
#define EGG_GROUP_FLYING 4
#define EGG_GROUP_FIELD 5
#define EGG_GROUP_FAIRY 6
#define EGG_GROUP_GRASS 7
#define EGG_GROUP_HUMANSHAPE 8
#define EGG_GROUP_WATER_3 9
#define EGG_GROUP_MINERAL 10
#define EGG_GROUP_AMORPHOUS 11
#define EGG_GROUP_WATER_2 12
#define EGG_GROUP_DITTO 13
#define EGG_GROUP_DRAGON 14
#define EGG_GROUP_UNDISCOVERED 15

// Body colors of the species data, for the Pokédex search
#define COLOR_RED 0
#define COLOR_BLUE 1
#define COLOR_YELLOW 2
#define COLOR_GREEN 3
#define COLOR_BLACK 4
#define COLOR_BROWN 5
#define COLOR_PURPLE 6
#define COLOR_GRAY 7
#define COLOR_WHITE 8
#define COLOR_PINK 9

// Gender ratios of the species data: the chance of female out of 254, or these
#define GENDER_RATIO_MALE_ONLY 0
#define GENDER_RATIO_FEMALE_ONLY 254
#define GENDER_RATIO_GENDERLESS 255

#endif // POKEBW2_CONSTANTS_POKEMON_H
