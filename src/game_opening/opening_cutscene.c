#include <nitro.h>
#include <string.h>

#include "constants/graphics.h"

#include "applications/title_screen.h"
#include "game_opening/opening_timeline.h"
#include "game_opening/opening_sprites.h"
#include "game_opening/opening_sparkles.h"
#include "game_opening/opening_starters.h"
#include "game_opening/struct_logo_sparkles_decl.h"
#include "game_opening/struct_opening_sprites_decl.h"
#include "game_opening/struct_opening_starters_decl.h"
#include "game_opening/struct_twinkle_pool_decl.h"
#include "overlay009/camera_configuration.h"

#include "bg_window.h"
#include "brightness_controller.h"
#include "camera.h"
#include "easy3d.h"
#include "g3d_pipeline.h"
#include "graphics.h"
#include "gx_layers.h"
#include "heap.h"
#include "main.h"
#include "math_util.h"
#include "narc.h"
#include "overlay_manager.h"
#include "particle_system.h"
#include "render_oam.h"
#include "screen_fade.h"
#include "sound.h"
#include "sys_task.h"
#include "sys_task_manager.h"
#include "system.h"
#include "unk_0202419C.h"
#include "vram_transfer.h"

FS_EXTERN_OVERLAY(game_opening);

#define ALL_WND_PLANES (GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ)

#define MAX_LANDSCAPE_MODELS 6

#define OFFSCREEN_BG_OFFSET 256

enum BgLayout {
    BG_LAYOUT_LANDSCAPE = 0,
    BG_LAYOUT_SCROLLING,
    BG_LAYOUT_SCROLLING_WITH_OVERLAY,
};

typedef struct {
    NNSG3dRenderObj renderObj;
    NNSG3dResFileHeader *modelFile;
} LandscapeModel;

typedef struct {
    void *textureFile;
    NNSG3dResTex *texture;
    Camera *camera;
    LandscapeModel models[MAX_LANDSCAPE_MODELS];
    int modelCount;
} Landscape;

typedef struct {
    u8 state;
    u8 subState;
    u8 isSetUp;
    u8 fadeLevel;
    u8 fadeTimer;
    u8 fadeDone;
    BgConfig *bgConfig;
    SysTask *fadeTask;
    LogoSparkles *sparkles;
    u8 *canSkip;
} LogoScene;

typedef struct {
    u8 state;
    u8 subState;
    u8 isSetUp;
    u8 spritesLoaded;
    u8 landscapeLoaded;
    u8 landscapeLoading;
    u8 showLandscape;
    u8 twinkleTimer;
    u8 twinkleInterval;
    u8 landscapeLoadStep;
    BgConfig *bgConfig;
    OpeningSprites *sprites;
    TwinklePool *twinkles;
    OpeningStarters *starters;
    Landscape landscape;
    u16 *savedPalette;
    u8 hideTwinkles;
    u8 enableWindowsPending;
    u8 disableWindowsPending;
    u8 showLayersMask;
    u8 hideLayersMask;
    s16 mainWndLeft;
    s16 mainWndRight;
    s16 mainWndTop;
    s16 mainWndBottom;
    s16 subWndLeft;
    s16 subWndRight;
    s16 subWndTop;
    s16 subWndBottom;
    u8 bgLayout;
    u8 swapScreensPending;
    u8 drawLandscape;
    u8 drawParticles;
    fx32 bg2ScrollY;
    fx32 bg1ScrollY;
} MainScene;

typedef struct {
    u8 state;
    u8 subState;
    u8 isSetUp;
    fx32 scrollY;
    BgConfig *bgConfig;
} ClosingScene;

typedef struct {
    enum HeapID heapID;
    int frame;
    BOOL skipRequested;
    BgConfig *bgConfig;
    G3DPipelineBuffers *pipeline;
    u32 savedRngSeed;
    LogoScene logoScene;
    MainScene mainScene;
    ClosingScene closingScene;
    u8 canSkip;
} OpeningCutscene;

static BOOL OpeningCutscene_Init(ApplicationManager *appMan, int *state);
static BOOL OpeningCutscene_Main(ApplicationManager *appMan, int *state);
static BOOL OpeningCutscene_Exit(ApplicationManager *appMan, int *state);
static BOOL OpeningCutscene_RunLogoScene(OpeningCutscene *cutscene);
static BOOL OpeningCutscene_RunMainScene(OpeningCutscene *cutscene);
static BOOL OpeningCutscene_RunClosingScene(OpeningCutscene *cutscene);
static void OpeningCutscene_Setup3DGraphics(void);
static void OpeningCutscene_EnableWindows(void);
static void OpeningCutscene_DisableWindows(void);
static void OpeningCutscene_FreeLandscape(OpeningCutscene *cutscene);
static void OpeningCutscene_FreeBgs(OpeningCutscene *cutscene, BOOL freeConfig);
static void OpeningCutscene_SetUpLogoScene(OpeningCutscene *cutscene);
static void LogoScene_LoadGraphics(LogoScene *scene);
static BOOL LogoScene_Update(LogoScene *scene, const int frame);
static void OpeningCutscene_CleanUpLogoScene(OpeningCutscene *cutscene);
static void LogoScene_FadeInFirstTask(SysTask *task, void *param);
static void LogoScene_FadeOutFirstTask(SysTask *task, void *param);
static void LogoScene_FadeInSecondMainTask(SysTask *task, void *param);
static void LogoScene_FadeInSecondSubTask(SysTask *task, void *param);
static void OpeningCutscene_SetUpMainScene(OpeningCutscene *cutscene);
static void MainScene_LoadGraphics(MainScene *scene);
static void MainScene_LoadScrollingBgs(OpeningCutscene *cutscene, MainScene *scene);
static void MainScene_ArrangeLayers(MainScene *scene);
static void MainScene_LoadStarterBgs(OpeningCutscene *cutscene, MainScene *scene);
static void MainScene_LoadFinalBgs(OpeningCutscene *cutscene, MainScene *scene);
static void MainScene_LoadLandscape(MainScene *scene, int landscapeIndex);
static BOOL MainScene_LoadLandscapeStep(MainScene *scene, int landscapeIndex);
static void MainScene_DrawLandscape(MainScene *scene);
static BOOL MainScene_Update(OpeningCutscene *cutscene, MainScene *scene, const int frame);
static void MainScene_UpdateScrollingBgs(MainScene *scene);
static void MainScene_ScrollBackground(MainScene *scene);
static void MainScene_UpdateSlidingBgs(MainScene *scene, int frame);
static void MainScene_SlideBgsInStaggered(MainScene *scene, int frame);
static void MainScene_SlideBgsIn(MainScene *scene);
static void MainScene_UpdateLandscapeCamera(MainScene *scene, const int frame);
static void MainScene_UpdateLandscapeCuts(OpeningCutscene *cutscene, MainScene *scene, int frame);
static void OpeningCutscene_CleanUpMainScene(OpeningCutscene *cutscene);
static void OpeningCutscene_SetUpClosingScene(OpeningCutscene *cutscene);
static void ClosingScene_LoadGraphics(ClosingScene *scene);
static BOOL ClosingScene_Update(ClosingScene *scene, const int frame);
static void OpeningCutscene_CleanUpClosingScene(OpeningCutscene *cutscene);

const ApplicationManagerTemplate gOpeningCutsceneAppTemplate = {
    OpeningCutscene_Init,
    OpeningCutscene_Main,
    OpeningCutscene_Exit,
    0xffffffff
};

static const int sLandscapeTextureIDs[] = {
    0x66,
    0x6D,
    0x70
};

static const int sLandscapeModelIDs[][MAX_LANDSCAPE_MODELS] = {
    { 0x64, 0x65, 0x0, 0x0, 0x0, 0x0 },
    { 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C },
    { 0x6E, 0x6F, 0x0, 0x0, 0x0, 0x0 }
};

static const CameraConfiguration sLandscapeCameraConfig = {
    0x29AEC1,
    { -0x29fe, 0x0, 0x0 },
    0x0,
    0x5C1,
    0x0
};

static const GXRgb sEdgeColors[8] = {
    GX_RGB(0, 0, 0),
    GX_RGB(4, 4, 4),
    GX_RGB(4, 4, 4),
    GX_RGB(4, 4, 4),
    GX_RGB(4, 4, 4),
    GX_RGB(4, 4, 4),
    GX_RGB(4, 4, 4),
    GX_RGB(4, 4, 4)
};

static const VecFx32 sLightVectors[4] = {
    { -2043, -3548, 0x6E },
    { 0x0, 0x0, 0x0 },
    { 0x0, 0x0, 0x1000 },
    { 0x0, 0x0, 0x1000 }
};

static const GXRgb sLightColors[4] = {
    GX_RGB(24, 24, 18),
    GX_RGB(0, 0, 0),
    GX_RGB(2, 2, 12),
    GX_RGB(31, 31, 31),
};

static void OpeningCutscene_SetLogoVRAMBanks(void)
{
    GXBanks banks = {
        GX_VRAM_BG_128_B,
        GX_VRAM_BGEXTPLTT_NONE,
        GX_VRAM_SUB_BG_128_C,
        GX_VRAM_SUB_BGEXTPLTT_NONE,
        GX_VRAM_OBJ_80_EF,
        GX_VRAM_OBJEXTPLTT_NONE,
        GX_VRAM_SUB_OBJ_16_I,
        GX_VRAM_SUB_OBJEXTPLTT_NONE,
        GX_VRAM_TEX_0_A,
        GX_VRAM_TEXPLTT_0_G
    };

    GXLayers_SetBanks(&banks);
}

static void OpeningCutscene_SetMainVRAMBanks(void)
{
    GXBanks banks = {
        GX_VRAM_BG_128_B,
        GX_VRAM_BGEXTPLTT_NONE,
        GX_VRAM_SUB_BG_128_C,
        GX_VRAM_SUB_BGEXTPLTT_NONE,
        GX_VRAM_OBJ_32_FG,
        GX_VRAM_OBJEXTPLTT_NONE,
        GX_VRAM_SUB_OBJ_16_I,
        GX_VRAM_SUB_OBJEXTPLTT_NONE,
        GX_VRAM_TEX_0_A,
        GX_VRAM_TEXPLTT_0123_E
    };

    GXLayers_SetBanks(&banks);
}

static void OpeningCutscene_SetClosingVRAMBanks(void)
{
    GXBanks banks = {
        GX_VRAM_BG_256_AB,
        GX_VRAM_BGEXTPLTT_NONE,
        GX_VRAM_SUB_BG_128_C,
        GX_VRAM_SUB_BGEXTPLTT_NONE,
        GX_VRAM_OBJ_NONE,
        GX_VRAM_OBJEXTPLTT_NONE,
        GX_VRAM_SUB_OBJ_NONE,
        GX_VRAM_SUB_OBJEXTPLTT_NONE,
        GX_VRAM_TEX_NONE,
        GX_VRAM_TEXPLTT_NONE
    };

    GXLayers_SetBanks(&banks);
}

static void OpeningCutscene_InitLogoBgs(OpeningCutscene *cutscene)
{
    cutscene->bgConfig = BgConfig_New(cutscene->heapID);

    {
        GraphicsModes modes = {
            GX_DISPMODE_GRAPHICS,
            GX_BGMODE_0,
            GX_BGMODE_0,
            GX_BG0_AS_2D
        };

        SetAllGraphicsModes(&modes);
    }

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0x800,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_256x256,
            .colorMode = GX_BG_COLORMODE_16,
            .screenBase = GX_BG_SCRBASE_0x0000,
            .charBase = GX_BG_CHARBASE_0x04000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 0,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_MAIN_1, &template, 0);
    }

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0x800,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_256x256,
            .colorMode = GX_BG_COLORMODE_16,
            .screenBase = GX_BG_SCRBASE_0x0800,
            .charBase = GX_BG_CHARBASE_0x08000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 1,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_MAIN_2, &template, 0);
    }

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0x800,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_256x256,
            .colorMode = GX_BG_COLORMODE_16,
            .screenBase = GX_BG_SCRBASE_0x1000,
            .charBase = GX_BG_CHARBASE_0x08000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 2,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_MAIN_3, &template, 0);
    }

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0x800,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_256x256,
            .colorMode = GX_BG_COLORMODE_16,
            .screenBase = GX_BG_SCRBASE_0x1000,
            .charBase = GX_BG_CHARBASE_0x08000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 0,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_SUB_1, &template, 0);
    }

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0x800,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_256x256,
            .colorMode = GX_BG_COLORMODE_16,
            .screenBase = GX_BG_SCRBASE_0x0000,
            .charBase = GX_BG_CHARBASE_0x04000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 1,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_SUB_2, &template, 0);
    }

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0x800,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_256x256,
            .colorMode = GX_BG_COLORMODE_16,
            .screenBase = GX_BG_SCRBASE_0x0800,
            .charBase = GX_BG_CHARBASE_0x04000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 2,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_SUB_3, &template, 0);
    }

    Bg_MaskPalette(BG_LAYER_MAIN_0, 0x0);
    Bg_MaskPalette(BG_LAYER_SUB_0, 0x0);
}

