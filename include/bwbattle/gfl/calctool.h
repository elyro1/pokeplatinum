#ifndef POKEBW2_GFL_CALCTOOL_H
#define POKEBW2_GFL_CALCTOOL_H

#include "types.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// Fixed point matrix, vector and collision helpers (calctool.c). The ROM embeds no name for this file; it is named
// after Game Freak's library's calculation tools. Names of the matrix and vector functions from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

struct CalcSphere {
    VecFx32 center;
    fx32 radius;
};

// A line segment with a radius
struct CalcCapsule {
    VecFx32 start;
    VecFx32 end;
    fx32 radius;
};

struct CalcHitResult {
    BOOL hit;
    // From the nearest point of the segment to the sphere's center, normalized
    VecFx32 dir;
    fx32 dist;
};

// A 2D affine matrix that rotates by a 16-bit angle and scales by the inverse of a scale on each axis, as a BG's
// matrix maps the screen to the BG. MAT2_ROT_* says how the angle is given
#define MAT2_ROT_IDX 0 // a 16-bit angle, 0x10000 for a full turn
#define MAT2_ROT_256 1 // 256 for a full turn
#define MAT2_ROT_DEG 2 // degrees
void MAT2_SetScaleRot(MtxFx22 *mtx, u16 rotation, fx32 scaleX, fx32 scaleY, u8 rotationMode);
void CalcSphere_Set(CalcSphere *sphere, const VecFx32 *center, fx32 radius);
// Returns whether a capsule touches a sphere, and, if result is not NULL, the distance and direction between them
BOOL CalcCapsule_HitSphere(const CalcCapsule *capsule, const CalcSphere *sphere, CalcHitResult *result);
void CalcCapsule_Set(CalcCapsule *capsule, const VecFx32 *start, const VecFx32 *end, fx32 radius);
void vecfx_mul(const VecFx32 *v, fx32 scale, VecFx32 *dest);
void vecfx_div(const VecFx32 *v, fx32 divisor, VecFx32 *dest);
// A rotation matrix from 16-bit angles about each axis
void MAT3_RotationEulerZYX(u16 x, u16 y, u16 z, MtxFx33 *mtx);
// A rotation matrix that turns the Z axis to a direction
void MAT3_RotationDir(const VecFx32 *dir, MtxFx33 *mtx);

#endif // POKEBW2_GFL_CALCTOOL_H
