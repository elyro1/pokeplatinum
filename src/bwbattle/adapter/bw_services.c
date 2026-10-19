#include "bwbattle/bwbattle.h"

#include "constants/heap.h"
#include "constants/charcode.h"
#include "constants/string.h"

#include "font.h"
#include "move_table.h"
#include "render_text.h"
#include "string_gf.h"
#include "text.h"
#include "pokedex.h"
#include "pokemon.h"
#include "savedata.h"
#include "sound_playback.h"

// The services bwbattle asks Platinum for (bwbattle.h). This file is Platinum's code: it sees Platinum's headers and
// none of BW2's.

unsigned int BwPlat_GetLevelExp(unsigned short species, unsigned short form, unsigned char level)
{
    (void)form;
    return Pokemon_GetSpeciesBaseExpAt(species, level);
}

int BwPlat_GetMoveCategory(unsigned short move)
{
    // BW2's categories: 0 status, 1 physical, 2 special
    switch (MoveTable_LoadParam(move, MOVEATTRIBUTE_CLASS)) {
    case CLASS_PHYSICAL:
        return 1;
    case CLASS_SPECIAL:
        return 2;
    default:
        return 0;
    }
}

bool BwPlat_IsCaught(unsigned short species)
{
    return Pokedex_HasCaughtSpecies(SaveData_GetPokedex(SaveData_Ptr()), species);
}

// Platinum's battle text font, as its healthboxes print names in
#define BW_TEXT_FONT FONT_SYSTEM
#define BW_GLYPH_SIZE 16

static void BwPlat_PlotPixel(u8 *tiles, unsigned int tileWidth, unsigned int tileHeight, int x, int y, u8 color)
{
    u8 *byte;

    if (x < 0 || y < 0 || x >= (int)tileWidth * 8 || y >= (int)tileHeight * 8) {
        return;
    }
    byte = tiles + ((y / 8) * tileWidth + x / 8) * 32 + (y % 8) * 4 + (x % 8) / 2;
    if (x & 1) {
        *byte = (*byte & 0x0f) | (color << 4);
    } else {
        *byte = (*byte & 0xf0) | color;
    }
}

unsigned int BwPlat_DrawText(const unsigned short *text, void *tiles, unsigned int tileWidth, unsigned int tileHeight,
    int x, int y, unsigned char letter, unsigned char shadow, unsigned char background)
{
    int startX = x;

    // Platinum's printers set the glyph colors again before each print, so this doesn't disturb them
    Text_GenerateFontHalfRowLookupTable(letter, background, shadow);
    for (; *text != CHAR_EOS; text++) {
        const TextGlyph *glyph = Font_TryLoadGlyph(BW_TEXT_FONT, *text);

        // The glyph's 16 by 16 pixels are four tiles: top left, top right, bottom left, bottom right
        for (int gy = 0; gy < BW_GLYPH_SIZE && gy < glyph->height; gy++) {
            for (int gx = 0; gx < glyph->width && gx < BW_GLYPH_SIZE; gx++) {
                int tile = (gy / 8) * 2 + gx / 8;
                u8 pair = glyph->gfx[tile * 32 + (gy % 8) * 4 + (gx % 8) / 2];
                u8 color = (gx & 1) ? pair >> 4 : pair & 0x0f;

                if (color != 0) {
                    BwPlat_PlotPixel(tiles, tileWidth, tileHeight, x + gx, y + gy, color);
                }
            }
        }
        x += glyph->width;
    }
    return x - startX;
}

unsigned int BwPlat_TextWidth(const unsigned short *text)
{
    return Font_CalcCharArrayWidth(BW_TEXT_FONT, text, 0);
}

void BwPlat_FormatNumber(unsigned short *dest, unsigned int size, int number, unsigned int digits, unsigned int pad)
{
    static const enum PaddingMode sPadding[] = { PADDING_MODE_NONE, PADDING_MODE_SPACES, PADDING_MODE_ZEROES };
    String *string = String_Init(digits + 2, HEAP_ID_BATTLE);

    String_FormatInt(string, number, digits, sPadding[pad < 3 ? pad : 0], CHARSET_MODE_EN);
    String_ToChars(string, dest, size);
    String_Free(string);
}

void BwPlat_PlayCry(unsigned short species, unsigned char form, int pan)
{
    if (species == 0 || species > NATIONAL_DEX_COUNT) {
        return;
    }
    Sound_PlayPokemonCryEx(POKECRY_NORMAL, species, pan < -128 ? -128 : (pan > 127 ? 127 : pan), 127, HEAP_ID_BATTLE,
        form);
}

bool BwPlat_IsCryPlaying(void)
{
    return Sound_IsPokemonCryPlaying();
}