static void OpeningCutscene_InitLandscapeBgs(OpeningCutscene *cutscene)
{
    cutscene->bgConfig = BgConfig_New(cutscene->heapID);

    {
        GraphicsModes modes = {
            GX_DISPMODE_GRAPHICS,
            GX_BGMODE_0,
            GX_BGMODE_0,
            GX_BG0_AS_3D
        };

        SetAllGraphicsModes(&modes);
    }

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_512x512,
            .colorMode = GX_BG_COLORMODE_16,
            .screenBase = GX_BG_SCRBASE_0x2000,
            .charBase = GX_BG_CHARBASE_0x10000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 3,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_MAIN_1, &template, 0);
    }

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_256x512,
            .colorMode = GX_BG_COLORMODE_256,
            .screenBase = GX_BG_SCRBASE_0x4000,
            .charBase = GX_BG_CHARBASE_0x18000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 3,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_MAIN_2, &template, 0);
    }

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_256x512,
            .colorMode = GX_BG_COLORMODE_256,
            .screenBase = GX_BG_SCRBASE_0x0000,
            .charBase = GX_BG_CHARBASE_0x08000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 3,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_MAIN_3, &template, 0);
    }

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0x1000,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_256x256,
            .colorMode = GX_BG_COLORMODE_256,
            .screenBase = GX_BG_SCRBASE_0x6000,
            .charBase = GX_BG_CHARBASE_0x14000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 0,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_SUB_0, &template, 0);
    }

    {
        BgTemplate templates[] = {
            {
                .x = 0,
                .y = 0,
                .bufferSize = 0,
                .baseTile = 0,
                .screenSize = BG_SCREEN_SIZE_512x512,
                .colorMode = GX_BG_COLORMODE_16,
                .screenBase = GX_BG_SCRBASE_0x0000,
                .charBase = GX_BG_CHARBASE_0x08000,
                .bgExtPltt = GX_BG_EXTPLTT_01,
                .priority = 3,
                .areaOver = 0,
                .mosaic = FALSE,
            },
            {
                .x = 0,
                .y = 0,
                .bufferSize = 0,
                .baseTile = 0,
                .screenSize = BG_SCREEN_SIZE_256x512,
                .colorMode = GX_BG_COLORMODE_16,
                .screenBase = GX_BG_SCRBASE_0x2000,
                .charBase = GX_BG_CHARBASE_0x0c000,
                .bgExtPltt = GX_BG_EXTPLTT_01,
                .priority = 3,
                .areaOver = 0,
                .mosaic = FALSE,
            },
            {
                .x = 0,
                .y = 0,
                .bufferSize = 0,
                .baseTile = 0,
                .screenSize = BG_SCREEN_SIZE_256x512,
                .colorMode = GX_BG_COLORMODE_16,
                .screenBase = GX_BG_SCRBASE_0x4000,
                .charBase = GX_BG_CHARBASE_0x10000,
                .bgExtPltt = GX_BG_EXTPLTT_01,
                .priority = 3,
                .areaOver = 0,
                .mosaic = FALSE,
            },
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_SUB_1, &templates[0], 0);
        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_SUB_2, &templates[1], 0);
        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_SUB_3, &templates[2], 0);
    }

    Bg_MaskPalette(BG_LAYER_MAIN_1, 0x0);
    Bg_MaskPalette(BG_LAYER_SUB_0, 0x0);
}

static void OpeningCutscene_InitScrollingBgs(OpeningCutscene *cutscene)
{
    OpeningCutscene_FreeBgs(cutscene, FALSE);

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_512x512,
            .colorMode = GX_BG_COLORMODE_16,
            .screenBase = GX_BG_SCRBASE_0x0000,
            .charBase = GX_BG_CHARBASE_0x08000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 3,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_MAIN_3, &template, 0);
    }

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_512x512,
            .colorMode = GX_BG_COLORMODE_16,
            .screenBase = GX_BG_SCRBASE_0x2000,
            .charBase = GX_BG_CHARBASE_0x0c000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 3,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_MAIN_1, &template, 0);
    }

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_512x512,
            .colorMode = GX_BG_COLORMODE_16,
            .screenBase = GX_BG_SCRBASE_0x4000,
            .charBase = GX_BG_CHARBASE_0x10000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 3,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_MAIN_2, &template, 0);
    }

    {
        BgTemplate templates[] = {
            {
                .x = 0,
                .y = 0,
                .bufferSize = 0,
                .baseTile = 0,
                .screenSize = BG_SCREEN_SIZE_512x512,
                .colorMode = GX_BG_COLORMODE_16,
                .screenBase = GX_BG_SCRBASE_0x0000,
                .charBase = GX_BG_CHARBASE_0x08000,
                .bgExtPltt = GX_BG_EXTPLTT_01,
                .priority = 3,
                .areaOver = 0,
                .mosaic = FALSE,
            },
            {
                .x = 0,
                .y = 0,
                .bufferSize = 0,
                .baseTile = 0,
                .screenSize = BG_SCREEN_SIZE_512x512,
                .colorMode = GX_BG_COLORMODE_16,
                .screenBase = GX_BG_SCRBASE_0x2000,
                .charBase = GX_BG_CHARBASE_0x0c000,
                .bgExtPltt = GX_BG_EXTPLTT_01,
                .priority = 3,
                .areaOver = 0,
                .mosaic = FALSE,
            },
            {
                .x = 0,
                .y = 0,
                .bufferSize = 0,
                .baseTile = 0,
                .screenSize = BG_SCREEN_SIZE_512x512,
                .colorMode = GX_BG_COLORMODE_16,
                .screenBase = GX_BG_SCRBASE_0x4000,
                .charBase = GX_BG_CHARBASE_0x10000,
                .bgExtPltt = GX_BG_EXTPLTT_01,
                .priority = 3,
                .areaOver = 0,
                .mosaic = FALSE,
            },
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_SUB_1, &templates[0], 0);
        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_SUB_2, &templates[1], 0);
        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_SUB_3, &templates[2], 0);
    }

    Bg_MaskPalette(BG_LAYER_MAIN_1, 0x0);
    Bg_MaskPalette(BG_LAYER_SUB_0, 0x0);
    GXLayers_EngineBToggleLayers(GX_PLANEMASK_BG0, 0);

    cutscene->mainScene.bgLayout = BG_LAYOUT_SCROLLING;
}

static void OpeningCutscene_InitOverlayBgs(OpeningCutscene *cutscene)
{
    OpeningCutscene_FreeLandscape(cutscene);
    OpeningCutscene_InitScrollingBgs(cutscene);

    {
        GraphicsModes modes = {
            GX_DISPMODE_GRAPHICS,
            GX_BGMODE_0,
            GX_BGMODE_0,
            GX_BG0_AS_2D
        };

        SetAllGraphicsModes(&modes);
    }

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_256x512,
            .colorMode = GX_BG_COLORMODE_16,
            .screenBase = GX_BG_SCRBASE_0x6000,
            .charBase = GX_BG_CHARBASE_0x14000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 0,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_MAIN_0, &template, 0);
    }

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_256x512,
            .colorMode = GX_BG_COLORMODE_16,
            .screenBase = GX_BG_SCRBASE_0x6000,
            .charBase = GX_BG_CHARBASE_0x14000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 0,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_SUB_0, &template, 0);
    }

    Bg_SetOffset(cutscene->bgConfig, BG_LAYER_MAIN_0, BG_OFFSET_UPDATE_SET_X, 0);
    Bg_SetOffset(cutscene->bgConfig, BG_LAYER_MAIN_0, BG_OFFSET_UPDATE_SET_Y, 0);
    Bg_SetOffset(cutscene->bgConfig, BG_LAYER_SUB_0, BG_OFFSET_UPDATE_SET_X, 0);
    Bg_SetOffset(cutscene->bgConfig, BG_LAYER_SUB_0, BG_OFFSET_UPDATE_SET_Y, 0);

    GXLayers_EngineAToggleLayers(GX_PLANEMASK_BG0, 0);
    GXLayers_EngineBToggleLayers(GX_PLANEMASK_BG0, 0);
    GXLayers_EngineAToggleLayers(GX_PLANEMASK_BG1, 1);
    GXLayers_EngineBToggleLayers(GX_PLANEMASK_BG1, 1);
    GXLayers_EngineAToggleLayers(GX_PLANEMASK_BG2, 1);
    GXLayers_EngineBToggleLayers(GX_PLANEMASK_BG2, 1);
    GXLayers_EngineAToggleLayers(GX_PLANEMASK_BG3, 1);
    GXLayers_EngineBToggleLayers(GX_PLANEMASK_BG3, 1);
    GXLayers_EngineAToggleLayers(GX_PLANEMASK_OBJ, 1);
    GXLayers_EngineBToggleLayers(GX_PLANEMASK_OBJ, 1);

    cutscene->mainScene.bgLayout = BG_LAYOUT_SCROLLING_WITH_OVERLAY;
}

static void OpeningCutscene_InitClosingBgs(OpeningCutscene *cutscene)
{
    cutscene->bgConfig = BgConfig_New(cutscene->heapID);

    {
        GraphicsModes modes = {
            GX_DISPMODE_GRAPHICS,
            GX_BGMODE_0,
            GX_BGMODE_0,
            GX_BG0_AS_2D
        };

        SetAllGraphicsModes(&modes);
    }

    {
        BgTemplate template = {
            .x = 0,
            .y = 0,
            .bufferSize = 0x1000,
            .baseTile = 0,
            .screenSize = BG_SCREEN_SIZE_256x512,
            .colorMode = GX_BG_COLORMODE_16,
            .screenBase = GX_BG_SCRBASE_0x0000,
            .charBase = GX_BG_CHARBASE_0x08000,
            .bgExtPltt = GX_BG_EXTPLTT_01,
            .priority = 1,
            .areaOver = 0,
            .mosaic = FALSE,
        };

        Bg_InitFromTemplate(cutscene->bgConfig, BG_LAYER_MAIN_2, &template, 0);
    }

    Bg_MaskPalette(BG_LAYER_MAIN_2, 0x0);
}

static void OpeningCutscene_Init3DPipeline(OpeningCutscene *cutscene)
{
    cutscene->pipeline = G3DPipeline_Init(cutscene->heapID, TEXTURE_VRAM_SIZE_128K, PALETTE_VRAM_SIZE_64K, OpeningCutscene_Setup3DGraphics);

    // The allocated keys are intentionally not kept; the allocations only reserve the VRAM.
    NNS_GfdAllocTexVram(0x2000 * 4, 0, 0);
    NNS_GfdAllocPlttVram(0x20 * 4, 0, NNS_GFD_ALLOC_FROM_LOW);
}

static void OpeningCutscene_Setup3DGraphics(void)
{
    NNS_G3dGeFlushBuffer();

    G2_SetBG0Priority(1);
    G3X_SetShading(GX_SHADING_TOON);
    G3X_AntiAlias(1);
    G3X_AlphaTest(0, 0);
    G3X_AlphaBlend(1);
    G3X_EdgeMarking(1);
    G3X_SetEdgeColorTable(sEdgeColors);
    G3X_SetFog(0, GX_FOGBLEND_COLOR_ALPHA, GX_FOGSLOPE_0x8000, 0);
    G3X_SetClearColor(GX_RGB(0, 0, 0), 0, 0x7fff, 63, 0);
    G3_ViewPort(0, 0, 255, 191);
}

static void OpeningCutscene_LogoVBlank(void *param)
{
    RenderOam_Transfer();
}

