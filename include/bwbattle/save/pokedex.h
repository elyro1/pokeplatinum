#ifndef POKEBW2_SAVE_POKEDEX_H
#define POKEBW2_SAVE_POKEDEX_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The Pokédex's save block 0x36 (swan's getPokedexSaveAddress). The lists of bits are by species - 1, and the ones of
// forms by the form's place in pokemonFormList

#define POKEDEX_SPECIES_BYTES 0x54
#define POKEDEX_FORM_BYTES 0xb
// The species that the Pokédex has entries in other languages of
#define POKEDEX_LANGUAGE_SPECIES_MAX 493
#define POKEDEX_LANGUAGE_COUNT 7
#define POKEDEX_LANGUAGE_BYTES 0x1b0
#define POKEDEX_MAGIC 0xbeefcafe

// The lists of species seen and shown: by sex, and then the same of shiny Pokémon
enum {
    POKEDEX_KIND_MALE,
    POKEDEX_KIND_FEMALE,
    POKEDEX_KIND_RARE_MALE,
    POKEDEX_KIND_RARE_FEMALE,
    POKEDEX_KIND_COUNT,
};

struct PokeDexSave {
    u32 magic;
    u32 nationalObtained : 1;
    u32 habitatListEnabled : 1;
    // Set to 1 with the Pokédex (func_0200d1dc)
    u32 unk4_2 : 8;
    // Which Pokédex the detail screen counts (func_0200d1f8)
    u32 mode : 2;
    // The species that the Pokédex's list was last on
    u32 lastSpecies : 10;
    u32 unused4_22 : 10;
    u8 caught[POKEDEX_SPECIES_BYTES];
    // A list of POKEDEX_SPECIES_BYTES for each kind
    u8 seen[POKEDEX_KIND_COUNT * POKEDEX_SPECIES_BYTES];
    // Which of them the Pokédex shows of a species: one bit set among the four lists
    u8 shown[POKEDEX_KIND_COUNT * POKEDEX_SPECIES_BYTES];
    // The forms seen and the form shown: a list of POKEDEX_FORM_BYTES for the normal ones, then one for the shiny
    u8 seenForms[2 * POKEDEX_FORM_BYTES];
    u8 shownForms[2 * POKEDEX_FORM_BYTES];
    // The languages a species was caught in, POKEDEX_LANGUAGE_COUNT bits a species
    u8 languages[POKEDEX_LANGUAGE_BYTES];
    // The personality of the first Spinda seen
    u32 spindaPersonality;
}; // size: 0x4dc

BOOL PokeDex_IsNationalObtained(PokeDexSave *pokedex);
u32 getSizeofPokedexData(void);
void initPokedex(PokeDexSave *pokedex);
void PokeDex_SetNationalObtained(PokeDexSave *pokedex);
void PokeDex_EnableHabitatList(PokeDexSave *pokedex);
BOOL PokeDex_IsHabitatListEnabled(PokeDexSave *pokedex);
void givePlayerPokedex(PokeDexSave *pokedex);
u32 func_0200d1dc(PokeDexSave *pokedex);
// Which Pokédex the detail screen counts: 0 the regional one, 1 the national one, 2 the national one once obtained
u32 func_0200d1f8(PokeDexSave *pokedex);
// Sets which Pokédex the detail screen counts, with func_0200d1f8's values
void func_0200d1e4(PokeDexSave *pokedex, u32 mode);
BOOL PokeDex_IsCaught(PokeDexSave *pokedex, u16 species);
BOOL PokeDex_IsSeen(PokeDexSave *pokedex, u16 species);
// The species that the Pokédex's list was last on, and sets it
void func_0200d200(PokeDexSave *pokedex, u16 species);
u16 func_0200d214(PokeDexSave *pokedex);
// Sets the sex, shininess and form that the Pokédex shows of a species
void addToDex(PokeDexSave *pokedex, u16 species, u32 sex, BOOL rare, u32 form);
// The forms of a species, 1 for one without any
u16 getNumberOfForms(u16 species);
// The sex, shininess and form that the Pokédex shows of a species
void func_0200d3c8(PokeDexSave *pokedex, u16 species, u32 *sex, u32 *rare, u32 *form, HeapID heapId);
// Whether the Pokédex has seen the species in that sex, shininess and form
BOOL func_0200d8d4(PokeDexSave *pokedex, u16 species, u32 sex, BOOL rare, u32 form);
// The personality of the Spinda the Pokédex shows, for index 0
u32 func_0200da18(PokeDexSave *pokedex, u32 index);
// Whether the Pokédex has the species' entry in a language
BOOL func_0200db5c(PokeDexSave *pokedex, u16 species, u32 language);
u16 PokeDex_GetSeenNoNational(PokeDexSave *pokedex);
u16 PokeDex_GetCaughtNoNational(PokeDexSave *pokedex);
// The same of the regional Pokédex, whose table they load on the heap
u16 PokeDex_GetSeenNoUnova(PokeDexSave *pokedex, HeapID heapId);
u16 PokeDex_GetCaughtNoUnova(PokeDexSave *pokedex, HeapID heapId);
// Registers a Pokémon as seen
void PokeDex_RegistPkm(PokeDexSave *pokedex, PartyPkm *pkm);
// Registers a Pokémon as seen and caught, with its language
void addPkmToDex(PokeDexSave *pokedex, PartyPkm *pkm);
// The count of seen Pokémon, in the national Pokédex once the player has it
u32 countSeenDexPokes(PokeDexSave *pokedex, HeapID heapId);
// Whether all the species are caught but the mythical ones, in the regional Pokédex too, and whether all of the
// regional one are seen
BOOL PokeDex_IsCompleteNational(PokeDexSave *pokedex);
BOOL PokeDex_IsCompleteUnova(PokeDexSave *pokedex, HeapID heapId);
BOOL PokeDex_IsSeenAllUnova(PokeDexSave *pokedex, HeapID heapId);
// What the professor says of the Pokédex: the message, the count it tells and unk, which the tables give for the count
// without the mythical species. Mode 1 is the national Pokédex's caught, 2 the regional one's seen and 3 its caught
void PokeDex_GetEvaluationParams(PokeDexSave *pokedex, u16 mode, HeapID heapId, u16 *message, u16 *count, u16 *unk);

#endif // POKEBW2_SAVE_POKEDEX_H
