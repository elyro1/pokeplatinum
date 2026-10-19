#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "pml/personal.h"
#include "pml/poke_party.h"

// The species' data of the personal archives: this game's, and Black and White's. The file's name is the ROM's
// string; the functions' names are swan's

// Black and White's personal data
#define ARCID_PERSONAL_BW1 302

// The file of ARCID_PERSONAL that holds the regional Pokédex numbers, and the number of a species that has none
#define PERSONAL_FILE_REGIONAL_DEX 709
#define REGIONAL_DEX_NONE 999

static u16 PML_PersonalGetDatID(u16 species, u16 form);

// The archives: this game's, then Black and White's
static ArcTool *g_PMLPersonalArcBW2[2];

void PML_PersonalArcInit(HeapID heapId) {
    if (g_PMLPersonalArcBW2[0] == NULL) {
        g_PMLPersonalArcBW2[0] = GFL_ArcSysCreateFileHandle(ARCID_PERSONAL, heapId);
        // bwbattle: Black and White's personal data (ARCID_PERSONAL_BW1) isn't extracted; nothing in battle reads it
    }
}

void PML_PersonalFetchBW2(u16 species, u16 form, PersonalData *dest) {
    u16 file = PML_PersonalGetDatID(species, form);

    GFL_ArcToolSeek(g_PMLPersonalArcBW2[0], GFL_ArcToolGetDataOfs(g_PMLPersonalArcBW2[0], file));
    GFL_ArcToolReadRaw(g_PMLPersonalArcBW2[0], sizeof(PersonalData), dest);
}

void PML_LearnsetLvUpLoad(u16 species, u16 form, void *dest) {
    GFL_ArcSysRead(dest, ARCID_LEARNSETS, PML_PersonalGetDatID(species, form));
}

void *PML_PersonalLoad(u16 species, u16 form, HeapID heapId) {
    PersonalData *personal = GFL_HeapAllocate(heapId, sizeof(PersonalData), FALSE, "poke_personal.c", 124);

    PML_PersonalFetchBW2(species, form, personal);
    return personal;
}

void PML_PersonalFree(void *personal) {
    GFL_HeapFree(personal);
}

u32 PML_PersonalGetParam(void *personal, u32 param) {
    PersonalData *data = personal;

    switch (param) {
    case 0:
        return data->hp;
    case 1:
        return data->attack;
    case 2:
        return data->defense;
    case 3:
        return data->speed;
    case 4:
        return data->spAttack;
    case 5:
        return data->spDefense;
    case PERSONAL_TYPE_1:
        return data->type1;
    case PERSONAL_TYPE_2:
        return data->type2;
    case 8:
        return data->catchRate;
    case 9:
        return data->baseExp;
    case 10:
        return data->evHP;
    case 11:
        return data->evAttack;
    case 12:
        return data->evDefense;
    case 13:
        return data->evSpeed;
    case 14:
        return data->evSpAttack;
    case 15:
        return data->evSpDefense;
    case PERSONAL_NO_BOUNCE:
        return data->noBounce;
    case 17:
        return data->items[0];
    case 18:
        return data->items[1];
    case 19:
        return data->items[2];
    case PERSONAL_SEX_RATIO:
        return data->sexRatio;
    case PERSONAL_HATCH_CYCLES:
        return data->hatchCycles;
    case 22:
        return data->friendship;
    case 23:
        return data->growthRate;
    case 24:
        return data->eggGroup1;
    case 25:
        return data->eggGroup2;
    case PERSONAL_ABILITY_1:
        return data->abilities[0];
    case PERSONAL_ABILITY_2:
        return data->abilities[1];
    case PERSONAL_ABILITY_HIDDEN:
        return data->abilities[2];
    case 29:
        return data->fleeRate;
    case 30:
        return data->formFile;
    case PERSONAL_FORM_SPRITE_OFFSET:
        return data->formSpriteOffset;
    case PERSONAL_FORM_COUNT:
        return data->formCount;
    case 33:
        return data->color;
    case PERSONAL_SPRITE_FLIP:
        return data->spriteFlip;
    case PERSONAL_PALETTE_FORMS:
        return data->paletteForms;
    case 36:
        return data->stage;
    case 37:
        return data->height;
    case PERSONAL_WEIGHT:
        return data->weight;
    case 39:
        return data->machines[0];
    case 40:
        return data->machines[1];
    case 41:
        return data->machines[2];
    case 42:
        return data->machines[3];
    case 43:
        return data->tutors[0];
    case 44:
        return data->tutors[1];
    case 45:
        return data->tutors[2];
    case 46:
        return data->tutors[3];
    case 47:
        return data->tutors[4];
    }
    return 0;
}

// Whether a sex ratio leaves one sex only: all male, all female, or none
BOOL isGenderlessOrSetGender(u32 genderRatio) {
    if (genderRatio == GENDER_RATIO_MALE_ONLY || genderRatio == GENDER_RATIO_FEMALE_ONLY ||
        genderRatio == GENDER_RATIO_GENDERLESS) {
        return TRUE;
    }
    return FALSE;
}

u16 *PML_PersonalLoadRegionalDexTable(HeapID heapId, u16 *count) {
    u32 size;
    u16 found = 0;
    u16 *table =
        GFL_ArcToolReadHeapNewLZGetLen(g_PMLPersonalArcBW2[0], PERSONAL_FILE_REGIONAL_DEX, FALSE, heapId, &size);
    u16 entries = size / sizeof(u16);

    if (count != NULL) {
        u16 i;

        for (i = 0; i < entries; i++) {
            if (table[i] != REGIONAL_DEX_NONE) {
                found++;
            }
        }
        *count = found;
    }
    return table;
}

// The file of a species' form in the personal and learnset archives: an egg's is 0, and a form's follows the species'
static u16 PML_PersonalGetDatID(u16 species, u16 form) {
    PersonalData personal;

    if (species == SPECIES_EGG || species == SPECIES_EGG + 1) {
        return 0;
    }
    GFL_ArcToolSeek(g_PMLPersonalArcBW2[0], GFL_ArcToolGetDataOfs(g_PMLPersonalArcBW2[0], species));
    GFL_ArcToolReadRaw(g_PMLPersonalArcBW2[0], sizeof(PersonalData), &personal);
    if (personal.formFile == 0 || personal.formCount <= form || form == 0) {
        return species;
    }
    return personal.formFile + form - 1;
}

void PML_PersonalFetchBW1(u16 species, u16 form, PersonalData *dest) {
    u16 file = PML_PersonalGetDatID(species, form);

    GFL_ArcToolSeek(g_PMLPersonalArcBW2[1], GFL_ArcToolGetDataOfs(g_PMLPersonalArcBW2[1], file));
    GFL_ArcToolReadRaw(g_PMLPersonalArcBW2[1], sizeof(PersonalData), dest);
}
