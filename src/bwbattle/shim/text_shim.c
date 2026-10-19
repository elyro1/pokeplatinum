#include "bwbattle.h"

#include "types.h"
#include "gfl/bmp.h"
#include "gfl/str.h"
#include "shim/bw_shim.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/str_tool.h"

// BW2's text drawing (system/printsys.c, gf_font.c, str_tool.c) as the gauges and effect actors call it: names, levels
// and numbers drawn into bitmaps. The strings hold Platinum's character codes (BwBattleMon.nickname, and the numbers
// BwPlat_FormatNumber writes), drawn in Platinum's font (bwbattle.h). BW2's font is smaller: it draws at y where
// Platinum's glyphs start BW_TEXT_Y_OFFSET higher, so that they fit the same rows

#define BW_TEXT_Y_OFFSET 4

static u8 sLetterColor = 1, sShadowColor = 2, sBackgroundColor = 0;

u32 GFL_FontGetBlockWidth(const StrBuf *strbuf, Font *font, u32 spacing) {
    (void)font;
    return BwPlat_TextWidth(GFL_StrBufGetStringPtr(strbuf)) + spacing * GFL_StrBufGetCharCount(strbuf);
}

static void BwText_Draw(GFLBitmap *bitmap, s16 x, s16 y, const StrBuf *strbuf, u8 letter, u8 shadow, u8 background) {
    BwPlat_DrawText(GFL_StrBufGetStringPtr(strbuf), GFL_BitmapGetPixelData(bitmap), GFL_BitmapGetWidth(bitmap) / 8,
                    GFL_BitmapGetHeight(bitmap) / 8, x, y - BW_TEXT_Y_OFFSET, letter, shadow, background);
}

void GFL_TextRendererDrawToBitmap(GFLBitmap *bitmap, s16 x, s16 y, const StrBuf *strbuf, Font *font) {
    (void)font;
    BwText_Draw(bitmap, x, y, strbuf, sLetterColor, sShadowColor, sBackgroundColor);
}

void GFL_TextRendererDrawToBitmapEx(GFLBitmap *bitmap, s16 x, s16 y, const StrBuf *strbuf, Font *font, u16 color) {
    (void)font;
    // PRINT_COLOR(text, shadow, background)
    BwText_Draw(bitmap, x, y, strbuf, (color >> 10) & 0x1f, (color >> 5) & 0x1f, color & 0x1f);
}

void GFL_TextRndGetGlobalColors(u8 *letter, u8 *shadow, u8 *background) {
    *letter = sLetterColor;
    *shadow = sShadowColor;
    *background = sBackgroundColor;
}

void GFL_TextRndUpdateColorIndexLUT(u8 letter, u8 shadow, u8 background) {
    sLetterColor = letter;
    sShadowColor = shadow;
    sBackgroundColor = background;
}

void GFL_WordSetFormatNumber(StrBuf *strbuf, s32 number, u32 digits, u32 pad, BOOL ascii) {
    u16 chars[12];

    (void)ascii; // Platinum's half-width digits either way
    BwPlat_FormatNumber(chars, NELEMS(chars), number, digits, pad);
    GFL_StrBufLoadString(strbuf, chars);
}
