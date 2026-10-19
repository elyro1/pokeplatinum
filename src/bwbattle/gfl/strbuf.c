#include "types.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/str.h"

// Marks a valid buffer, and is cleared when it is freed
#define STRBUF_MAGIC 0xb6f8d2ec

struct StrBuf {
    u16 size;
    u16 length;
    u32 magic;
    u16 chars[1];
};

static u16 sTerminator = 0xffff;

void GFL_StrBufSetTerminator(u16 terminator) {
    sTerminator = terminator;
}

BOOL GFL_StrBufIsValid(const StrBuf *strbuf) {
    if (strbuf != NULL && strbuf->magic == STRBUF_MAGIC) {
        return TRUE;
    }
    return FALSE;
}

StrBuf *GFL_StrBufCreate(u32 size, HeapID heapId) {
    // The header and size characters, and one more
    StrBuf *strbuf =
        GFL_HeapAllocate(heapId, size * sizeof(u16) + sizeof(StrBuf) - sizeof(u16), FALSE, "strbuf.c", 100);

    strbuf->magic = STRBUF_MAGIC;
    strbuf->size = size;
    strbuf->length = 0;
    strbuf->chars[0] = sTerminator;
    return strbuf;
}

void GFL_StrBufFree(StrBuf *strbuf) {
    strbuf->magic = 0;
    GFL_HeapFree(strbuf);
}

void GFL_StrBufClear(StrBuf *strbuf) {
    strbuf->length = 0;
    strbuf->chars[0] = sTerminator;
}

void GFL_StrBufCopy(StrBuf *dest, const StrBuf *src) {
    if (dest->size > src->length) {
        MI_CpuCopy8(src->chars, dest->chars, (src->length + 1) * sizeof(u16));
        dest->length = src->length;
    }
}

StrBuf *GFL_StrBufClone(const StrBuf *strbuf, HeapID heapId) {
    StrBuf *clone = GFL_StrBufCreate(strbuf->length + 1, heapId);

    GFL_StrBufCopy(clone, strbuf);
    return clone;
}

BOOL GFL_StrBufCmp(const StrBuf *a, const StrBuf *b) {
    int i;

    for (i = 0; a->chars[i] == b->chars[i]; i++) {
        if (a->chars[i] == sTerminator) {
            return TRUE;
        }
    }
    return FALSE;
}

u16 GFL_StrBufGetCharCount(const StrBuf *strbuf) {
    return strbuf->length;
}

void GFL_StrBufInsertTerminator(StrBuf *strbuf, u32 length) {
    if (length < strbuf->length) {
        strbuf->chars[length] = sTerminator;
        strbuf->length = length;
    }
}

void GFL_StrBufLoadString(StrBuf *strbuf, const u16 *src) {
    strbuf->length = 0;
    while (*src != sTerminator) {
        if (strbuf->length >= strbuf->size - 1) {
            break;
        }
        strbuf->chars[strbuf->length++] = *src++;
    }
    strbuf->chars[strbuf->length] = sTerminator;
}

void GFL_StrBufLoadFixedString(StrBuf *strbuf, const u16 *str, u32 length) {
    u32 i;

    if (length <= strbuf->size) {
        for (i = 0; i < length - 1; i++) {
            strbuf->chars[i] = str[i];
            if (strbuf->chars[i] == sTerminator) {
                break;
            }
        }
        strbuf->length = i;
        if (i == length - 1) {
            strbuf->chars[length - 1] = sTerminator;
        }
    }
}

void GFL_StrBufCopyString(StrBuf *strbuf, const u16 *src, u32 length) {
    if (length != 0 && length <= strbuf->size) {
        MI_CpuCopy8(src, strbuf->chars, length * sizeof(u16));
        strbuf->length = length - 1;
    }
}

void GFL_StrBufStoreString(const StrBuf *strbuf, u16 *dest, u32 size) {
    u32 i;

    for (i = 0; i < size; i++) {
        dest[i] = strbuf->chars[i];
        if (dest[i] == sTerminator) {
            break;
        }
    }
    if (i == size) {
        *(dest + size - 1) = sTerminator;
    }
}

const u16 *GFL_StrBufGetStringPtr(const StrBuf *strbuf) {
    return strbuf->chars;
}

u16 GFL_StrBufGetTerminator(void) {
    return sTerminator;
}

void GFL_StrBufConcat(StrBuf *dest, const StrBuf *src) {
    if (dest->length + src->length + 1 <= dest->size) {
        MI_CpuCopy8(src->chars, &dest->chars[dest->length], (src->length + 1) * sizeof(u16));
        dest->length += src->length;
    }
}

void GFL_StrBufAppend(StrBuf *strbuf, u16 c) {
    if (strbuf->length + 1 < strbuf->size) {
        strbuf->chars[strbuf->length++] = c;
        strbuf->chars[strbuf->length] = sTerminator;
    }
}
