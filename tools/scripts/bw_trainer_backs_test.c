#include <assert.h>
#include <math.h>
#include <stdio.h>

// Exercise the real draw function and generated bundle; discard unrelated functions at link time.
#include "../../src/bwbattle/system/mcss.c"
#include "../../src/bwbattle/assets/trainer_backs.c"

REGType32v s_reg_G3_END_VTXS;
static double sMatrix[6], sVertices[8][2];
static double sDepth, sVertexDepth[8];
static unsigned sVertexCount;

// Interpret the SDK's geometry commands so tests can inspect the actual sprite and shadow vertices.
void SIM_HandleG3Command(draw_msg_t *msg)
{
    double x, y;
    switch (msg->type) {
    case DRAW_CMD_G3_RESTOREMTX:
        memset(sMatrix, 0, sizeof(sMatrix));
        sMatrix[0] = sMatrix[3] = 1;
        sDepth = 0;
        break;
    case DRAW_CMD_G3_SCALE:
        x = msg->data.scale[0] / 4096.0;
        y = msg->data.scale[1] / 4096.0;
        sMatrix[0] *= x;
        sMatrix[2] *= x;
        sMatrix[1] *= y;
        sMatrix[3] *= y;
        break;
    case DRAW_CMD_G3_TRANSLATE:
        x = msg->data.scale[0] / 4096.0;
        y = msg->data.scale[1] / 4096.0;
        sMatrix[4] += sMatrix[0] * x + sMatrix[1] * y;
        sMatrix[5] += sMatrix[2] * x + sMatrix[3] * y;
        sDepth += msg->data.scale[2] / 4096.0;
        break;
    case DRAW_CMD_G3_VTX16:
        assert(sVertexCount < 8);
        x = msg->data.xyz.x / 4096.0;
        y = msg->data.xyz.y / 4096.0;
        sVertices[sVertexCount][0] = sMatrix[0] * x + sMatrix[1] * y + sMatrix[4];
        sVertexDepth[sVertexCount] = sDepth + msg->data.xyz.z / 4096.0;
        sVertices[sVertexCount++][1] = sMatrix[2] * x + sMatrix[3] * y + sMatrix[5];
        break;
    default:
        break;
    }
}

void G3_RotZ(fx32 s, fx32 c)
{
    double a = sMatrix[0], b = sMatrix[1], d = sMatrix[2], e = sMatrix[3];
    double sn = s / 4096.0, cs = c / 4096.0;
    sMatrix[0] = a * cs + b * sn;
    sMatrix[1] = -a * sn + b * cs;
    sMatrix[2] = d * cs + e * sn;
    sMatrix[3] = -d * sn + e * cs;
}

u32 GFL_G3DSysGetViewWidth(void) { return 256; }
void G3i_OrthoW_(fx32 t, fx32 b, fx32 l, fx32 r, fx32 n, fx32 f, fx32 w, BOOL draw, MtxFx44 *mtx) { }

static void DrawAtDepth(MCSS *sprite, MCSSSystem *system, const BwTrainerBackCell *cell, unsigned index,
    u16 rotation, fx32 depth)
{
    // Deliberately vary the old rig's pivot: it must not displace an attached joint.
    NNSG2dAnimDataSRT srt = { index, rotation, 4096, 4096, 3, -2 }, multi = { 0 };
    NNSG2dImagePaletteProxy palette = { 0 };
    fx32 z = depth;
    sVertexCount = 0;
    MCSS_DrawQuad(system, sprite, cell->x * 256, cell->y * 256, cell->width * 4096,
        cell->height * 4096, cell->s * 4096, cell->t * 4096, &srt, &multi, &palette, 0, FALSE, &z, 0);
    assert(sVertexCount == 8);
}

static void Draw(MCSS *sprite, MCSSSystem *system, const BwTrainerBackCell *cell, unsigned index, u16 rotation)
{
    DrawAtDepth(sprite, system, cell, index, rotation, 0);
}

static BOOL Opaque(const BwTrainerBackData *data, const BwTrainerBackCell *cell, int x, int y)
{
    return x >= 0 && y >= 0 && x < cell->width && y < cell->height
        && data->image[(cell->t + y) * 256 + cell->s + x] != 0;
}

static double Point(const BwTrainerBackCell *cell, double x, double y, unsigned axis, unsigned first)
{
    return sVertices[first][axis]
        + x / cell->width * (sVertices[first + 3][axis] - sVertices[first][axis])
        + y / cell->height * (sVertices[first + 1][axis] - sVertices[first][axis]);
}

