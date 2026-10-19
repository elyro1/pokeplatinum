#include "types.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/std.h"
#include "nitro/gx.h"
#include "nitro/hw.h"

// The planes shown on each engine, GX_PLANEMASK_*, kept so that one can be shown or hidden without the others
static struct {
    u32 sub;
    u32 main;
} sEnabledPlanes;

void GFL_BGSysInitVRAM(u32 banks) {
    GX_SetBankForLCDC(GX_VRAM_LCDC_ALL & (banks ^ 0xffff));
    MIi_CpuClearFast(0, (void *)HW_LCDC_VRAM, HW_LCDC_VRAM_SIZE);
    GX_DisableBankForLCDC();
    MIi_CpuClearFast(0xc0, (void *)HW_OAM, HW_OAM_SIZE);
    MIi_CpuClearFast(0, (void *)HW_BG_PLTT, HW_PLTT_SIZE);
    MIi_CpuClearFast(0xc0, (void *)HW_DB_OAM, HW_OAM_SIZE);
    MIi_CpuClearFast(0, (void *)HW_DB_BG_PLTT, HW_PLTT_SIZE);
}

void GFL_BGSysSetVRAMBanks(const BGSysVRAMConfig *config) {
    GX_ResetBankForBG();
    GX_ResetBankForBGExtPltt();
    GX_ResetBankForSubBG();
    GX_ResetBankForSubBGExtPltt();
    GX_ResetBankForOBJ();
    GX_ResetBankForOBJExtPltt();
    GX_ResetBankForSubOBJ();
    GX_ResetBankForSubOBJExtPltt();
    GX_ResetBankForTex();
    GX_ResetBankForTexPltt();
    GX_SetBankForBG(config->bgMain);
    GX_SetBankForBGExtPltt(config->bgExtPaletteMain);
    GX_SetBankForSubBG(config->bgSub);
    GX_SetBankForSubBGExtPltt(config->bgExtPaletteSub);
    GX_SetBankForOBJ(config->objMain);
    GX_SetBankForOBJExtPltt(config->objExtPaletteMain);
    GX_SetBankForSubOBJ(config->objSub);
    GX_SetBankForSubOBJExtPltt(config->objExtPaletteSub);
    GX_SetBankForTex(config->texture);
    GX_SetBankForTexPltt(config->texturePalette);
    GX_SetOBJVRamModeChar(config->objMappingMain);
    GXS_SetOBJVRamModeChar(config->objMappingSub);
}

// bwbattle: inside Platinum's battle the shown planes are Platinum's: BW2 keeps track of what it would show (its
// effects hide the message windows while a move plays), and the hardware isn't told. BW2's own copy would also
// leave out the 3D plane and the OBJs, which Platinum turned on
static void BwLayers_SetVisibleMain(u32 planes) {
#ifdef BWBATTLE
    (void)planes;
#else
    GX_SetVisiblePlane(planes);
#endif
}

static void BwLayers_SetVisibleSub(u32 planes) {
#ifdef BWBATTLE
    (void)planes;
#else
    GXS_SetVisiblePlane(planes);
#endif
}

void GFL_BGSysDisableBGsA(void) {
    sEnabledPlanes.main &= GX_PLANEMASK_OBJ;
}

void GFL_BGSysDisableOBJA(void) {
    sEnabledPlanes.main &= GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3;
}

void GFL_BGSysDisableAllA(void) {
    GFL_BGSysDisableBGsA();
    GFL_BGSysDisableOBJA();
}

void GFL_BGSysSetBGEnabledA(u32 planes, BOOL enabled) {
    if (enabled == TRUE) {
        if (sEnabledPlanes.main & planes) {
            return;
        }
    } else {
        if (!(sEnabledPlanes.main & planes)) {
            return;
        }
    }
    sEnabledPlanes.main ^= planes;
    BwLayers_SetVisibleMain(sEnabledPlanes.main);
}

void GFL_BGSysSetEnabledBGsA(u32 enabled) {
    sEnabledPlanes.main = enabled;
    BwLayers_SetVisibleMain(sEnabledPlanes.main);
}

void GFL_BGSysDisableBGsB(void) {
    sEnabledPlanes.sub &= GX_PLANEMASK_OBJ;
}

void GFL_BGSysDisableOBJB(void) {
    sEnabledPlanes.sub &= GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3;
}

void GFL_BGSysDisableAllB(void) {
    GFL_BGSysDisableBGsB();
    GFL_BGSysDisableOBJB();
}

void GFL_BGSysSetBGEnabledB(u32 planes, BOOL enabled) {
    if (enabled == TRUE) {
        if (sEnabledPlanes.sub & planes) {
            return;
        }
    } else {
        if (!(sEnabledPlanes.sub & planes)) {
            return;
        }
    }
    sEnabledPlanes.sub ^= planes;
    BwLayers_SetVisibleSub(sEnabledPlanes.sub);
}

void GFL_BGSysSetEnabledBGsB(u32 enabled) {
    sEnabledPlanes.sub = enabled;
    BwLayers_SetVisibleSub(sEnabledPlanes.sub);
}

void GFL_BGSysEnableEngines(void) {
    GX_DispOn();
    GXS_DispOn();
}

void GFL_BGSysSetDisplayLayout(u32 layout) {
    GX_SetDispSelect(layout);
}

u32 GFL_BGSysGetEnabledBGsA(void) {
    return sEnabledPlanes.main;
}

u32 GFL_BGSysGetEnabledBGsB(void) {
    return sEnabledPlanes.sub;
}
