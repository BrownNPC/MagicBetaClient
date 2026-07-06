#pragma once
#include "so/builtin/builtin.h"

// -- Embeds --

#include "so/builtin/builtin.h"

#ifndef so_build_hosted
// Pure C memchr implementation for freestanding environments.
static inline void* memchr(const void* s, int c, size_t n) {
    const unsigned char* p = s;
    unsigned char target = (unsigned char)c;
    while (n--) {
        if (*p == target) return (void*)p;
        p++;
    }
    return NULL;
}
#endif

static inline so_int bytealg_Compare(so_Slice a, so_Slice b) {
    so_int n = a.len;
    if (b.len < n) n = b.len;
    int cmp = memcmp(a.ptr, b.ptr, (size_t)n);
    if (cmp != 0) return cmp;
    if (a.len < b.len) return -1;
    if (a.len > b.len) return +1;
    return 0;
}

static inline so_int bytealg_IndexByte(so_Slice b, so_byte c) {
    void* at = memchr(b.ptr, (int)c, (size_t)b.len);
    if (at == NULL) return -1;
    return (so_int)((char*)at - (char*)b.ptr);
}

// -- Variables and constants --

// PrimeRK is the prime base used in Rabin-Karp algorithm.
static const int64_t bytealg_PrimeRK = 16777619;

// -- Functions and methods --

// IndexRabinKarp uses the Rabin-Karp search algorithm to return the index of the
// first occurrence of sep in s, or -1 if not present.
so_int bytealg_IndexRabinKarp(so_Slice s, so_Slice sep);

// LastIndexRabinKarp uses the Rabin-Karp search algorithm to return the last index of the
// occurrence of sep in s, or -1 if not present.
so_int bytealg_LastIndexRabinKarp(so_Slice s, so_Slice sep);

// HashStrRev returns the hash of the reverse of sep and the
// appropriate multiplicative factor for use in Rabin-Karp algorithm.
so_R_u32_u32 bytealg_HashStrRev(so_Slice sep);
so_int bytealg_Count(so_Slice b, so_byte c);
so_int bytealg_CountString(so_String s, so_byte c);

// Equal reports whether a and b
// are the same length and contain the same bytes.
// A nil argument is equivalent to an empty slice.
//
// Equal is equivalent to bytes.Equal.
// It is provided here for convenience,
// because some packages cannot depend on bytes.
bool bytealg_Equal(so_Slice a, so_Slice b);
so_int bytealg_IndexByteString(so_String s, so_byte c);
so_int bytealg_LastIndexByte(so_Slice s, so_byte c);
so_int bytealg_LastIndexByteString(so_String s, so_byte c);