static void OpeningCutscene_MainVBlank(void *param)
{
    int i;
    OpeningCutscene *cutscene = param;
    MainScene *scene = &cutscene->mainScene;

    if (scene->swapScreensPending == TRUE) {
        gSystem.whichScreenIs3D = DS_SCREEN_SUB;

        GXLayers_SwapDisplay();
        GXLayers_EngineBToggleLayers(GX_PLANEMASK_BG0, 1);
        GXLayers_EngineAToggleLayers(GX_PLANEMASK_BG2, 0);
        GXLayers_EngineAToggleLayers(GX_PLANEMASK_BG3, 0);
        GXLayers_EngineBToggleLayers(GX_PLANEMASK_OBJ, 1);

        scene->swapScreensPending = FALSE;

        Camera_ComputeProjectionMatrix(0, scene->landscape.camera);
        Camera_SetAsActive(scene->landscape.camera);
    }

    if (cutscene->bgConfig != NULL) {
        Bg_RunScheduledUpdates(cutscene->bgConfig);
    }

    for (i = 0; i < BG_LAYER_MAX; i++) {
        if (scene->showLayersMask & 1) {
            Bg_ToggleLayer(i, TRUE);
        }

        if (scene->hideLayersMask & 1) {
            Bg_ToggleLayer(i, FALSE);
        }

        scene->showLayersMask >>= 1;
        scene->hideLayersMask >>= 1;
    }

    G2_SetWnd0Position(scene->mainWndLeft, scene->mainWndTop, scene->mainWndRight, scene->mainWndBottom);
    G2S_SetWnd0Position(scene->subWndLeft, scene->subWndTop, scene->subWndRight, scene->subWndBottom);

    if (scene->enableWindowsPending == TRUE) {
        OpeningCutscene_EnableWindows();
        scene->enableWindowsPending = FALSE;
    }

    if (scene->disableWindowsPending == TRUE) {
        OpeningCutscene_DisableWindows();
        scene->disableWindowsPending = FALSE;
    }

    VramTransfer_Process();
    RenderOam_Transfer();
    OpeningStarters_UpdateSpriteGraphics(cutscene->mainScene.starters);
}

static void OpeningCutscene_EnableWindows(void)
{
    GX_SetVisibleWnd(GX_WNDMASK_W0);
    G2_SetWnd0InsidePlane(ALL_WND_PLANES, TRUE);
    G2_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_OBJ, TRUE);
    GXS_SetVisibleWnd(GX_WNDMASK_W0);
    G2S_SetWnd0InsidePlane(ALL_WND_PLANES, TRUE);
    G2S_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_OBJ, TRUE);
}

static void OpeningCutscene_DisableWindows(void)
{
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
}

static BOOL OpeningCutscene_Init(ApplicationManager *appMan, int *state)
{
    OpeningCutscene *cutscene;

    BrightnessController_ResetAllControllers();
    SetScreenColorBrightness(DS_SCREEN_MAIN, COLOR_WHITE);
    SetScreenColorBrightness(DS_SCREEN_SUB, COLOR_WHITE);
    SetVBlankCallback(NULL, NULL);
    SetHBlankCallback(NULL, NULL);
    GXLayers_DisableEngineALayers();
    GXLayers_DisableEngineBLayers();
    SetAutorepeat(4, 8);
    Heap_Create(HEAP_ID_APPLICATION, HEAP_ID_76, 0xa0000);

    cutscene = ApplicationManager_NewData(appMan, sizeof(OpeningCutscene), HEAP_ID_76);
    memset(cutscene, 0, sizeof(OpeningCutscene));

    cutscene->heapID = HEAP_ID_76;
    cutscene->skipRequested = FALSE;
    cutscene->canSkip = FALSE;

    gSystem.whichScreenIs3D = DS_SCREEN_MAIN;
    GXLayers_SwapDisplay();
    cutscene->savedRngSeed = LCRNG_GetSeed();

    LCRNG_SetSeed(0);

    return TRUE;
}

static BOOL OpeningCutscene_Main(ApplicationManager *appMan, int *state)
{
    OpeningCutscene *cutscene = ApplicationManager_Data(appMan);

    if ((cutscene->canSkip) && ((gSystem.pressedKeys & PAD_BUTTON_A) || (gSystem.pressedKeys & PAD_BUTTON_START))) {
        cutscene->skipRequested = TRUE;
        gSystem.showTitleScreenIntro = FALSE;
        SetScreenColorBrightness(DS_SCREEN_MAIN, COLOR_BLACK);
        SetScreenColorBrightness(DS_SCREEN_SUB, COLOR_BLACK);
    }

    switch (*state) {
    case 0:
        cutscene->logoScene.canSkip = &cutscene->canSkip;
        (*state)++;
        break;
    case 1:
        if (OpeningCutscene_RunLogoScene(cutscene)) {
            (*state)++;
        }
        break;
    case 2:
        if (OpeningCutscene_RunMainScene(cutscene)) {
            (*state)++;
        }
        break;
    case 3:
        if (OpeningCutscene_RunClosingScene(cutscene)) {
            (*state)++;
        }
        break;
    case 4:
        if (cutscene->frame >= OPENING_END_FRAME) {
            return TRUE;
        }
        break;
    default:
        GF_ASSERT(FALSE);
        break;
    }

    if (cutscene->skipRequested) {
        return TRUE;
    }

    cutscene->frame++;
    return FALSE;
}

static BOOL OpeningCutscene_Exit(ApplicationManager *appMan, int *state)
{
    OpeningCutscene *cutscene = ApplicationManager_Data(appMan);

    if (IsScreenFadeDone() == FALSE) {
        FinishScreenFade();
    }

    LCRNG_SetSeed(cutscene->savedRngSeed);
    ApplicationManager_FreeData(appMan);
    Heap_Destroy(HEAP_ID_76);
    EnqueueApplication(FS_OVERLAY_ID(game_opening), &gTitleScreenAppTemplate);

    return TRUE;
}

static BOOL OpeningCutscene_RunLogoScene(OpeningCutscene *cutscene)
{
    u8 *state = &(cutscene->logoScene.state);

    if (cutscene->skipRequested) {
        (*state) = 3;
    }

    switch (*state) {
    case 0:
        OpeningCutscene_SetUpLogoScene(cutscene);
        (*state)++;
        break;
    case 1:
        LogoScene_LoadGraphics(&cutscene->logoScene);
        (*state)++;
        break;
    case 2:
        if (LogoScene_Update(&cutscene->logoScene, cutscene->frame)) {
            (*state)++;
        }
        break;
    case 3:
        OpeningCutscene_CleanUpLogoScene(cutscene);
        return TRUE;
    }

    return FALSE;
}

static void OpeningCutscene_SetUpLogoScene(OpeningCutscene *cutscene)
{
    OpeningCutscene_SetLogoVRAMBanks();
    OpeningCutscene_InitLogoBgs(cutscene);

    cutscene->logoScene.bgConfig = cutscene->bgConfig;
    cutscene->logoScene.sparkles = LogoSparkles_New();

    SetVBlankCallback(OpeningCutscene_LogoVBlank, (void *)cutscene);

    cutscene->logoScene.isSetUp = TRUE;
}

