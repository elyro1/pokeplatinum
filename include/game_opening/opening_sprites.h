#ifndef POKEPLATINUM_OPENING_SPRITES_H
#define POKEPLATINUM_OPENING_SPRITES_H

#include "game_opening/struct_opening_sprites_decl.h"
#include "game_opening/struct_twinkle_pool_decl.h"

#define TWINKLE_SPRITE_COUNT 16

enum OpeningSpriteSlot {
    OPENING_SPRITE_LEFT_MAIN = 0,
    OPENING_SPRITE_LEFT_SUB,
    OPENING_SPRITE_RIGHT_MAIN,
    OPENING_SPRITE_SLIDE_FROM_LEFT,
    OPENING_SPRITE_FLOCK_1,
    OPENING_SPRITE_FLOCK_2,
    OPENING_SPRITE_FLOCK_3,
    OPENING_SPRITE_FLOCK_4,
    OPENING_SPRITE_FLOCK_5,
    OPENING_SPRITE_FLOCK_6,
    OPENING_SPRITE_SLIDE_FROM_RIGHT,
    OPENING_SPRITE_SPINNER_MAIN,
    OPENING_SPRITE_SPINNER_SUB,
    OPENING_SPRITE_BAND_MAIN_A,
    OPENING_SPRITE_BAND_MAIN_B,
    OPENING_SPRITE_BAND_SUB_A,
    OPENING_SPRITE_BAND_SUB_B,

    OPENING_SPRITE_COUNT,
};

int OpeningSprites_GetSize(void);
void OpeningSprites_Init(OpeningSprites *sprites);
void OpeningSprites_Free(OpeningSprites *sprites);
void OpeningSprites_CreateSprites(OpeningSprites *sprites);
void OpeningSprites_CreateTwinkleSprites(OpeningSprites *sprites, TwinklePool *twinkles);
void OpeningSprites_HideTwinkleSprites(TwinklePool *twinkles);
void OpeningSprites_Update(OpeningSprites *sprites);
void OpeningSprites_SetVisible(OpeningSprites *sprites, const u8 slot, const u8 visible);
void OpeningSprites_SetLeftRightAnimating(OpeningSprites *sprites, const u8 animating);
void OpeningSprites_SpreadLeftRight(OpeningSprites *sprites);
BOOL OpeningSprites_UpdateSlideFromLeft(OpeningSprites *sprites, int frame);
BOOL OpeningSprites_UpdateSlideFromRight(OpeningSprites *sprites, int frame);
void OpeningSprites_ScrollBands(OpeningSprites *sprites, fx32 speed);
void OpeningSprites_SetBandPalettes(OpeningSprites *sprites, BOOL useFinalPalette);
void OpeningSprites_UpdateSpinners(OpeningSprites *sprites, int frame);
void OpeningSprites_UpdateFlock(OpeningSprites *sprites, const int frame);

#endif // POKEPLATINUM_OPENING_SPRITES_H
