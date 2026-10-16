#ifndef POKEPLATINUM_OPENING_STARTERS_H
#define POKEPLATINUM_OPENING_STARTERS_H

#include "game_opening/struct_opening_starters_decl.h"

#include "particle_system.h"
#include "spl.h"

int OpeningStarters_GetSize(void);
ParticleSystem *OpeningStarters_GetParticleSystem(OpeningStarters *starters);
SPLEmitter *OpeningStarters_CreateEmitter(OpeningStarters *starters, int emitterID);
void OpeningStarters_Init(OpeningStarters *starters);
void OpeningStarters_Free(OpeningStarters *starters);
void OpeningStarters_UpdateSpriteGraphics(OpeningStarters *starters);
void OpeningStarters_Draw(OpeningStarters *starters);
BOOL OpeningStarters_UpdateAppearance(OpeningStarters *starters, const u8 index);

#endif // POKEPLATINUM_OPENING_STARTERS_H
