#ifndef POKEBW2_PML_POKE_GRAPHIC_H
#define POKEBW2_PML_POKE_GRAPHIC_H

#include "types.h"
#include "gfl/arc.h"
#include "nnsys/g2d.h"
#include "struct_decls.h"

// The files of Pokémon and trainer sprites, and loading them as cell actor resources (pokegra.c). Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except POKEGRA_DIR_*, PokeGra_*, TrGra_*, MakeTrGraArcHandle
// and LoadSingleCellSpindaGraphicsByBoxData

// Which side of a Pokémon a sprite shows; anything but the front is the back
#define POKEGRA_DIR_FRONT 0
#define POKEGRA_DIR_BACK 1

// The files of a Pokémon's sprite in GetPokemonGraphicsARCID's archive. Each takes the species, its form and sex,
// whether it is shiny, the direction (POKEGRA_DIR_*) and whether it is an egg. A form or sex without files of its own
// gets the species' files
u32 GetPokemonGraphicsARCID(void);
// The characters of the whole sprite as one cell
u32 GetPokemonSingleCellCharacterDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg);
// The characters of the sprite's parts, which GetPokemonMultiCellsDataNo puts together
u32 GetPokemonCharacterDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg);
u32 GetPokemonPaletteDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg);
u32 GetPokemonCellsDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg);
u32 GetPokemonCellAnimeDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg);
u32 GetPokemonMultiCellsDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg);
u32 GetPokemonMultiCellAnimeDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg);
u32 GetPokemonBinFileDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg);
// Rearranges a single-cell sprite's characters, its four OBJs one after the other, into one image 12 characters wide
void PokeGra_CellCharsToImage(NNSG2dCharacterData *chars, HeapID heapId);

// The files of a trainer class's sprite in GetTrainerSpriteARCID's archive, with the flags of GetTrSpriteBaseDatID
u32 GetTrainerSpriteARCID(BOOL back);
u32 GetTrSpriteCharacter2DatID(u32 trainerClass, u32 flags);
u32 GetTrSpritePaletteDatID(u32 trainerClass, u32 flags);
u32 GetTrSpriteCellDatID(u32 trainerClass, u32 flags);
u32 GetTrSpriteCellAnmDatID(u32 trainerClass, u32 flags);
u32 GetTrSpriteMultiCellDatID(u32 trainerClass, u32 flags);
u32 GetTrSpriteMultiCellAnmDatID(u32 trainerClass, u32 flags);
u32 GetTrSpriteBinDatID(u32 trainerClass, u32 flags);

// Loads a Pokémon's sprite as characters of one cell, with a Spinda's spots drawn from its personality, returning the
// file to free
void *LoadSingleCellSpindaGraphicsByBoxData(NNSG2dCharacterData **chars, BoxPkm *pkm, u32 dir, HeapID heapId);
ArcTool *MakePokeGraArcHandle(HeapID heapId);
// Load a Pokémon's palette, its characters (with a Spinda's spots drawn from personality) and its cells with their
// animations as cell actor resources, from the archive MakePokeGraArcHandle opens. The cells are the common ones of
// the app archive for the OBJ mapping
u32 PokeGra_LoadClActPalette(ArcTool *arc, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg, u32 vramType,
                             u16 offset, HeapID heapId);
u32 PokeGra_LoadClActChars(ArcTool *arc, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg, u32 personality,
                           u32 vramType, HeapID heapId);
u32 PokeGra_LoadClActCellAnims(u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg, u32 mapping, u32 vramType,
                               HeapID heapId);
// The same for a Pokémon's data
u32 PokeGra_LoadClActPaletteByBoxData(ArcTool *arc, BoxPkm *pkm, u32 dir, u32 vramType, u16 offset, HeapID heapId);
u32 PokeGra_LoadClActCharsByBoxData(ArcTool *arc, BoxPkm *pkm, u32 dir, u32 vramType, HeapID heapId);
u32 PokeGra_LoadClActCellAnimsByBoxData(BoxPkm *pkm, u32 dir, u32 mapping, u32 vramType, HeapID heapId);

// The same for a trainer class's front sprite
ArcTool *MakeTrGraArcHandle(HeapID heapId);
u32 TrGra_LoadClActPalette(ArcTool *arc, u32 trainerClass, u32 vramType, u16 offset, HeapID heapId);
u32 TrGra_LoadClActChars(ArcTool *arc, u32 trainerClass, u32 vramType, HeapID heapId);
u32 TrGra_LoadClActCellAnims(u32 trainerClass, u32 mapping, u32 vramType, HeapID heapId);
// Replaces the data of loaded characters and palette with a trainer class's front sprite
void TrGra_ReplaceClActCharsAndPalette(ArcTool *arc, u32 trainerClass, u32 chars, u32 palette, HeapID heapId);

#endif // POKEBW2_PML_POKE_GRAPHIC_H
