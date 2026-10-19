// The battle view's field: the ground and weather model around the stage, with its animations and palette fade. The
// name is the ROM's string, from GFL_HeapAllocate's asserts. BtlvField_Create is swan's name; the others are ours

#include "battle/btlv_field.h"
#include "types.h"
#include "battle/btlv_effect.h"
#include "battle/btlv_stage.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nnsys/g3d.h"

// The battle's 3D graphics archive, and the archive of the field table
#define BTLV_FIELD_GRA_ARC 11
#define BTLV_FIELD_TABLE_ARC 0x97

#define BTLV_FIELD_RES_NONE 0xffff

// A field's resources in file 1 of archive 0x97: the model, then three animations, each by variant, the season or a
// special form; BTLV_FIELD_RES_NONE for none
typedef u16 BtlvFieldResIds[4][8];

// An animation's playback, 0x10 bytes
typedef struct {
    u32 mode;    // 0x0  0: loops at normal speed; 1: steps by speed while playing
    u32 playing; // 0x4
    fx32 speed;  // 0x8
    s32 frames;  // 0xc  mode 1: the steps left
} BtlvFieldAnm;

// The field's work, 0x7c bytes
struct BtlvField {
    void *res;                  // 0x00  the model and its textures
    s32 anmCount;               // 0x04
    BtlvFieldAnm *anmParams;    // 0x08
    void **anmRes;              // 0x0c
    void **anms;                // 0x10
    G3DModel *mdl;              // 0x14
    G3DActor *actor;            // 0x18
    SRTMatrix status;           // 0x1c
    BtlvTexPaletteFade palFade; // 0x58
    // 0x68
    u32 hidden : 1;
    u32 pauseAnm : 1; // skips one animation step
    u32 fieldId;      // 0x6c
    HeapID heapId;    // 0x70
    TCB *uploadTask;  // 0x74
    u16 fieldParam;   // 0x78  the field in the low byte, the variant in the high one
};

static void BtlvField_FreeAnms(BtlvField *field);
static void BtlvField_UploadTexTask(TCB *tcb, void *data);
static void BtlvField_FreeResources(BtlvField *field);

// The variant of each season, 0 for none
static const u8 data_ov168_021f2f6c[] = { 0, 4, 5, 6, 7 };

BtlvField *BtlvField_Create(BOOL arg0, u8 fieldId, u8 variant, HeapID heapId, u32 mode, s32 fieldParam, BOOL arg6) {
    BtlvField *field;
    BtlvFieldResIds *table;
    u32 palSize;
    int i;
    int j;

    field = GFL_HeapAllocate(heapId, sizeof(BtlvField), TRUE, "btlv_field.c", 0x76);
    table = GFL_ArcSysReadHeapNew(BTLV_FIELD_TABLE_ARC, 1, HEAPID_TAIL(heapId));

    if (mode == 1) {
        fieldId = 0x14;
    } else if (mode == 2) {
        if (fieldParam == 0) {
            fieldId = 0x15;
            variant = 0;
        } else {
            fieldId = fieldParam;
            variant = fieldParam >> 8;
            if (variant != 0) {
                variant += 3;
            }
        }
        field->fieldParam = fieldId | (variant << 8);
    }
    if (arg6 && fieldId == 0x2e) {
        variant = 4;
    }

    field->heapId = heapId;
    field->fieldId = fieldId;
    if (table[fieldId][0][variant] != BTLV_FIELD_RES_NONE) {
        field->res = GFL_G3DSysReadArcSysResource(BTLV_FIELD_GRA_ARC, table[fieldId][0][variant]);
    } else {
        field->res = GFL_G3DSysReadArcSysResource(BTLV_FIELD_GRA_ARC, table[fieldId][0][0]);
    }
    GFL_G3DResUploadTexData(field->res);

    field->anmCount = 0;
    for (i = 1; i < 4; i++) {
        if (table[fieldId][i][variant] != BTLV_FIELD_RES_NONE) {
            field->anmCount++;
        }
    }

    j = 0;
    field->mdl = GFL_G3DMdlCreate(field->res, 0, field->res);
    if (field->anmCount != 0) {
        field->anmRes = GFL_HeapAllocate(field->heapId, field->anmCount * sizeof(void *), FALSE, "btlv_field.c", 0xc0);
        field->anms = GFL_HeapAllocate(field->heapId, field->anmCount * sizeof(void *), FALSE, "btlv_field.c", 0xc1);
        field->anmParams =
            GFL_HeapAllocate(field->heapId, field->anmCount * sizeof(BtlvFieldAnm), TRUE, "btlv_field.c", 0xc2);
        for (i = 1; i < 4; i++) {
            if (table[fieldId][i][variant] != BTLV_FIELD_RES_NONE) {
                field->anmRes[j] = GFL_G3DSysReadArcSysResource(BTLV_FIELD_GRA_ARC, table[fieldId][i][variant]);
                field->anms[j] = GFL_G3DAnmCreate(field->mdl, field->anmRes[j], 0);
                field->anmParams[j].mode = 0;
                j++;
            }
        }
        // GFL_G3DActorCreate's count is a u16
        field->actor = GFL_G3DActorCreate(field->mdl, field->anms, (u16)field->anmCount);
        for (i = 0; i < field->anmCount; i++) {
            GFL_G3DActorBindAnm(field->actor, i);
        }
    } else {
        field->actor = GFL_G3DActorCreate(field->mdl, NULL, 0);
    }

    field->status.translation.x = 0;
    field->status.translation.y = 0;
    field->status.translation.z = 0;
    field->status.scale.x = FX32_ONE;
    field->status.scale.y = FX32_ONE;
    field->status.scale.z = FX32_ONE;
    MTX_Identity33_(&field->status.rotation);

    palSize = NNS_G3DResGetTexBlock(GFL_G3DResGetResData(field->res))->plttInfo.sizePltt << 3;
    field->palFade.resources = GFL_HeapAllocate(field->heapId, sizeof(void *), FALSE, "btlv_field.c", 0xea);
    field->palFade.palettes = GFL_HeapAllocate(field->heapId, sizeof(void *), FALSE, "btlv_field.c", 0xeb);
    field->palFade.resources[0] = field->res;
    field->palFade.palettes[0] = GFL_HeapAllocate(field->heapId, palSize, FALSE, "btlv_field.c", 0xee);
    field->palFade.active = FALSE;
    field->palFade.count = 1;

    GFL_HeapFree(table);
    return field;
}