static void LogoScene_LoadGraphics(LogoScene *scene)
{
    Graphics_LoadTilesToBgLayer(NARC_INDEX_DEMO__TITLE__OP_DEMO, 16, scene->bgConfig, BG_LAYER_MAIN_2, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilesToBgLayer(NARC_INDEX_DEMO__TITLE__OP_DEMO, 16, scene->bgConfig, BG_LAYER_SUB_2, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayer(NARC_INDEX_DEMO__TITLE__OP_DEMO, 17, scene->bgConfig, BG_LAYER_MAIN_2, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayer(NARC_INDEX_DEMO__TITLE__OP_DEMO, 18, scene->bgConfig, BG_LAYER_SUB_2, 0, 0, FALSE, HEAP_ID_76);

    Graphics_LoadPalette(NARC_INDEX_DEMO__TITLE__OP_DEMO, 15, PAL_LOAD_MAIN_BG, 0, 0, HEAP_ID_76);
    Graphics_LoadPalette(NARC_INDEX_DEMO__TITLE__OP_DEMO, 15, PAL_LOAD_SUB_BG, 0, 0, HEAP_ID_76);

    Graphics_LoadTilesToBgLayer(NARC_INDEX_DEMO__TITLE__TITLEDEMO, 15, scene->bgConfig, BG_LAYER_MAIN_1, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayer(NARC_INDEX_DEMO__TITLE__TITLEDEMO, 13, scene->bgConfig, BG_LAYER_MAIN_1, 0, 0, FALSE, HEAP_ID_76);

    Graphics_LoadPalette(NARC_INDEX_DEMO__TITLE__TITLEDEMO, 14, PAL_LOAD_MAIN_BG, 0, 2 * 16 * 1, HEAP_ID_76);

    Graphics_LoadTilemapToBgLayer(NARC_INDEX_DEMO__TITLE__OP_DEMO, 19, scene->bgConfig, BG_LAYER_MAIN_3, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayer(NARC_INDEX_DEMO__TITLE__OP_DEMO, 19, scene->bgConfig, BG_LAYER_SUB_3, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilesToBgLayer(NARC_INDEX_DEMO__TITLE__OP_DEMO, 114, scene->bgConfig, BG_LAYER_SUB_1, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayer(NARC_INDEX_DEMO__TITLE__OP_DEMO, 113, scene->bgConfig, BG_LAYER_SUB_1, 0, 0, FALSE, HEAP_ID_76);

    Graphics_LoadPalette(NARC_INDEX_DEMO__TITLE__OP_DEMO, 115, PAL_LOAD_SUB_BG, 0, 2 * 16 * 1, HEAP_ID_76);

    GXLayers_EngineAToggleLayers(GX_PLANEMASK_BG2, 0);
    GXLayers_EngineBToggleLayers(GX_PLANEMASK_BG2, 0);

    G2_SetBlendAlpha(GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG3, 0, 16);
    G2S_SetBlendAlpha(GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG3, 0, 16);

    OS_WaitIrq(1, OS_IE_V_BLANK);

    ResetScreenMasterBrightness(DS_SCREEN_MAIN);
    ResetScreenMasterBrightness(DS_SCREEN_SUB);
    GXLayers_TurnBothDispOn();
}

static BOOL LogoScene_Update(LogoScene *scene, const int frame)
{
    u8 *subState = &(scene->subState);

    LogoSparkles_Update(scene->sparkles, frame);

    switch (*subState) {
    case 0:
        scene->fadeTimer = 0;
        scene->fadeLevel = 0;
        scene->fadeDone = FALSE;
        scene->fadeTask = SysTask_Start(LogoScene_FadeInFirstTask, scene, 0);
        Sound_SetSceneAndPlayBGM(SOUND_SCENE_TITLE_SCREEN, SEQ_TITLE00_sseq, 1);
        (*subState)++;
        break;
    case 1:
        if ((scene->fadeDone) && (frame >= LOGO_FRAME_FADE_OUT_FIRST)) {
            scene->fadeTimer = 0;
            scene->fadeLevel = 16;
            scene->fadeDone = FALSE;
            scene->fadeTask = SysTask_Start(LogoScene_FadeOutFirstTask, scene, 0);
            (*subState)++;
        }
        break;
    case 2:
        if ((scene->fadeDone) && (frame >= LOGO_FRAME_SHOW_SECOND)) {
            GXLayers_EngineAToggleLayers(GX_PLANEMASK_BG1, 0);
            GXLayers_EngineBToggleLayers(GX_PLANEMASK_BG1, 0);

            G2_SetBlendAlpha(GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG3, 0, 16);
            G2S_SetBlendAlpha(GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG3, 0, 16);

            GXLayers_EngineAToggleLayers(GX_PLANEMASK_BG2, 1);
            GXLayers_EngineBToggleLayers(GX_PLANEMASK_BG2, 1);

            scene->fadeTimer = 0;
            scene->fadeLevel = 0;
            scene->fadeDone = FALSE;
            scene->fadeTask = SysTask_Start(LogoScene_FadeInSecondMainTask, scene, 0);
            (*subState)++;
        }
        break;
    case 3:
        if (scene->fadeDone) {
            scene->fadeTimer = 0;
            scene->fadeLevel = 0;
            scene->fadeDone = FALSE;
            scene->fadeTask = SysTask_Start(LogoScene_FadeInSecondSubTask, scene, 0);
            (*subState)++;
        }
        break;
    case 4:
        if ((scene->fadeDone) && (frame >= LOGO_FRAME_FADE_TO_BLACK)) {
            StartScreenFade(FADE_SUB_THEN_MAIN, FADE_TYPE_BRIGHTNESS_OUT, FADE_TYPE_BRIGHTNESS_OUT, COLOR_BLACK, 18, 1, HEAP_ID_76);
            (*subState)++;
        }
        break;
    case 5:
        if (IsScreenFadeDone()) {
            return TRUE;
        }
    }

    return FALSE;
}

static void OpeningCutscene_CleanUpLogoScene(OpeningCutscene *cutscene)
{
    SetVBlankCallback(NULL, NULL);

    if (cutscene->logoScene.isSetUp) {
        LogoSparkles_Free(cutscene->logoScene.sparkles);

        Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_MAIN_1);
        Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_MAIN_2);
        Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_SUB_2);
        Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_MAIN_3);
        Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_SUB_3);
        Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_SUB_1);
        Heap_Free(cutscene->bgConfig);

        cutscene->logoScene.isSetUp = FALSE;
    }

    if (cutscene->logoScene.fadeTask != NULL) {
        GF_ASSERT(cutscene->skipRequested);
        SysTask_Done(cutscene->logoScene.fadeTask);
        cutscene->logoScene.fadeTask = NULL;
    }
}

static void LogoScene_FadeInFirstTask(SysTask *task, void *param)
{
    LogoScene *scene = param;

    scene->fadeTimer++;

    if (scene->fadeTimer >= 6) {
        scene->fadeTimer = 0;
        scene->fadeLevel++;
    }

    if (scene->fadeLevel >= 16) {
        SysTask_Done(task);
        scene->fadeTask = NULL;
        scene->fadeDone = TRUE;
    }

    G2_SetBlendAlpha(GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG3, scene->fadeLevel, 16);
    G2S_SetBlendAlpha(GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG3, scene->fadeLevel, 16);
}

static void LogoScene_FadeOutFirstTask(SysTask *task, void *param)
{
    LogoScene *scene = param;

    scene->fadeTimer++;

    if (scene->fadeTimer >= 4) {
        scene->fadeTimer = 0;
        scene->fadeLevel--;
    }

    if (scene->fadeLevel == 0) {
        SysTask_Done(task);
        scene->fadeTask = NULL;
        scene->fadeDone = TRUE;
        *(scene->canSkip) = TRUE;
    }

    G2_SetBlendAlpha(GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG3, scene->fadeLevel, 16);
    G2S_SetBlendAlpha(GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG3, scene->fadeLevel, 16);
}

static void LogoScene_FadeInSecondMainTask(SysTask *task, void *param)
{
    LogoScene *scene = param;

    scene->fadeTimer++;

    if (scene->fadeTimer >= 4) {
        scene->fadeTimer = 0;
        scene->fadeLevel++;
    }

    if (scene->fadeLevel >= 16) {
        SysTask_Done(task);
        scene->fadeTask = NULL;
        scene->fadeDone = TRUE;
    }

    G2_SetBlendAlpha(GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG3, scene->fadeLevel, 16);
}

static void LogoScene_FadeInSecondSubTask(SysTask *task, void *param)
{
    LogoScene *scene = param;

    scene->fadeTimer++;

    if (scene->fadeTimer >= 4) {
        scene->fadeTimer = 0;
        scene->fadeLevel++;
    }

    if (scene->fadeLevel >= 16) {
        SysTask_Done(task);
        scene->fadeTask = NULL;
        scene->fadeDone = TRUE;
    }

    G2S_SetBlendAlpha(GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG3, scene->fadeLevel, 16);
}

static BOOL OpeningCutscene_RunMainScene(OpeningCutscene *cutscene)
{
    u8 *state = &(cutscene->mainScene.state);

    if (cutscene->skipRequested) {
        (*state) = 4;
    }

    switch (*state) {
    case 0:
        OpeningCutscene_SetUpMainScene(cutscene);
        (*state)++;
        break;
    case 1:
        MainScene_LoadGraphics(&cutscene->mainScene);
        (*state)++;
        break;
    case 2:
        if (MainScene_LoadLandscapeStep(&cutscene->mainScene, 0) == TRUE) {
            cutscene->mainScene.showLandscape = TRUE;
            (*state)++;
        }
        break;
    case 3:
        if (MainScene_Update(cutscene, &cutscene->mainScene, cutscene->frame)) {
            (*state)++;
        }

        OpeningSprites_Update(cutscene->mainScene.sprites);

        if (cutscene->mainScene.showLandscape) {
            if (cutscene->mainScene.landscapeLoading == FALSE) {
                MainScene_DrawLandscape(&cutscene->mainScene);
            }
        } else {
            OpeningStarters_Draw(cutscene->mainScene.starters);
        }
        break;
    case 4:
        OpeningCutscene_CleanUpMainScene(cutscene);
        return TRUE;
    }

    return FALSE;
}

static void OpeningCutscene_SetUpMainScene(OpeningCutscene *cutscene)
{
    u8 i;
    int spritesSize;
    int startersSize;

    OpeningCutscene_SetMainVRAMBanks();
    OpeningCutscene_InitLandscapeBgs(cutscene);
    OpeningCutscene_Init3DPipeline(cutscene);

    spritesSize = OpeningSprites_GetSize();
    startersSize = OpeningStarters_GetSize();

    cutscene->mainScene.sprites = Heap_Alloc(HEAP_ID_76, spritesSize);
    cutscene->mainScene.twinkles = TwinklePool_New(TWINKLE_SPRITE_COUNT);
    cutscene->mainScene.starters = Heap_Alloc(HEAP_ID_76, startersSize);

    memset(cutscene->mainScene.sprites, 0, spritesSize);
    memset(cutscene->mainScene.starters, 0, startersSize);

    cutscene->mainScene.twinkleInterval = 4;
    OpeningStarters_Init(cutscene->mainScene.starters);
    cutscene->mainScene.bgConfig = cutscene->bgConfig;

    gSystem.whichScreenIs3D = DS_SCREEN_MAIN;
    GXLayers_SwapDisplay();

    for (i = 0; i < 4; i++) {
        NNS_G3dGlbLightVector(i, sLightVectors[i].x, sLightVectors[i].y, sLightVectors[i].z);
        NNS_G3dGlbLightColor(i, sLightColors[i]);
    }

    NNS_G3dGlbMaterialColorDiffAmb(GX_RGB(16, 16, 16), GX_RGB(10, 12, 12), 0);
    NNS_G3dGlbMaterialColorSpecEmi(GX_RGB(18, 18, 18), GX_RGB(14, 14, 14), 0);
    NNS_G3dGlbPolygonAttr(13, GX_POLYGONMODE_MODULATE, GX_CULL_BACK, 0, 31, GX_POLYGON_ATTR_MISC_FOG);

    SetVBlankCallback(OpeningCutscene_MainVBlank, (void *)cutscene);
    BrightnessController_SetScreenBrightness(-16, GX_BLEND_PLANEMASK_BG3, BRIGHTNESS_MAIN_SCREEN);

    cutscene->mainScene.isSetUp = TRUE;
}

static void OpeningCutscene_FreeLandscape(OpeningCutscene *cutscene)
{
    int i;

    if (cutscene->mainScene.landscapeLoaded || cutscene->mainScene.landscapeLoading) {
        Camera_ClearActive();
        Camera_Delete(cutscene->mainScene.landscape.camera);

        for (i = 0; i < cutscene->mainScene.landscape.modelCount; i++) {
            Heap_Free(cutscene->mainScene.landscape.models[i].modelFile);
        }

        cutscene->mainScene.landscape.modelCount = 0;

        Heap_Free(cutscene->mainScene.landscape.textureFile);

        cutscene->mainScene.landscapeLoaded = FALSE;
        cutscene->mainScene.landscapeLoading = FALSE;
    }
}

static void OpeningCutscene_FreeBgs(OpeningCutscene *cutscene, BOOL freeConfig)
{
    if (cutscene->mainScene.isSetUp && (cutscene->bgConfig != NULL)) {
        if (cutscene->mainScene.bgLayout == BG_LAYOUT_LANDSCAPE) {
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_MAIN_3);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_MAIN_1);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_MAIN_2);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_SUB_0);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_SUB_1);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_SUB_2);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_SUB_3);
        } else if (cutscene->mainScene.bgLayout == BG_LAYOUT_SCROLLING) {
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_MAIN_3);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_MAIN_1);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_MAIN_2);

            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_SUB_1);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_SUB_2);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_SUB_3);
        } else {
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_MAIN_0);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_MAIN_3);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_MAIN_1);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_MAIN_2);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_SUB_0);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_SUB_1);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_SUB_2);
            Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_SUB_3);
        }

        if (freeConfig == TRUE) {
            Heap_Free(cutscene->bgConfig);
            cutscene->bgConfig = NULL;
        }
    }
}

static void OpeningCutscene_CleanUpMainScene(OpeningCutscene *cutscene)
{
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    OpeningCutscene_FreeLandscape(cutscene);

    if (cutscene->mainScene.spritesLoaded) {
        OpeningSprites_Free(cutscene->mainScene.sprites);
        cutscene->mainScene.spritesLoaded = FALSE;
    }

    if (cutscene->mainScene.isSetUp) {
        if (cutscene->mainScene.twinkles != NULL) {
            TwinklePool_Free(cutscene->mainScene.twinkles);
            cutscene->mainScene.twinkles = NULL;
        }

        if (cutscene->mainScene.sprites != NULL) {
            Heap_Free(cutscene->mainScene.sprites);
            cutscene->mainScene.sprites = NULL;
        }

        if (cutscene->mainScene.starters != NULL) {
            OpeningStarters_Free(cutscene->mainScene.starters);
            Heap_Free(cutscene->mainScene.starters);
            cutscene->mainScene.starters = NULL;
        }

        OpeningCutscene_FreeBgs(cutscene, TRUE);

        if (cutscene->mainScene.savedPalette != NULL) {
            Heap_Free(cutscene->mainScene.savedPalette);
            cutscene->mainScene.savedPalette = NULL;
        }

        G3DPipelineBuffers_Free(cutscene->pipeline);
        cutscene->mainScene.isSetUp = FALSE;
    }

    NNS_G3dGeFlushBuffer();

    G3X_SetFog(0, GX_FOGBLEND_COLOR_ALPHA, GX_FOGSLOPE_0x8000, 0);
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);

    SetVBlankCallback(NULL, NULL);
}

static void MainScene_LoadGraphics(MainScene *scene)
{
    NARC *narc = NARC_ctor(NARC_INDEX_DEMO__TITLE__OP_DEMO, HEAP_ID_76);

    {
        Graphics_LoadPaletteWithSrcOffsetFromOpenNARC(narc, 96, PAL_LOAD_MAIN_BG, 0x20 * 0xc, 0x20 * 0xc, 0x20 * 2, HEAP_ID_76);
        Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 95, scene->bgConfig, BG_LAYER_MAIN_3, 0, 0, FALSE, HEAP_ID_76);
        Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 12, scene->bgConfig, BG_LAYER_MAIN_3, 0, 0, FALSE, HEAP_ID_76);

        Graphics_LoadPaletteWithSrcOffsetFromOpenNARC(narc, 13, PAL_LOAD_MAIN_BG, 0x20 * 0xe, 0x20 * 0xe, 0x20 * 2, HEAP_ID_76);
        Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 14, scene->bgConfig, BG_LAYER_MAIN_2, 0, 0, FALSE, HEAP_ID_76);
        Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 12, scene->bgConfig, BG_LAYER_MAIN_2, 0, 0, FALSE, HEAP_ID_76);

        {
            NARC *weatherNarc = NARC_ctor(NARC_INDEX_DATA__WEATHER_SYS, HEAP_ID_76);

            Graphics_LoadTilesToBgLayerFromOpenNARC(weatherNarc, 56, scene->bgConfig, BG_LAYER_MAIN_1, 0, 0, FALSE, HEAP_ID_76);
            Graphics_LoadTilemapToBgLayerFromOpenNARC(weatherNarc, 57, scene->bgConfig, BG_LAYER_MAIN_1, 0, 0, FALSE, HEAP_ID_76);
            Graphics_LoadTilemapToBgLayerFromOpenNARC(weatherNarc, 57, scene->bgConfig, BG_LAYER_MAIN_1, 32 * 32, 0, FALSE, HEAP_ID_76);
            Graphics_LoadPaletteFromOpenNARC(weatherNarc, 55, PAL_LOAD_MAIN_BG, 0x20 * 0, 0x20, HEAP_ID_76);
            NARC_dtor(weatherNarc);
        }
    }

    {
        Graphics_LoadPaletteFromOpenNARC(narc, 61, PAL_LOAD_SUB_BG, 0, 0, HEAP_ID_76);
        Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 59, scene->bgConfig, BG_LAYER_SUB_1, 0, 0, FALSE, HEAP_ID_76);
        Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 63, scene->bgConfig, BG_LAYER_SUB_1, 0, 0, FALSE, HEAP_ID_76);
        Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 58, scene->bgConfig, BG_LAYER_SUB_2, 0, 0, FALSE, HEAP_ID_76);
        Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 62, scene->bgConfig, BG_LAYER_SUB_2, 0, 0, FALSE, HEAP_ID_76);
        Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 60, scene->bgConfig, BG_LAYER_SUB_3, 0, 0, FALSE, HEAP_ID_76);
        Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 64, scene->bgConfig, BG_LAYER_SUB_3, 0, 0, FALSE, HEAP_ID_76);
    }

    {
        Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 14, scene->bgConfig, BG_LAYER_SUB_0, 0, 0, FALSE, HEAP_ID_76);
        Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 12, scene->bgConfig, BG_LAYER_SUB_0, 0, 0, FALSE, HEAP_ID_76);
        Graphics_LoadPaletteWithSrcOffsetFromOpenNARC(narc, 13, PAL_LOAD_SUB_BG, 0x20 * 0xe, 0x20 * 0xe, 0x20 * 2, HEAP_ID_76);
    }

    OpeningSprites_Init(scene->sprites);
    OpeningSprites_CreateSprites(scene->sprites);
    OpeningSprites_CreateTwinkleSprites(scene->sprites, scene->twinkles);

    MI_CpuClear16((void *)HW_BG_PLTT, 2);
    MI_CpuClear16((void *)HW_DB_BG_PLTT, 2);

    GXLayers_EngineAToggleLayers(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, 0);
    GXLayers_EngineBToggleLayers(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, 0);

    ResetScreenMasterBrightness(DS_SCREEN_MAIN);
    ResetScreenMasterBrightness(DS_SCREEN_SUB);
    NARC_dtor(narc);

    scene->spritesLoaded = TRUE;
}

