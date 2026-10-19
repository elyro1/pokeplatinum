#ifndef BWBATTLE_TYPES_H
#define BWBATTLE_TYPES_H

// BW2's types.h, on the port's NitroSDK types instead of BW2's own

#include <stddef.h>
#include <stdint.h>
#include <nitro/types.h>

#ifndef NELEMS
#define NELEMS(array) (sizeof(array) / sizeof((array)[0]))
#endif

#ifndef ATTRIBUTE_ALIGN
#define ATTRIBUTE_ALIGN(num) __attribute__((aligned(num)))
#endif

#endif // BWBATTLE_TYPES_H
