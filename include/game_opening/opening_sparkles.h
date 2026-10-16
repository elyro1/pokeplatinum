#ifndef POKEPLATINUM_OPENING_SPARKLES_H
#define POKEPLATINUM_OPENING_SPARKLES_H

#include "game_opening/struct_logo_sparkles_decl.h"
#include "game_opening/struct_twinkle_pool_decl.h"

#include "sprite.h"

LogoSparkles *LogoSparkles_New(void);
void LogoSparkles_Free(LogoSparkles *sparkles);
void LogoSparkles_Update(LogoSparkles *sparkles, const int frame);

TwinklePool *TwinklePool_New(const int count);
void TwinklePool_Free(TwinklePool *pool);
Sprite **TwinklePool_GetSpriteSlot(TwinklePool *pool, const int index);
void TwinklePool_Spawn(TwinklePool *pool, BOOL hidden);

#endif // POKEPLATINUM_OPENING_SPARKLES_H