static void MainScene_LoadScrollingBgs(OpeningCutscene *cutscene, MainScene *scene)
{
    NARC *narc;

    Bg_SetControlParam(scene->bgConfig, BG_LAYER_MAIN_2, BG_CONTROL_PARAM_COLOR_MODE, GX_BG_COLORMODE_16);
    Bg_SetControlParam(scene->bgConfig, BG_LAYER_MAIN_3, BG_CONTROL_PARAM_COLOR_MODE, GX_BG_COLORMODE_16);

    G2_SetBG0Priority(0);
    Bg_SetPriority(BG_LAYER_MAIN_1, 3);

    narc = NARC_ctor(NARC_INDEX_DEMO__TITLE__OP_DEMO, HEAP_ID_76);

    Graphics_LoadPaletteFromOpenNARC(narc, 68, PAL_LOAD_MAIN_BG, 0, 0, HEAP_ID_76);

    Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 66, scene->bgConfig, BG_LAYER_MAIN_1, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 70, scene->bgConfig, BG_LAYER_MAIN_1, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 65, scene->bgConfig, BG_LAYER_MAIN_2, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 69, scene->bgConfig, BG_LAYER_MAIN_2, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 67, scene->bgConfig, BG_LAYER_MAIN_3, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 71, scene->bgConfig, BG_LAYER_MAIN_3, 0, 0, FALSE, HEAP_ID_76);

    MI_CpuClear16((void *)HW_BG_PLTT, 2);
    MI_CpuClear16((void *)HW_DB_BG_PLTT, 2);

    Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_1, BG_OFFSET_UPDATE_SET_X, 0);
    Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_2, BG_OFFSET_UPDATE_SET_X, 0);
    Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_3, BG_OFFSET_UPDATE_SET_X, 0);
    Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_1, BG_OFFSET_UPDATE_SET_X, 0);
    Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_2, BG_OFFSET_UPDATE_SET_X, 0);
    Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_3, BG_OFFSET_UPDATE_SET_X, 0);

    NARC_dtor(narc);
}

static void MainScene_LoadStarterBgs(OpeningCutscene *cutscene, MainScene *scene)
{
    NARC *narc;

    OpeningCutscene_InitScrollingBgs(cutscene);

    narc = NARC_ctor(NARC_INDEX_DEMO__TITLE__OP_DEMO, HEAP_ID_76);

    Graphics_LoadPaletteFromOpenNARC(narc, 72, PAL_LOAD_MAIN_BG, 0, 0, HEAP_ID_76);
    Graphics_LoadPaletteFromOpenNARC(narc, 72, PAL_LOAD_SUB_BG, 0, 0, HEAP_ID_76);

    scene->savedPalette = Heap_Alloc(HEAP_ID_76, 0x200);

    MI_CpuCopy16((void *)HW_BG_PLTT, scene->savedPalette, 0x200);
    MI_CpuClear16(scene->savedPalette, 2);

    Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 73, scene->bgConfig, BG_LAYER_MAIN_1, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 76, scene->bgConfig, BG_LAYER_MAIN_1, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 73, scene->bgConfig, BG_LAYER_SUB_1, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 76, scene->bgConfig, BG_LAYER_SUB_1, 0, 0, FALSE, HEAP_ID_76);

    Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 74, scene->bgConfig, BG_LAYER_MAIN_2, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 77, scene->bgConfig, BG_LAYER_MAIN_2, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 74, scene->bgConfig, BG_LAYER_SUB_2, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 77, scene->bgConfig, BG_LAYER_SUB_2, 0, 0, FALSE, HEAP_ID_76);

    Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 75, scene->bgConfig, BG_LAYER_MAIN_3, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 78, scene->bgConfig, BG_LAYER_MAIN_3, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 75, scene->bgConfig, BG_LAYER_SUB_3, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 78, scene->bgConfig, BG_LAYER_SUB_3, 0, 0, FALSE, HEAP_ID_76);

    MI_CpuClear16((void *)HW_BG_PLTT, 2);
    MI_CpuClear16((void *)HW_DB_BG_PLTT, 2);

    Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_1, BG_OFFSET_UPDATE_SET_X, -128);
    Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_2, BG_OFFSET_UPDATE_SET_X, -200);
    Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_3, BG_OFFSET_UPDATE_SET_X, -152);
    Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_1, BG_OFFSET_UPDATE_SET_X, 128);
    Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_2, BG_OFFSET_UPDATE_SET_X, 0x18 * 8);
    Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_3, BG_OFFSET_UPDATE_SET_X, 0x16 * 8);

    NARC_dtor(narc);
}

static void MainScene_LoadFinalBgs(OpeningCutscene *cutscene, MainScene *scene)
{
    NARC *narc;

    OpeningCutscene_InitOverlayBgs(cutscene);

    narc = NARC_ctor(NARC_INDEX_DEMO__TITLE__OP_DEMO, HEAP_ID_76);

    Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 79, scene->bgConfig, BG_LAYER_MAIN_0, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 80, scene->bgConfig, BG_LAYER_MAIN_0, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilesToBgLayerFromOpenNARC(narc, 81, scene->bgConfig, BG_LAYER_SUB_0, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayerFromOpenNARC(narc, 82, scene->bgConfig, BG_LAYER_SUB_0, 0, 0, FALSE, HEAP_ID_76);

    NARC_dtor(narc);
}

// Places the scrolling layers on whichever screen is currently showing the 3D scene.
static void MainScene_ArrangeLayers(MainScene *scene)
{
    if (gSystem.whichScreenIs3D == DS_SCREEN_MAIN) {
        Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_1, BG_OFFSET_UPDATE_SET_Y, 0);
        Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_2, BG_OFFSET_UPDATE_SET_Y, 0);
        Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_3, BG_OFFSET_UPDATE_SET_Y, 0);
        Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_1, BG_OFFSET_UPDATE_SET_Y, OFFSCREEN_BG_OFFSET);
        Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_2, BG_OFFSET_UPDATE_SET_Y, OFFSCREEN_BG_OFFSET);
        Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_3, BG_OFFSET_UPDATE_SET_Y, OFFSCREEN_BG_OFFSET);
    } else {
        Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_1, BG_OFFSET_UPDATE_SET_Y, OFFSCREEN_BG_OFFSET);
        Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_2, BG_OFFSET_UPDATE_SET_Y, OFFSCREEN_BG_OFFSET);
        Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_3, BG_OFFSET_UPDATE_SET_Y, OFFSCREEN_BG_OFFSET);
        Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_1, BG_OFFSET_UPDATE_SET_Y, 0);
        Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_2, BG_OFFSET_UPDATE_SET_Y, 0);
        Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_3, BG_OFFSET_UPDATE_SET_Y, 0);
    }

    Bg_SetPriority(BG_LAYER_MAIN_1, 1);
    Bg_SetPriority(BG_LAYER_MAIN_2, 2);
    Bg_SetPriority(BG_LAYER_MAIN_3, 0);
    Bg_SetPriority(BG_LAYER_SUB_1, 1);
    Bg_SetPriority(BG_LAYER_SUB_2, 2);
    Bg_SetPriority(BG_LAYER_SUB_3, 0);

    OpeningSprites_SetBandPalettes(scene->sprites, FALSE);
}

static void MainScene_LoadLandscapeTexture(MainScene *scene, int landscapeIndex)
{
    BOOL uploaded;
    u8 *textureData;
    u32 trimmedSize;

    scene->landscape.textureFile = NARC_AllocAndReadWholeMemberByIndexPair(NARC_INDEX_DEMO__TITLE__OP_DEMO, sLandscapeTextureIDs[landscapeIndex], HEAP_ID_76);
    scene->landscape.texture = NNS_G3dGetTex((NNSG3dResFileHeader *)scene->landscape.textureFile);

    uploaded = Easy3D_UploadTextureToVRAM(scene->landscape.texture);
    GF_ASSERT(uploaded);

    GF_ASSERT(scene->landscape.texture->texInfo.ofsTex != 0);

    textureData = (u8 *)scene->landscape.texture + scene->landscape.texture->texInfo.ofsTex;
    trimmedSize = (u32)(textureData - (u8 *)scene->landscape.textureFile);

    Heap_Realloc(scene->landscape.textureFile, trimmedSize);
}

static void MainScene_LoadLandscapeModels(MainScene *scene, int landscapeIndex, int first, int end)
{
    u8 i;
    NARC *narc;
    BOOL bound;
    NNSG3dResMdlSet *modelSet;
    NNSG3dResMdl *model;

    if (end > MAX_LANDSCAPE_MODELS) {
        end = MAX_LANDSCAPE_MODELS;
    }

    narc = NARC_ctor(NARC_INDEX_DEMO__TITLE__OP_DEMO, HEAP_ID_76);

    for (i = first; i < end; i++) {
        if (sLandscapeModelIDs[landscapeIndex][i] == 0) {
            break;
        }

        scene->landscape.models[i].modelFile = NARC_AllocAndReadWholeMember(narc, sLandscapeModelIDs[landscapeIndex][i], HEAP_ID_76);

        bound = Easy3D_BindTextureToResource(scene->landscape.models[i].modelFile, scene->landscape.texture);
        GF_ASSERT(bound);

        modelSet = NNS_G3dGetMdlSet(scene->landscape.models[i].modelFile);
        GF_ASSERT(modelSet->dict.numEntry == 1);

        model = NNS_G3dGetMdlByIdx(NNS_G3dGetMdlSet(scene->landscape.models[i].modelFile), 0);
        GF_ASSERT(model);

        NNS_G3dRenderObjInit(&scene->landscape.models[i].renderObj, model);
        NNS_G3dMdlUseGlbDiff(model);
        NNS_G3dMdlUseGlbAmb(model);
        NNS_G3dMdlUseGlbSpec(model);
        NNS_G3dMdlUseGlbEmi(model);
        NNS_G3dMdlUseGlbLightEnableFlag(model);

        scene->landscape.modelCount++;
    }

    NARC_dtor(narc);
}

static void MainScene_InitLandscapeCamera(MainScene *scene)
{
    scene->landscape.camera = Camera_Alloc(HEAP_ID_76);

    {
        VecFx32 target = { 0, 0, FX32_ONE * 16 * 6 };

        Camera_InitWithTarget(&target, sLandscapeCameraConfig.distance, &sLandscapeCameraConfig.cameraAngle, sLandscapeCameraConfig.fovY, sLandscapeCameraConfig.projectionMtx, 0, scene->landscape.camera);
        Camera_SetFOV(0x981, scene->landscape.camera);

        {
            VecFx32 offset = { 0, FX32_ONE * 16 * 5, 0 };
            Camera_Move(&offset, scene->landscape.camera);
        }

        Camera_SetAsActive(scene->landscape.camera);
    }
}