static void BtlvField_FreeAnms(BtlvField *field) {
    int i;

    GFL_G3DActorFree(field->actor);
    if (field->anmCount != 0) {
        for (i = 0; i < field->anmCount; i++) {
            GFL_G3DAnmFree(field->anms[i]);
            GFL_G3DResFree(field->anmRes[i]);
        }
        GFL_HeapFree(field->anmParams);
        GFL_HeapFree(field->anms);
        GFL_HeapFree(field->anmRes);
    }
}

void BtlvField_Reload(BtlvField *field, s32 fieldParam) {
    BtlvFieldResIds *table;
    u8 variant;
    u32 palSize;
    int i;
    int j;

    BtlvField_FreeResources(field);
    table = GFL_ArcSysReadHeapNew(BTLV_FIELD_TABLE_ARC, 1, HEAPID_TAIL(field->heapId));
    variant = data_ov168_021f2f6c[(fieldParam >> 8) & 0xff];
    if (table[fieldParam & 0xff][0][variant] != BTLV_FIELD_RES_NONE) {
        field->res = GFL_G3DSysReadArcSysResource(BTLV_FIELD_GRA_ARC, table[fieldParam & 0xff][0][variant]);
    }
    GFL_G3DResSetupTexData(field->res);
    field->uploadTask = GFL_VBlankTCBAdd(BtlvField_UploadTexTask, field, 10);

    field->anmCount = 0;
    for (i = 1; i < 4; i++) {
        if (table[fieldParam & 0xff][i][variant] != BTLV_FIELD_RES_NONE) {
            field->anmCount++;
        }
    }

    j = 0;
    field->mdl = GFL_G3DMdlCreate(field->res, 0, field->res);
    if (field->anmCount != 0) {
        field->anmRes = GFL_HeapAllocate(field->heapId, field->anmCount * sizeof(void *), FALSE, "btlv_field.c", 0x1ca);
        field->anms = GFL_HeapAllocate(field->heapId, field->anmCount * sizeof(void *), FALSE, "btlv_field.c", 0x1cb);
        field->anmParams =
            GFL_HeapAllocate(field->heapId, field->anmCount * sizeof(BtlvFieldAnm), TRUE, "btlv_field.c", 0x1cc);
        for (i = 1; i < 4; i++) {
            if (table[fieldParam & 0xff][i][variant] != BTLV_FIELD_RES_NONE) {
                field->anmRes[j] =
                    GFL_G3DSysReadArcSysResource(BTLV_FIELD_GRA_ARC, table[fieldParam & 0xff][i][variant]);
                field->anms[j] = GFL_G3DAnmCreate(field->mdl, field->anmRes[j], 0);
                field->anmParams[j].mode = 0;
                j++;
            }
        }
        field->actor = GFL_G3DActorCreate(field->mdl, field->anms, (u16)field->anmCount);
        for (i = 0; i < field->anmCount; i++) {
            GFL_G3DActorBindAnm(field->actor, i);
        }
    } else {
        field->actor = GFL_G3DActorCreate(field->mdl, NULL, 0);
    }

    field->status.translation.x = 0;
    field->status.translation.y = 0;
    field->status.translation.z = 0;
    field->status.scale.x = FX32_ONE;
    field->status.scale.y = FX32_ONE;
    field->status.scale.z = FX32_ONE;
    MTX_Identity33_(&field->status.rotation);

    palSize = NNS_G3DResGetTexBlock(GFL_G3DResGetResData(field->res))->plttInfo.sizePltt << 3;
    field->palFade.resources = GFL_HeapAllocate(field->heapId, sizeof(void *), FALSE, "btlv_field.c", 0x1f4);
    field->palFade.palettes = GFL_HeapAllocate(field->heapId, sizeof(void *), FALSE, "btlv_field.c", 0x1f5);
    field->palFade.resources[0] = field->res;
    field->palFade.palettes[0] = GFL_HeapAllocate(field->heapId, palSize, FALSE, "btlv_field.c", 0x1f8);
    field->palFade.active = FALSE;
    field->palFade.count = 1;

    GFL_HeapFree(table);
    field->fieldParam = fieldParam;
}