int main(void)
{
    MCSS sprite = { 0 };
    MCSSSystem system = { 0 };
    NNSG2dMultiCellHierarchyData node = { 0, -6, -57, 0 };
    NNSG2dMultiCellData multi = { 1, 1, &node };
    NNSG2dMultiCellDataBank bank = { 0 };
    const u16 rotations[] = { 62988, 63498, 64007, 64517, 65027, 0 };
    // Elbow and wrist landmarks in the supplied resting-arm cells, in BwTrainerBack order.
    const int restLandmarks[][4] = {
        { 4, 31, 12, 49 }, { 6, 28, 8, 47 }, { 5, 29, 10, 46 }, { 4, 29, 10, 47 },
        { 6, 28, 12, 47 }, { 5, 30, 11, 48 }, { 5, 29, 11, 47 }, { 5, 28, 9, 46 },
    };
    // Top of the shoulder cap in each standing body, measured from its cropped artwork.
    // An inset arm pivot must not put the cut sleeve above this contour beside the neck.
    const int shoulderTop[] = { 40, 40, 39, 38, 39, 44, 40, 47 };
    unsigned checks = 0;
    bank.pMultiCellDataArray = &multi;
    sprite.mcAnim.pMultiCellDataBank = &bank;
    sprite.shadowAlpha = 31;
    for (unsigned trainer = BW_TRAINER_BACK_LUCAS; trainer <= BW_TRAINER_BACK_MIRA; trainer++) {
        const BwTrainerBackData *data = BwTrainerBack_Get(trainer);
        unsigned lastArm = trainer == BW_TRAINER_BACK_DAWN ? 4 : 3;
        // Use the production traversal and capture its actual geometry depths. A correct attachment can
        // still look detached if the body draws over the upper arm at the joint.
        sprite.trainerBack = trainer;
        sprite.alpha = 31;
        int nodeCount = trainer == BW_TRAINER_BACK_DAWN ? 3 : 2;
        double bodyDepth = 0, armDepth = 0, sleeveDepth = 0;
        for (int step = 0; step < nodeCount; step++) {
            fx32 depth;
            int nodeIdx = MCSS_TrainerBackNode(trainer, nodeCount, step, &depth);
            unsigned index = trainer == BW_TRAINER_BACK_DAWN ? (nodeIdx == 0 ? 3 : nodeIdx == 1 ? 2 : 0)
                : (nodeIdx == 0 ? 1 : 0);
            DrawAtDepth(&sprite, &system, &data->cells[index], index, 0, depth);
            if (index == 0) {
                bodyDepth = sVertexDepth[0];
                if (trainer != BW_TRAINER_BACK_DAWN) assert(step == 0);
            } else if (index == 2) {
                sleeveDepth = sVertexDepth[0];
            } else {
                armDepth = sVertexDepth[0];
            }
            for (unsigned v = 4; v < 8; v++) assert(sVertexDepth[v] == 0); // shadow remains on its plane
        }
        if (trainer == BW_TRAINER_BACK_DAWN) {
            assert(armDepth < sleeveDepth && sleeveDepth < bodyDepth);
        } else {
            assert(armDepth > bodyDepth);
        }
        fx32 stillDepth;
        assert(MCSS_TrainerBackNode(trainer, 1, 0, &stillDepth) == 0 && stillDepth == 0);
        checks++;
        // The resting hand must also extend beyond the torso silhouette. Joint overlap alone allowed
        // it to fold entirely behind the torso while all throw-pose checks still passed.
        const BwTrainerBackCell *rest = &data->cells[1], *body = &data->cells[0];
        double bodyLeft = body->x + (trainer == BW_TRAINER_BACK_DAWN ? -2 : 0);
        double bodyTop = -56 - body->y;
        sprite.trainerBack = trainer;
        sprite.alpha = 31;
        Draw(&sprite, &system, rest, 1, 0);
        const int *landmarks = restLandmarks[trainer - BW_TRAINER_BACK_LUCAS];
        assert(Opaque(data, rest, landmarks[0], landmarks[1]));
        assert(Opaque(data, rest, landmarks[2], landmarks[3]));
        for (unsigned first = 0; first <= 4; first += 4) {
            double sx = Point(rest, -rest->x, rest->y, 0, first);
            double sy = -Point(rest, -rest->x, rest->y, 1, first);
            double ex = Point(rest, landmarks[0], landmarks[1], 0, first) - sx;
            double ey = -Point(rest, landmarks[0], landmarks[1], 1, first) - sy;
            double wx = Point(rest, landmarks[2], landmarks[3], 0, first) - sx;
            double wy = -Point(rest, landmarks[2], landmarks[3], 1, first) - sy;
            // Facing right, the elbow must lie behind the shoulder-to-wrist line. This signed bend
            // survives rotation, but changes sign if the resting limb is accidentally reflected.
            if (ex * wy - ey * wx >= 0) fprintf(stderr, "Forward resting elbow: trainer %u\n", trainer);
            assert(ex * wy - ey * wx < 0);
        }
        unsigned visibleHand = 0, handPixels = 0, visibleUpperSleeve = 0;
        double armTop = body->height;
        for (int y = 0; y < rest->height; y++) for (int x = 0; x < rest->width; x++) {
            if (!Opaque(data, rest, x, y)) continue;
            double px = Point(rest, x + 0.5, y + 0.5, 0, 0) * 16;
            double py = Point(rest, x + 0.5, y + 0.5, 1, 0) * -16;
            int bx = (int)floor(px - bodyLeft), by = (int)floor(py - bodyTop);
            BOOL visible = !Opaque(data, body, bx, by);
            if (py - bodyTop < armTop) armTop = py - bodyTop;
            if (y >= rest->height * 3 / 4) {
                handPixels++;
                visibleHand += visible;
            }
            // Lucas's arm draws in front of the torso. Count the visible blue sleeve at the shoulder
            // even where it overlaps the body; testing only pixels outside the body rewards moving
            // the arm out past the shoulder. The armTop check also rejects moving it up to the neck.
            u16 color = data->colors[data->image[(rest->t + y) * 256 + rest->s + x]];
            if (trainer == BW_TRAINER_BACK_LUCAS && bx >= 27 && bx <= 40 && by >= 40 && by <= 53
                && (color >> 10 & 31) > (color & 31) + 5) {
                visibleUpperSleeve++;
            }
        }
        if (armTop < shoulderTop[trainer - BW_TRAINER_BACK_LUCAS] - 1
            || armTop > shoulderTop[trainer - BW_TRAINER_BACK_LUCAS] + 3) {
            fprintf(stderr, "Misaligned resting shoulder: trainer %u, sleeve top %.2f, shoulder %d\n",
                trainer, armTop, shoulderTop[trainer - BW_TRAINER_BACK_LUCAS]);
            assert(FALSE);
        }
        if (visibleHand < 16 || visibleHand * 2 < handPixels) {
            fprintf(stderr, "Hidden resting hand: trainer %u, visible %u/%u\n", trainer, visibleHand, handPixels);
        }
        assert(visibleHand >= 16 && visibleHand * 2 >= handPixels);
        if (trainer == BW_TRAINER_BACK_LUCAS) {
            if (visibleUpperSleeve < 12) fprintf(stderr, "Low resting sleeve: %u visible shoulder pixels\n", visibleUpperSleeve);
            assert(visibleUpperSleeve >= 12);
        }
        checks++;
        for (unsigned i = 0; i + 1 < data->cellCount; i++) {
            const BwTrainerBackCell *cell = &data->cells[i];
            for (unsigned alpha = 17; alpha <= 31; alpha += 14) {
                sprite.alpha = alpha;
                for (unsigned r = 0; r < NELEMS(rotations); r++) {
                    sprite.trainerBack = trainer;
                    Draw(&sprite, &system, cell, i, rotations[r]);
                    if (i >= 1 && i <= lastArm) {
                        const BwTrainerBackCell *target = &data->cells[0];
                        double left = target->x + (trainer == BW_TRAINER_BACK_DAWN ? -2 : 0);
                        double top = -56 - target->y;
                        BOOL elbow = trainer == BW_TRAINER_BACK_DAWN && i == 3;
                        assert(Opaque(data, cell, -cell->x, cell->y));
                        // A fixed pivot alone is insufficient: opaque arm pixels must overlap the body/sleeve.
                        if (elbow) {
                            target = &data->cells[2];
                        }
                        for (unsigned first = 0; first <= 4; first += 4) {
                            assert(fabs(Point(cell, -cell->x, cell->y, 0, first) * 16 - cell->attachX) < 1e-6);
                            assert(fabs(Point(cell, -cell->x, cell->y, 1, first) * -16 - cell->attachY) < 1e-6);
                            unsigned overlap = 0;
                            for (int dy = -3; dy <= 3; dy++) for (int dx = -3; dx <= 3; dx++) {
                                int x = -cell->x + dx, y = cell->y + dy;
                                if (!Opaque(data, cell, x, y)) continue;
                                double px = Point(cell, x + 0.5, y + 0.5, 0, first) * 16;
                                double py = Point(cell, x + 0.5, y + 0.5, 1, first) * -16;
                                int tx = elbow ? (int)floor(target->attachX - px - target->x) : (int)floor(px - left);
                                int ty = elbow ? (int)floor(py - target->attachY + target->y) : (int)floor(py - top);
                                overlap += Opaque(data, target, tx, ty);
                            }
                            if (overlap < 6) fprintf(stderr, "Gap: trainer %u cell %u rotation %u overlap %u\n",
                                trainer, i, rotations[r], overlap);
                            assert(overlap >= 6);
                        }
                    } else {
                        double expected[8][2];
                        memcpy(expected, sVertices, sizeof(expected));
                        sprite.trainerBack = BW_TRAINER_BACK_NONE;
                        Draw(&sprite, &system, cell, i, rotations[r]);
                        assert(memcmp(expected, sVertices, sizeof(expected)) == 0);
                    }
                    // Image fallbacks must retain their existing transform, even with a registered trainer.
                    double expected[8][2];
                    sprite.image = (u8 *)1;
                    sprite.trainerBack = BW_TRAINER_BACK_NONE;
                    Draw(&sprite, &system, cell, i, rotations[r]);
                    memcpy(expected, sVertices, sizeof(expected));
                    sprite.trainerBack = trainer;
                    Draw(&sprite, &system, cell, i, rotations[r]);
                    assert(memcmp(expected, sVertices, sizeof(expected)) == 0);
                    sprite.image = NULL;
                    checks++;
                }
            }
        }
    }
    printf("PASS: %u trainer/cell/rotation/fade cases, including elbow direction, layering, joint coverage, shadows and fallbacks\n", checks);
}