// Loads the whole landscape in one go.
static void MainScene_LoadLandscape(MainScene *scene, int landscapeIndex)
{
    MainScene_LoadLandscapeTexture(scene, landscapeIndex);
    MainScene_LoadLandscapeModels(scene, landscapeIndex, 0, MAX_LANDSCAPE_MODELS);
    MainScene_InitLandscapeCamera(scene);

    scene->landscapeLoaded = TRUE;
}

// Loads the landscape spread over several frames; returns TRUE once it is complete.
static BOOL MainScene_LoadLandscapeStep(MainScene *scene, int landscapeIndex)
{
    if (landscapeIndex == 0) {
        switch (scene->landscapeLoadStep) {
        case 0:
            scene->landscapeLoading = TRUE;
            MainScene_LoadLandscapeTexture(scene, landscapeIndex);
            MainScene_InitLandscapeCamera(scene);
            break;
        case 1:
            MainScene_LoadLandscapeModels(scene, landscapeIndex, 0, 1);
            break;
        case 2:
        default:
            MainScene_LoadLandscapeModels(scene, landscapeIndex, 1, MAX_LANDSCAPE_MODELS);
            scene->landscapeLoadStep = 0;
            scene->landscapeLoading = FALSE;
            scene->landscapeLoaded = TRUE;
            return TRUE;
        }
    } else {
        switch (scene->landscapeLoadStep) {
        case 0:
            scene->landscapeLoading = TRUE;
            MainScene_LoadLandscapeTexture(scene, landscapeIndex);
            MainScene_InitLandscapeCamera(scene);
            break;
        case 1:
            MainScene_LoadLandscapeModels(scene, landscapeIndex, 0, 2);
            break;
        case 2:
            MainScene_LoadLandscapeModels(scene, landscapeIndex, 2, 4);
            break;
        case 3:
        default:
            MainScene_LoadLandscapeModels(scene, landscapeIndex, 4, MAX_LANDSCAPE_MODELS);
            scene->landscapeLoadStep = 0;
            scene->landscapeLoading = FALSE;
            scene->landscapeLoaded = TRUE;
            return TRUE;
        }
    }

    scene->landscapeLoadStep++;
    return FALSE;
}

static void MainScene_DrawLandscape(MainScene *scene)
{
    u8 i;
    MtxFx33 rotation = { FX32_ONE, 0, 0, 0, FX32_ONE, 0, 0, 0, FX32_ONE };
    VecFx32 scale = { FX32_ONE, FX32_ONE, FX32_ONE };
    VecFx32 translation = { 0, 0, 0 };

    NNS_G3dGeFlushBuffer();

    G3_ResetG3X();
    Camera_ComputeViewMatrix();

    if (scene->drawLandscape == TRUE) {
        for (i = 0; i < scene->landscape.modelCount; i++) {
            DC_FlushAll();
            Easy3D_DrawRenderObj(&scene->landscape.models[i].renderObj, &translation, &rotation, &scale);
        }
    }

    if (scene->drawParticles == TRUE) {
        NNS_G3dGeFlushBuffer();
        G3_ResetG3X();
        ParticleSystem_DrawAll();
        ParticleSystem_UpdateAll();
    }

    G3_RequestSwapBuffers(GX_SORTMODE_AUTO, GX_BUFFERMODE_W);
}

static BOOL MainScene_Update(OpeningCutscene *cutscene, MainScene *scene, const int frame)
{
    u8 *subState = &(scene->subState);

    if (!scene->twinkleTimer) {
        TwinklePool_Spawn(scene->twinkles, scene->hideTwinkles);
    }

    scene->twinkleTimer = (scene->twinkleTimer + 1) % scene->twinkleInterval;

    switch (*subState) {
    case 0:
        // Fade in the overlay, revealing the sparkling sub screen
        if (frame >= MAIN_FRAME_FADE_IN_OVERLAY) {
            Bg_ToggleLayer(BG_LAYER_MAIN_3, TRUE);
            BrightnessController_StartTransition(8, 0, -16, GX_BLEND_PLANEMASK_BG3, BRIGHTNESS_MAIN_SCREEN);
            (*subState)++;
        }
        break;
    case 1:
        if ((BrightnessController_IsTransitionComplete(BRIGHTNESS_MAIN_SCREEN) == TRUE) && (frame == MAIN_FRAME_START_PARTICLES)) {
            Bg_ToggleLayer(BG_LAYER_MAIN_0, TRUE);
            OpeningStarters_CreateEmitter(scene->starters, 9);
            OpeningStarters_CreateEmitter(scene->starters, 10);
            OpeningStarters_CreateEmitter(scene->starters, 11);
            G2_SetBlendAlpha(0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3, 0, 0);
            scene->drawParticles = TRUE;
            OpeningSprites_SetLeftRightAnimating(scene->sprites, TRUE);
            (*subState)++;
        }
        break;
    case 2:
        if (frame == MAIN_FRAME_FLASH_OUT) {
            StartScreenFade(FADE_MAIN_ONLY, FADE_TYPE_BRIGHTNESS_OUT, FADE_TYPE_BRIGHTNESS_OUT, COLOR_WHITE, 4, 1, HEAP_ID_76);
        }

        if (frame == MAIN_FRAME_FLASH_IN) {
            GF_ASSERT(IsScreenFadeDone() == TRUE);
            scene->showLayersMask = (1 << 2);
            scene->hideLayersMask = (1 << 3);
            StartScreenFade(FADE_MAIN_ONLY, FADE_TYPE_BRIGHTNESS_IN, FADE_TYPE_BRIGHTNESS_IN, COLOR_WHITE, 4, 1, HEAP_ID_76);
        }

        if (frame == MAIN_FRAME_PREPARE_LANDSCAPE) {
            GXLayers_EngineAToggleLayers(GX_PLANEMASK_BG0, 0);
            scene->swapScreensPending = TRUE;
            scene->drawParticles = FALSE;
            scene->drawLandscape = TRUE;
            GF_ASSERT(ParticleSystem_GetActiveEmitterCount(OpeningStarters_GetParticleSystem(scene->starters)) == 0);
        }

        if (frame == MAIN_FRAME_SHOW_SUB_OBJ) {
            GXLayers_EngineBToggleLayers(GX_PLANEMASK_OBJ, 1);
        }

        if (frame >= MAIN_FRAME_START_LANDSCAPE) {
            GXLayers_EngineAToggleLayers(GX_PLANEMASK_BG0, 1);
            StartScreenFade(FADE_MAIN_ONLY, FADE_TYPE_BRIGHTNESS_IN, FADE_TYPE_BRIGHTNESS_IN, COLOR_WHITE, 18, 1, HEAP_ID_76);
            scene->twinkleInterval = 16;
            (*subState)++;
        }
        break;
    case 3:
        // Landscape fly-over
        MainScene_UpdateLandscapeCuts(cutscene, scene, frame);
        MainScene_UpdateLandscapeCamera(scene, frame);
        OpeningSprites_UpdateFlock(scene->sprites, frame);

        OpeningSprites_UpdateSlideFromLeft(scene->sprites, frame);
        OpeningSprites_UpdateSlideFromRight(scene->sprites, frame);

        if (frame >= LANDSCAPE_FRAME_FADE_TO_WHITE) {
            StartScreenFade(FADE_BOTH_SCREENS, FADE_TYPE_BRIGHTNESS_OUT, FADE_TYPE_BRIGHTNESS_OUT, COLOR_WHITE, 18, 1, HEAP_ID_76);
            (*subState)++;
        }

        break;
    case 4:
        MainScene_UpdateLandscapeCamera(scene, frame);
        OpeningSprites_UpdateFlock(scene->sprites, frame);
        OpeningSprites_UpdateSlideFromLeft(scene->sprites, frame);
        OpeningSprites_UpdateSlideFromRight(scene->sprites, frame);

        if (IsScreenFadeDone() && (frame >= LANDSCAPE_FRAME_END)) {
            Bg_ToggleLayer(BG_LAYER_SUB_0, FALSE);
            OpeningSprites_HideTwinkleSprites(scene->twinkles);
            scene->hideTwinkles = TRUE;
            MainScene_LoadScrollingBgs(cutscene, scene);
            OpeningSprites_SetVisible(scene->sprites, OPENING_SPRITE_LEFT_SUB, TRUE);
            OpeningSprites_SetVisible(scene->sprites, OPENING_SPRITE_RIGHT_MAIN, TRUE);

            Bg_ToggleLayer(BG_LAYER_MAIN_1, TRUE);
            Bg_ToggleLayer(BG_LAYER_MAIN_2, TRUE);
            Bg_ToggleLayer(BG_LAYER_MAIN_3, TRUE);
            Bg_ToggleLayer(BG_LAYER_SUB_1, TRUE);
            Bg_ToggleLayer(BG_LAYER_SUB_2, TRUE);
            Bg_ToggleLayer(BG_LAYER_SUB_3, TRUE);
            Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_1, BG_OFFSET_UPDATE_SET_X, 256);
            Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_1, BG_OFFSET_UPDATE_SET_Y, 0);
            Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_2, BG_OFFSET_UPDATE_SET_X, 0);
            Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_2, BG_OFFSET_UPDATE_SET_Y, 256);
            Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_3, BG_OFFSET_UPDATE_SET_X, 0);
            Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_3, BG_OFFSET_UPDATE_SET_Y, 0);
            Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_1, BG_OFFSET_UPDATE_SET_X, 256);
            Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_1, BG_OFFSET_UPDATE_SET_Y, 0);
            Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_2, BG_OFFSET_UPDATE_SET_X, 0);
            Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_2, BG_OFFSET_UPDATE_SET_Y, 256);
            Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_3, BG_OFFSET_UPDATE_SET_X, 0);
            Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_3, BG_OFFSET_UPDATE_SET_Y, 0);

            scene->bg2ScrollY = 256 << FX32_SHIFT;
            scene->mainWndLeft = 0;
            scene->mainWndRight = 255;
            scene->mainWndTop = 32;
            scene->mainWndBottom = 159;
            scene->subWndLeft = 0;
            scene->subWndRight = 255;
            scene->subWndTop = 32;
            scene->subWndBottom = 159;
            scene->enableWindowsPending = TRUE;
            scene->showLandscape = FALSE;

            G2_BlendNone();
            G2S_BlendNone();
            StartScreenFade(FADE_BOTH_SCREENS, FADE_TYPE_BRIGHTNESS_IN, FADE_TYPE_BRIGHTNESS_IN, COLOR_WHITE, 18, 1, HEAP_ID_76);
            (*subState)++;
        }
        break;
    case 5:
        // Scrolling backgrounds
        MainScene_UpdateScrollingBgs(scene);
        OpeningSprites_SpreadLeftRight(scene->sprites);

        if (IsScreenFadeDone()) {
            if (frame >= SCROLL_FRAME_END) {
                Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_MAIN_3, BG_OFFSET_UPDATE_SET_X, 0);
                Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_MAIN_3, BG_OFFSET_UPDATE_SET_Y, 256);
                Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_SUB_3, BG_OFFSET_UPDATE_SET_X, 0);
                Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_SUB_3, BG_OFFSET_UPDATE_SET_Y, 256);
                Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_MAIN_2, BG_OFFSET_UPDATE_SET_X, 0);
                Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_MAIN_2, BG_OFFSET_UPDATE_SET_Y, 0);
                Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_SUB_2, BG_OFFSET_UPDATE_SET_X, 0);
                Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_SUB_2, BG_OFFSET_UPDATE_SET_Y, 0);

                scene->hideLayersMask = (1 << 1) | (1 << 5);
                scene->disableWindowsPending = TRUE;

                OpeningSprites_SetVisible(scene->sprites, OPENING_SPRITE_LEFT_SUB, FALSE);
                OpeningSprites_SetVisible(scene->sprites, OPENING_SPRITE_RIGHT_MAIN, FALSE);
                (*subState)++;
            }
        }
        break;
    case 6:
        MainScene_ScrollBackground(scene);
        MainScene_UpdateSlidingBgs(scene, frame);

        if (frame == SCROLL_FRAME_FADE_OUT) {
            StartScreenFade(FADE_BOTH_SCREENS, FADE_TYPE_BRIGHTNESS_OUT, FADE_TYPE_BRIGHTNESS_OUT, COLOR_BLACK, 4, 1, HEAP_ID_76);
            (*subState)++;
        }
        break;
    case 7:
        MainScene_ScrollBackground(scene);
        MainScene_UpdateSlidingBgs(scene, frame);

        if (IsScreenFadeDone()) {
            scene->showLayersMask = (1 << 1) | (1 << 5);
            scene->hideLayersMask = (1 << 2) | (1 << 6);
            Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_MAIN_1, BG_OFFSET_UPDATE_SET_X, 0);
            Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_MAIN_1, BG_OFFSET_UPDATE_SET_Y, 0);
            Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_SUB_1, BG_OFFSET_UPDATE_SET_X, 0);
            Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_SUB_1, BG_OFFSET_UPDATE_SET_Y, 0);
            StartScreenFade(FADE_BOTH_SCREENS, FADE_TYPE_BRIGHTNESS_IN, FADE_TYPE_BRIGHTNESS_IN, COLOR_BLACK, 4, 1, HEAP_ID_76);
            (*subState)++;
        }
        break;
    case 8:
        MainScene_ScrollBackground(scene);
        MainScene_UpdateSlidingBgs(scene, frame);
        OpeningSprites_UpdateSpinners(scene->sprites, frame);

        if (IsScreenFadeDone()) {
            if (frame == SCROLL_FRAME_SPIN_START) {
                Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_MAIN_1, BG_OFFSET_UPDATE_SET_X, 0);
                Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_MAIN_1, BG_OFFSET_UPDATE_SET_Y, 256);
                Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_SUB_1, BG_OFFSET_UPDATE_SET_X, 0);
                Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_SUB_1, BG_OFFSET_UPDATE_SET_Y, 256);
                OpeningSprites_SetVisible(scene->sprites, OPENING_SPRITE_SPINNER_MAIN, TRUE);
                OpeningSprites_SetVisible(scene->sprites, OPENING_SPRITE_SPINNER_SUB, TRUE);
            } else if (frame >= SCROLL_FRAME_SLIDE_OUT) {
                StartScreenFade(FADE_BOTH_SCREENS, FADE_TYPE_BRIGHTNESS_OUT, FADE_TYPE_BRIGHTNESS_OUT, COLOR_WHITE, 4, 1, HEAP_ID_76);
                (*subState)++;
            }
        }
        break;
    case 9:
        OpeningSprites_UpdateSpinners(scene->sprites, frame);

        if (IsScreenFadeDone()) {
            StartScreenFade(FADE_BOTH_SCREENS, FADE_TYPE_BRIGHTNESS_IN, FADE_TYPE_BRIGHTNESS_IN, COLOR_WHITE, 64, 1, HEAP_ID_76);
            OpeningSprites_SetVisible(scene->sprites, OPENING_SPRITE_SPINNER_MAIN, FALSE);
            OpeningSprites_SetVisible(scene->sprites, OPENING_SPRITE_SPINNER_SUB, FALSE);
            OpeningSprites_SetVisible(scene->sprites, OPENING_SPRITE_BAND_MAIN_A, TRUE);
            OpeningSprites_SetVisible(scene->sprites, OPENING_SPRITE_BAND_MAIN_B, TRUE);
            OpeningSprites_SetVisible(scene->sprites, OPENING_SPRITE_BAND_SUB_A, TRUE);
            OpeningSprites_SetVisible(scene->sprites, OPENING_SPRITE_BAND_SUB_B, TRUE);
            OpeningSprites_SetBandPalettes(scene->sprites, FALSE);
            MainScene_LoadStarterBgs(cutscene, scene);
            gSystem.whichScreenIs3D = DS_SCREEN_MAIN;
            GXLayers_SwapDisplay();
            MainScene_ArrangeLayers(scene);
            scene->disableWindowsPending = TRUE;
            (*subState)++;
        } else {
            MainScene_ScrollBackground(scene);
            MainScene_UpdateSlidingBgs(scene, frame);
        }
        break;
    case 10:
        // Starters part: backgrounds slide in
        OpeningSprites_ScrollBands(scene->sprites, 16 << FX32_SHIFT);
        MainScene_SlideBgsInStaggered(scene, frame);

        if (IsScreenFadeDone()) {
            if (frame >= SCROLL_FRAME_STARTERS_START) {
                StartScreenFade(FADE_MAIN_ONLY, FADE_TYPE_BRIGHTNESS_OUT, FADE_TYPE_BRIGHTNESS_OUT, COLOR_BLACK, 4, 1, HEAP_ID_76);
                (*subState)++;
            }
        }
        break;
    case 11:
        OpeningSprites_ScrollBands(scene->sprites, 16 << FX32_SHIFT);

        if (IsScreenFadeDone()) {
            G2_SetBlendAlpha(GX_BLEND_PLANEMASK_NONE, GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD, 0, 0);
            MI_CpuClear16((void *)HW_BG_PLTT, 0x200);
            StartScreenFade(FADE_MAIN_ONLY, FADE_TYPE_BRIGHTNESS_IN, FADE_TYPE_BRIGHTNESS_IN, COLOR_BLACK, 4, 1, HEAP_ID_76);
            (*subState)++;
        }
        break;
    case 12:
        // First starter appears
        OpeningSprites_ScrollBands(scene->sprites, 16 << FX32_SHIFT);

        if (IsScreenFadeDone()) {
            if (OpeningStarters_UpdateAppearance(scene->starters, 0)) {
                StartScreenFade(FADE_BOTH_SCREENS, FADE_TYPE_BRIGHTNESS_OUT, FADE_TYPE_BRIGHTNESS_OUT, COLOR_BLACK, 4, 1, HEAP_ID_76);
                (*subState)++;
            }
        }
        break;
    case 13:
        OpeningSprites_ScrollBands(scene->sprites, 16 << FX32_SHIFT);

        if (IsScreenFadeDone()) {
            gSystem.whichScreenIs3D = DS_SCREEN_SUB;
            GXLayers_SwapDisplay();
            MainScene_ArrangeLayers(scene);

            StartScreenFade(FADE_BOTH_SCREENS, FADE_TYPE_BRIGHTNESS_IN, FADE_TYPE_BRIGHTNESS_IN, COLOR_BLACK, 4, 1, HEAP_ID_76);
            (*subState)++;
        }
        break;
    case 14:
        // Second starter appears
        OpeningSprites_ScrollBands(scene->sprites, 16 << FX32_SHIFT);

        if (IsScreenFadeDone()) {
            if (frame >= SCROLL_FRAME_STARTER_2) {
                if (OpeningStarters_UpdateAppearance(scene->starters, 1)) {
                    StartScreenFade(FADE_BOTH_SCREENS, FADE_TYPE_BRIGHTNESS_OUT, FADE_TYPE_BRIGHTNESS_OUT, COLOR_BLACK, 4, 1, HEAP_ID_76);
                    (*subState)++;
                }
            }
        }
        break;
    case 15:
        OpeningSprites_ScrollBands(scene->sprites, 16 << FX32_SHIFT);

        if (IsScreenFadeDone()) {
            MI_CpuCopy16(scene->savedPalette, (void *)HW_BG_PLTT, 0x200);

            gSystem.whichScreenIs3D = DS_SCREEN_MAIN;
            GXLayers_SwapDisplay();

            MainScene_LoadFinalBgs(cutscene, scene);
            MainScene_ArrangeLayers(scene);
            OpeningSprites_SetBandPalettes(scene->sprites, TRUE);

            Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_MAIN_1, BG_OFFSET_UPDATE_SET_X, -60);
            Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_MAIN_2, BG_OFFSET_UPDATE_SET_X, -60);
            Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_MAIN_3, BG_OFFSET_UPDATE_SET_X, -60);
            Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_SUB_1, BG_OFFSET_UPDATE_SET_X, 60);
            Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_SUB_2, BG_OFFSET_UPDATE_SET_X, 60);
            Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_SUB_3, BG_OFFSET_UPDATE_SET_X, 60);
            StartScreenFade(FADE_MAIN_ONLY, FADE_TYPE_BRIGHTNESS_IN, FADE_TYPE_BRIGHTNESS_IN, COLOR_BLACK, 16, 1, HEAP_ID_76);
            (*subState)++;
        }
        break;
    case 16:
        OpeningSprites_ScrollBands(scene->sprites, 16 << FX32_SHIFT);

        if (IsScreenFadeDone()) {
            StartScreenFade(FADE_SUB_ONLY, FADE_TYPE_BRIGHTNESS_IN, FADE_TYPE_BRIGHTNESS_IN, COLOR_BLACK, 16, 1, HEAP_ID_76);
            (*subState)++;
        }
        break;
    case 17:
        OpeningSprites_ScrollBands(scene->sprites, 16 << FX32_SHIFT);

        if (IsScreenFadeDone()) {
            if (frame >= SCROLL_FRAME_FINAL_SLIDE_IN) {
                MainScene_SlideBgsIn(scene);
            }

            if (frame >= SCROLL_FRAME_FINAL_SLIDE_IN + 1) {
                scene->showLayersMask |= (1 << 0) | (1 << 4);

                StartScreenFade(FADE_BOTH_SCREENS, FADE_TYPE_BRIGHTNESS_OUT, FADE_TYPE_BRIGHTNESS_OUT, COLOR_WHITE, 6, 1, HEAP_ID_76);
                (*subState)++;
            }
        }
        break;
    case 18:
        MainScene_SlideBgsIn(scene);
        OpeningSprites_ScrollBands(scene->sprites, 16 << FX32_SHIFT);

        if (IsScreenFadeDone()) {
            gSystem.whichScreenIs3D = DS_SCREEN_SUB;
            GXLayers_SwapDisplay();
            (*subState)++;
        }
        break;
    case 19:
        return TRUE;
    }

    return FALSE;
}