static void BtlvField_UploadTexTask(TCB *tcb, void *data) {
    BtlvField *field = data;

    GFL_G3DResUploadTexDataCore(field->res);
    GFL_TCBRemove(tcb);
}

static void BtlvField_FreeResources(BtlvField *field) {
    GFL_HeapFree(field->palFade.palettes[0]);
    GFL_HeapFree(field->palFade.palettes);
    GFL_HeapFree(field->palFade.resources);
    BtlvField_FreeAnms(field);
    GFL_G3DResFreeTexData(field->res);
    GFL_G3DMdlFree(field->mdl);
    GFL_G3DResFree(field->res);
}

void BtlvField_Delete(BtlvField *field) {
    BtlvField_FreeAnms(field);
    GFL_G3DMdlFree(field->mdl);
    GFL_G3DResFree(field->res);
    GFL_HeapFree(field->palFade.palettes[0]);
    GFL_HeapFree(field->palFade.palettes);
    GFL_HeapFree(field->palFade.resources);
    GFL_HeapFree(field);
}

void BtlvField_Main(BtlvField *field) {
    int i;
    fx32 step;

    if (field->pauseAnm) {
        field->pauseAnm = FALSE;
        return;
    }
    if (field->anmCount != 0) {
        for (i = 0; i < field->anmCount; i++) {
            step = 0;
            switch (field->anmParams[i].mode) {
            case 0:
                step = FX32_ONE;
                break;
            case 1:
                if (field->anmParams[i].playing == TRUE) {
                    step = field->anmParams[i].speed;
                    field->anmParams[i].frames--;
                    if (field->anmParams[i].frames == 0) {
                        field->anmParams[i].playing = FALSE;
                    }
                }
                break;
            }
            if (step != 0) {
                GFL_G3DActorStepAnmFrameLoop(field->actor, i, step);
            }
        }
    }
    BtlvEffect_UpdateTexPaletteFade(&field->palFade);
}

void BtlvField_Draw(BtlvField *field) {
    if (!field->hidden) {
        GFL_G3DSysDrawObj(field->actor, &field->status);
    }
}

void BtlvField_StartPaletteFade(BtlvField *field, u8 evy, u8 targetEvy, u8 waitFrames, u16 color) {
    field->palFade.active = TRUE;
    field->palFade.evy = evy;
    field->palFade.targetEvy = targetEvy;
    field->palFade.wait = 0;
    field->palFade.waitFrames = waitFrames;
    field->palFade.color = color;
}

BOOL BtlvField_IsPaletteFading(BtlvField *field) {
    if (field->palFade.active) {
        return TRUE;
    }
    return FALSE;
}

void BtlvField_SetHidden(BtlvField *field, BOOL hide) {
    field->hidden = hide;
}
