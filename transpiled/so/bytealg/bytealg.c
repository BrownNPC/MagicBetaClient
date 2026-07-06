#include "bytealg.h"

// -- Forward declarations --
static so_R_u32_u32 hashStr(so_Slice sep);

// -- bytealg.go --

// IndexRabinKarp uses the Rabin-Karp search algorithm to return the index of the
// first occurrence of sep in s, or -1 if not present.
so_int bytealg_IndexRabinKarp(so_Slice s, so_Slice sep) {
    // Rabin-Karp search
    so_R_u32_u32 _res1 = hashStr(sep);
    uint32_t hashss = _res1.val;
    uint32_t pow = _res1.val2;
    so_int n = so_len(sep);
    uint32_t h = 0;
    for (so_int i = 0; i < n; i++) {
        h = h * bytealg_PrimeRK + (uint32_t)(so_at(so_byte, s, i));
    }
    if (h == hashss && so_string_eq(so_bytes_string(so_slice(so_byte, s, 0, n)), so_bytes_string(sep))) {
        return 0;
    }
    for (so_int i = n; i < so_len(s);) {
        h *= bytealg_PrimeRK;
        h += (uint32_t)(so_at(so_byte, s, i));
        h -= pow * (uint32_t)(so_at(so_byte, s, i - n));
        i++;
        if (h == hashss && so_string_eq(so_bytes_string(so_slice(so_byte, s, i - n, i)), so_bytes_string(sep))) {
            return i - n;
        }
    }
    return -1;
}

// LastIndexRabinKarp uses the Rabin-Karp search algorithm to return the last index of the
// occurrence of sep in s, or -1 if not present.
so_int bytealg_LastIndexRabinKarp(so_Slice s, so_Slice sep) {
    // Rabin-Karp search from the end of the string
    so_R_u32_u32 _res1 = bytealg_HashStrRev(sep);
    uint32_t hashss = _res1.val;
    uint32_t pow = _res1.val2;
    so_int n = so_len(sep);
    so_int last = so_len(s) - n;
    uint32_t h = 0;
    for (so_int i = so_len(s) - 1; i >= last; i--) {
        h = h * bytealg_PrimeRK + (uint32_t)(so_at(so_byte, s, i));
    }
    if (h == hashss && so_string_eq(so_bytes_string(so_slice(so_byte, s, last, s.len)), so_bytes_string(sep))) {
        return last;
    }
    for (so_int i = last - 1; i >= 0; i--) {
        h *= bytealg_PrimeRK;
        h += (uint32_t)(so_at(so_byte, s, i));
        h -= pow * (uint32_t)(so_at(so_byte, s, i + n));
        if (h == hashss && so_string_eq(so_bytes_string(so_slice(so_byte, s, i, i + n)), so_bytes_string(sep))) {
            return i;
        }
    }
    return -1;
}

// hashStr returns the hash and the appropriate multiplicative
// factor for use in Rabin-Karp algorithm.
static so_R_u32_u32 hashStr(so_Slice sep) {
    uint32_t hash = (uint32_t)(0);
    for (so_int i = 0; i < so_len(sep); i++) {
        hash = hash * bytealg_PrimeRK + (uint32_t)(so_at(so_byte, sep, i));
    }
    uint32_t pow = 1, sq = bytealg_PrimeRK;
    for (so_int i = so_len(sep); i > 0; i >>= 1) {
        if ((i & 1) != 0) {
            pow *= sq;
        }
        sq *= sq;
    }
    return (so_R_u32_u32){.val = hash, .val2 = pow};
}

// HashStrRev returns the hash of the reverse of sep and the
// appropriate multiplicative factor for use in Rabin-Karp algorithm.
so_R_u32_u32 bytealg_HashStrRev(so_Slice sep) {
    uint32_t hash = (uint32_t)(0);
    for (so_int i = so_len(sep) - 1; i >= 0; i--) {
        hash = hash * bytealg_PrimeRK + (uint32_t)(so_at(so_byte, sep, i));
    }
    uint32_t pow = 1, sq = bytealg_PrimeRK;
    for (so_int i = so_len(sep); i > 0; i >>= 1) {
        if ((i & 1) != 0) {
            pow *= sq;
        }
        sq *= sq;
    }
    return (so_R_u32_u32){.val = hash, .val2 = pow};
}

// -- compare.go --

// -- count.go --

so_int bytealg_Count(so_Slice b, so_byte c) {
    so_int n = 0;
    for (so_int _ = 0; _ < so_len(b); _++) {
        so_byte x = so_at(so_byte, b, _);
        if (x == c) {
            n++;
        }
    }
    return n;
}

so_int bytealg_CountString(so_String s, so_byte c) {
    so_int n = 0;
    for (so_int i = 0; i < so_len(s); i++) {
        if (so_at(so_byte, s, i) == c) {
            n++;
        }
    }
    return n;
}

// -- equal.go --

// Equal reports whether a and b
// are the same length and contain the same bytes.
// A nil argument is equivalent to an empty slice.
//
// Equal is equivalent to bytes.Equal.
// It is provided here for convenience,
// because some packages cannot depend on bytes.
bool bytealg_Equal(so_Slice a, so_Slice b) {
    return so_string_eq(so_bytes_string(a), so_bytes_string(b));
}

// -- index.go --

so_int bytealg_IndexByteString(so_String s, so_byte c) {
    return bytealg_IndexByte(so_string_bytes(s), c);
}

so_int bytealg_LastIndexByte(so_Slice s, so_byte c) {
    for (so_int i = so_len(s) - 1; i >= 0; i--) {
        if (so_at(so_byte, s, i) == c) {
            return i;
        }
    }
    return -1;
}

so_int bytealg_LastIndexByteString(so_String s, so_byte c) {
    return bytealg_LastIndexByte(so_string_bytes(s), c);
}