static void MainScene_UpdateScrollingBgs(MainScene *scene)
{
    scene->bg2ScrollY += 0x280;
    scene->bg1ScrollY += 0x110;

    Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_1, BG_OFFSET_UPDATE_SET_Y, -(scene->bg1ScrollY >> FX32_SHIFT));
    Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_2, BG_OFFSET_UPDATE_SET_Y, -(scene->bg2ScrollY >> FX32_SHIFT));

    Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_1, BG_OFFSET_UPDATE_SET_Y, -(scene->bg1ScrollY >> FX32_SHIFT));
    Bg_SetOffset(scene->bgConfig, BG_LAYER_SUB_2, BG_OFFSET_UPDATE_SET_Y, -(scene->bg2ScrollY >> FX32_SHIFT));
}

static void MainScene_ScrollBackground(MainScene *scene)
{
    Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_MAIN_3, BG_OFFSET_UPDATE_ADD_X, 16);
    Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_SUB_3, BG_OFFSET_UPDATE_ADD_X, -16);
}

static void MainScene_UpdateSlidingBgs(MainScene *scene, int frame)
{
    if (frame == SCROLL_FRAME_SPIN_START) {
        G2_SetWnd0InsidePlane(ALL_WND_PLANES, TRUE);
        G2_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ, TRUE);

        scene->mainWndLeft = 0;
        scene->mainWndRight = 255;
        scene->mainWndTop = 0;
        scene->mainWndBottom = 8 * 0x11;

        G2_SetWnd0Position(scene->mainWndLeft, scene->mainWndTop, scene->mainWndRight, scene->mainWndBottom);
        GX_SetVisibleWnd(GX_WNDMASK_W0);
        G2S_SetWnd0InsidePlane(ALL_WND_PLANES, TRUE);
        G2S_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ, TRUE);
        G2S_SetWnd1InsidePlane(ALL_WND_PLANES, TRUE);

        scene->subWndLeft = 0;
        scene->subWndRight = 255;
        scene->subWndTop = 0;
        scene->subWndBottom = 8 * 0x11;

        G2S_SetWnd0Position(scene->subWndLeft, scene->subWndTop, scene->subWndRight, scene->subWndBottom);
        G2S_SetWnd1Position(1, scene->subWndTop, 0, scene->subWndBottom);
        GXS_SetVisibleWnd(GX_WNDMASK_W0 | GX_WNDMASK_W1);
    } else if (frame > SCROLL_FRAME_SPIN_START) {
        if (Bg_GetXOffset2(scene->bgConfig, BG_LAYER_MAIN_1) >= 256) {
            scene->hideLayersMask |= (1 << 1);
        }

        if (Bg_GetXOffset2(scene->bgConfig, BG_LAYER_SUB_1) <= -256) {
            scene->hideLayersMask |= (1 << 5);
        }

        Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_MAIN_1, BG_OFFSET_UPDATE_ADD_X, 12);
        Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_MAIN_1, BG_OFFSET_UPDATE_ADD_Y, -1);
        Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_SUB_1, BG_OFFSET_UPDATE_ADD_X, -12);
        Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_SUB_1, BG_OFFSET_UPDATE_ADD_Y, -1);
    }
}

// Slides a main screen layer towards an X offset of 0 from the left, and its sub screen counterpart from the right.
static void MainScene_SlideLayerPairIn(MainScene *scene, enum BgLayer mainLayer, enum BgLayer subLayer, int speed)
{
    int offset;

    offset = Bg_GetXOffset(scene->bgConfig, mainLayer) + speed;

    if (offset > 0) {
        offset = 0;
    }

    Bg_ScheduleScroll(scene->bgConfig, mainLayer, BG_OFFSET_UPDATE_SET_X, offset);
    offset = Bg_GetXOffset(scene->bgConfig, subLayer) - speed;

    if (offset < 0) {
        offset = 0;
    }

    Bg_ScheduleScroll(scene->bgConfig, subLayer, BG_OFFSET_UPDATE_SET_X, offset);
}

static void MainScene_SlideBgsInStaggered(MainScene *scene, int frame)
{
    if (frame > SCROLL_FRAME_SLIDE_OUT_2) {
        MainScene_SlideLayerPairIn(scene, BG_LAYER_MAIN_1, BG_LAYER_SUB_1, 20);
    }

    if (frame >= SCROLL_FRAME_SLIDE_OUT_3) {
        MainScene_SlideLayerPairIn(scene, BG_LAYER_MAIN_2, BG_LAYER_SUB_2, 20);
    }

    if (frame >= SCROLL_FRAME_SLIDE_OUT_4) {
        MainScene_SlideLayerPairIn(scene, BG_LAYER_MAIN_3, BG_LAYER_SUB_3, 20);
    }
}

static void MainScene_SlideBgsIn(MainScene *scene)
{
    MainScene_SlideLayerPairIn(scene, BG_LAYER_MAIN_1, BG_LAYER_SUB_1, 12);
    MainScene_SlideLayerPairIn(scene, BG_LAYER_MAIN_2, BG_LAYER_SUB_2, 12);
    MainScene_SlideLayerPairIn(scene, BG_LAYER_MAIN_3, BG_LAYER_SUB_3, 12);
}

static void MainScene_UpdateLandscapeCamera(MainScene *scene, const int frame)
{
    VecFx32 movement = { 0, 0, 0 };

    if (frame < LANDSCAPE_FRAME_CUT_1) {
        {
            u16 fov;
            fov = Camera_GetFOV(scene->landscape.camera);

            if (fov - 32 > 0x5c1) {
                Camera_AdjustFOV(-32, scene->landscape.camera);
            } else {
                Camera_SetFOV(0x5c1, scene->landscape.camera);
            }
        }

        movement.z -= FX32_ONE * 2;
        Camera_Move(&movement, scene->landscape.camera);
    } else if (frame == LANDSCAPE_FRAME_CUT_1) {
        VecFx32 target = { -(FX32_ONE * 16 * 4), 0, -FX32_ONE * 16 * 3 };
        Camera_InitWithTarget(&target, sLandscapeCameraConfig.distance, &sLandscapeCameraConfig.cameraAngle, sLandscapeCameraConfig.fovY, sLandscapeCameraConfig.projectionMtx, 0, scene->landscape.camera);

        {
            VecFx32 offset = { 0, FX32_ONE * 16 * 5, 0 };
            Camera_Move(&offset, scene->landscape.camera);
        }

        {
            NNS_G3dGeFlushBuffer();

            G3X_SetFog(1, GX_FOGBLEND_COLOR_ALPHA, GX_FOGSLOPE_0x1000, 4500);
            G3X_SetFogColor(GX_RGB(31, 31, 31), 31);

            {
                int i;
                u32 fogTable[8];

                for (i = 0; i < 8; i++) {
                    fogTable[i] = (u32)(((i * 16) << 0) | ((i * 16 + 4) << 8) | ((i * 16 + 8) << 16) | ((i * 16 + 12) << 24));
                }

                G3X_SetFogTable(fogTable);
            }
        }

        {
            G2_SetBlendAlpha(GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BD, 7, 16 - 7);
            Bg_SetPriority(BG_LAYER_MAIN_1, 0);
            Bg_ToggleLayer(BG_LAYER_MAIN_1, TRUE);
        }
    } else if (frame < LANDSCAPE_FRAME_CUT_2) {
        movement.x += FX32_ONE * 4;
        Camera_Move(&movement, scene->landscape.camera);
        Bg_ScheduleScroll(scene->bgConfig, BG_LAYER_MAIN_1, BG_OFFSET_UPDATE_ADD_X, 4);
    } else if (frame == LANDSCAPE_FRAME_CUT_2) {
        VecFx32 target = { FX32_ONE * 16 * 30, 0, FX32_ONE * 0 - 0x70000 };
        Camera_InitWithTarget(&target, sLandscapeCameraConfig.distance, &sLandscapeCameraConfig.cameraAngle, sLandscapeCameraConfig.fovY, sLandscapeCameraConfig.projectionMtx, 0, scene->landscape.camera);
        {
            VecFx32 offset = { 0, FX32_ONE * 16 * 5, 0 };
            Camera_Move(&offset, scene->landscape.camera);
        }

        {
            CameraAngle angle = { 0, 0x680, 0, 0 };
            Camera_AdjustAngleAroundSelf(&angle, scene->landscape.camera);
        }

        {
            NNS_G3dGeFlushBuffer();
            G3X_SetFog(0, GX_FOGBLEND_COLOR_ALPHA, GX_FOGSLOPE_0x8000, 0);
        }

        {
            G2_BlendNone();
            Bg_ToggleLayer(BG_LAYER_MAIN_1, FALSE);
        }
    } else {
        if (frame >= LANDSCAPE_FRAME_ZOOM_OUT) {
            u16 fov;

            fov = Camera_GetFOV(scene->landscape.camera);

            if (fov + 24 < 0x4000) {
                Camera_AdjustFOV(24, scene->landscape.camera);
            }
        }

        movement.x += -0x2000;
        Camera_Move(&movement, scene->landscape.camera);
    }
}

// Handles the fades and landscape swaps for the two cuts of the fly-over.
static void MainScene_UpdateLandscapeCuts(OpeningCutscene *cutscene, MainScene *scene, int frame)
{
    BOOL loaded;

    switch (frame) {
    case LANDSCAPE_FRAME_FADE_OUT_1:
        StartScreenFade(FADE_MAIN_ONLY, FADE_TYPE_BRIGHTNESS_OUT, FADE_TYPE_BRIGHTNESS_OUT, COLOR_BLACK, 4, 1, HEAP_ID_76);
        break;
    case LANDSCAPE_FRAME_CUT_1:
        GF_ASSERT(IsScreenFadeDone() == TRUE);
        OpeningCutscene_FreeLandscape(cutscene);
        MainScene_LoadLandscapeStep(scene, 1);
        break;
    case LANDSCAPE_FRAME_CUT_1 + 1:
        MainScene_LoadLandscapeStep(scene, 1);
        break;
    case LANDSCAPE_FRAME_CUT_1 + 2:
        MainScene_LoadLandscapeStep(scene, 1);
        break;
    case LANDSCAPE_FRAME_CUT_1 + 3:
        loaded = MainScene_LoadLandscapeStep(scene, 1);
        GF_ASSERT(loaded == TRUE);
        StartScreenFade(FADE_MAIN_ONLY, FADE_TYPE_BRIGHTNESS_IN, FADE_TYPE_BRIGHTNESS_IN, COLOR_BLACK, 4, 1, HEAP_ID_76);
        break;
    case LANDSCAPE_FRAME_FADE_OUT_2:
        StartScreenFade(FADE_MAIN_ONLY, FADE_TYPE_BRIGHTNESS_OUT, FADE_TYPE_BRIGHTNESS_OUT, COLOR_BLACK, 4, 1, HEAP_ID_76);
        break;
    case LANDSCAPE_FRAME_CUT_2:
        GF_ASSERT(IsScreenFadeDone() == TRUE);
        OpeningCutscene_FreeLandscape(cutscene);
        MainScene_LoadLandscape(scene, 2);
        StartScreenFade(FADE_MAIN_ONLY, FADE_TYPE_BRIGHTNESS_IN, FADE_TYPE_BRIGHTNESS_IN, COLOR_BLACK, 4, 1, HEAP_ID_76);
        break;
    }
}

static BOOL OpeningCutscene_RunClosingScene(OpeningCutscene *cutscene)
{
    u8 *state = &(cutscene->closingScene.state);

    if (cutscene->skipRequested) {
        (*state) = 3;
    }

    switch (*state) {
    case 0:
        OpeningCutscene_SetUpClosingScene(cutscene);
        (*state)++;
        break;
    case 1:
        ClosingScene_LoadGraphics(&cutscene->closingScene);
        (*state)++;
        break;
    case 2:
        if (ClosingScene_Update(&cutscene->closingScene, cutscene->frame)) {
            (*state)++;
        }
        break;
    case 3:
        OpeningCutscene_CleanUpClosingScene(cutscene);
        return TRUE;
    }

    return FALSE;
}

static void OpeningCutscene_SetUpClosingScene(OpeningCutscene *cutscene)
{
    OpeningCutscene_SetClosingVRAMBanks();
    OpeningCutscene_InitClosingBgs(cutscene);

    cutscene->closingScene.bgConfig = cutscene->bgConfig;

    SetScreenColorBrightness(DS_SCREEN_MAIN, COLOR_WHITE);
    SetScreenColorBrightness(DS_SCREEN_SUB, COLOR_WHITE);

    G2_BlendNone();

    cutscene->closingScene.isSetUp = TRUE;
}

static void ClosingScene_LoadGraphics(ClosingScene *scene)
{
    Graphics_LoadPalette(NARC_INDEX_DEMO__TITLE__OP_DEMO, 99, PAL_LOAD_MAIN_BG, 0, 0, HEAP_ID_76);
    Graphics_LoadTilesToBgLayer(NARC_INDEX_DEMO__TITLE__OP_DEMO, 98, scene->bgConfig, BG_LAYER_MAIN_2, 0, 0, FALSE, HEAP_ID_76);
    Graphics_LoadTilemapToBgLayer(NARC_INDEX_DEMO__TITLE__OP_DEMO, 97, scene->bgConfig, BG_LAYER_MAIN_2, 0, 0, FALSE, HEAP_ID_76);

    GXLayers_DisableEngineALayers();
    GXLayers_EngineASetLayers(0);
    GXLayers_EngineBSetLayers(0);

    scene->scrollY = -24 * FX32_ONE;

    Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_2, BG_OFFSET_UPDATE_SET_X, 0);
    Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_2, BG_OFFSET_UPDATE_SET_Y, scene->scrollY / FX32_ONE);
}

static BOOL ClosingScene_Update(ClosingScene *scene, const int frame)
{
    u8 *subState = &(scene->subState);

    switch (*subState) {
    case 0:
        if (frame >= CLOSING_FRAME_FADE_IN_WHITE) {
            StartScreenFade(FADE_BOTH_SCREENS, FADE_TYPE_BRIGHTNESS_IN, FADE_TYPE_BRIGHTNESS_IN, COLOR_WHITE, 30, 1, HEAP_ID_76);
            (*subState)++;
        }
        break;
    case 1:
        if (IsScreenFadeDone()) {
            GXLayers_EngineAToggleLayers(GX_PLANEMASK_BG2, 1);
            StartScreenFade(FADE_BOTH_SCREENS, FADE_TYPE_BRIGHTNESS_IN, FADE_TYPE_BRIGHTNESS_IN, COLOR_BLACK, 90, 1, HEAP_ID_76);
            (*subState)++;
        }
        break;
    case 2:
        if (IsScreenFadeDone()) {
            (*subState)++;
        }
        break;
    case 3:
        if (frame >= CLOSING_FRAME_FADE_OUT) {
            StartScreenFade(FADE_BOTH_SCREENS, FADE_TYPE_BRIGHTNESS_OUT, FADE_TYPE_BRIGHTNESS_OUT, COLOR_BLACK, 8, 1, HEAP_ID_76);

            (*subState)++;
        }
        break;
    case 4:
        if (IsScreenFadeDone()) {
            return TRUE;
        }
    }

    scene->scrollY += 0x400;
    Bg_SetOffset(scene->bgConfig, BG_LAYER_MAIN_2, BG_OFFSET_UPDATE_SET_Y, scene->scrollY / FX32_ONE);

    return FALSE;
}

static void OpeningCutscene_CleanUpClosingScene(OpeningCutscene *cutscene)
{
    if (cutscene->closingScene.isSetUp) {
        Bg_FreeTilemapBuffer(cutscene->bgConfig, BG_LAYER_MAIN_2);
        Heap_Free(cutscene->bgConfig);
        cutscene->closingScene.isSetUp = FALSE;
    }
}
