#include "utf8.h"

// -- Types --

typedef struct acceptRange acceptRange;

// acceptRange gives the range of valid values for the second byte in a UTF-8
// sequence.
typedef struct acceptRange {
    uint8_t lo;
    uint8_t hi;
} acceptRange;

// -- Forward declarations --
static so_R_rune_int decodeRuneSlow(so_Slice p);
static so_R_rune_int decodeRuneInStringSlow(so_String s);
static so_int encodeRuneNonASCII(so_Slice p, so_rune r);
static so_Slice appendRuneNonASCII(so_Slice p, so_rune r);
static uintptr_t wordSlice(so_Slice s);
static uintptr_t wordString(so_String s);

// -- Variables and constants --

// Code points in the surrogate range are not valid for UTF-8.
static const int64_t surrogateMin = 0xD800;
static const int64_t surrogateMax = 0xDFFF;
static const int64_t tx = 0b10000000;
static const int64_t t2 = 0b11000000;
static const int64_t t3 = 0b11100000;
static const int64_t t4 = 0b11110000;
static const int64_t maskx = 0b00111111;
static const int64_t mask2 = 0b00011111;
static const int64_t mask3 = 0b00001111;
static const int64_t mask4 = 0b00000111;
static const int64_t rune1Max = ((int64_t)1 << 7) - 1;
static const int64_t rune2Max = ((int64_t)1 << 11) - 1;
static const int64_t rune3Max = ((int64_t)1 << 16) - 1;
static const int64_t locb = 0b10000000;
static const int64_t hicb = 0b10111111;
static const int64_t xx = 0xF1;
static const int64_t as = 0xF0;
static const int64_t s1 = 0x02;
static const int64_t s2 = 0x13;
static const int64_t s3 = 0x03;
static const int64_t s4 = 0x23;
static const int64_t s5 = 0x34;
static const int64_t s6 = 0x04;
static const int64_t s7 = 0x44;
static const so_rune runeErrorByte0 = (t3 | (utf8_RuneError >> 12));
static const so_rune runeErrorByte1 = (tx | ((utf8_RuneError >> 6) & maskx));
static const so_rune runeErrorByte2 = (tx | (utf8_RuneError & maskx));

// first is information about the first byte in a UTF-8 sequence.
static uint8_t first[256] = {as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s2, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s4, s3, s3, s5, s6, s6, s6, s7, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx};

// acceptRanges has size 16 to avoid bounds checks in the code that uses it.
static acceptRange acceptRanges[16] = {[0] = (acceptRange){locb, hicb}, [1] = (acceptRange){0xA0, hicb}, [2] = (acceptRange){locb, 0x9F}, [3] = (acceptRange){0x90, hicb}, [4] = (acceptRange){locb, 0x8F}};
static const int64_t ptrSize = ((int64_t)4 << ((uint64_t)(~(uintptr_t)(0)) >> 63));
static const int64_t hiBits = ((int64_t)0x8080808080808080 >> (64 - 8 * ptrSize));

// -- Implementation --

// FullRune reports whether the bytes in p begin with a full UTF-8 encoding of a rune.
// An invalid encoding is considered a full Rune since it will convert as a width-1 error rune.
bool utf8_FullRune(so_Slice p) {
    so_int n = so_len(p);
    if (n == 0) {
        return false;
    }
    uint8_t x = first[so_at(so_byte, p, 0)];
    if (n >= (so_int)(x & 7)) {
        // ASCII, invalid or valid.
        return true;
    }
    // Must be short or invalid.
    acceptRange accept = acceptRanges[(x >> 4)];
    if (n > 1 && (so_at(so_byte, p, 1) < accept.lo || accept.hi < so_at(so_byte, p, 1))) {
        return true;
    } else if (n > 2 && (so_at(so_byte, p, 2) < locb || hicb < so_at(so_byte, p, 2))) {
        return true;
    }
    return false;
}

// FullRuneInString is like FullRune but its input is a string.
bool utf8_FullRuneInString(so_String s) {
    so_int n = so_len(s);
    if (n == 0) {
        return false;
    }
    uint8_t x = first[so_at(so_byte, s, 0)];
    if (n >= (so_int)(x & 7)) {
        // ASCII, invalid, or valid.
        return true;
    }
    // Must be short or invalid.
    acceptRange accept = acceptRanges[(x >> 4)];
    if (n > 1 && (so_at(so_byte, s, 1) < accept.lo || accept.hi < so_at(so_byte, s, 1))) {
        return true;
    } else if (n > 2 && (so_at(so_byte, s, 2) < locb || hicb < so_at(so_byte, s, 2))) {
        return true;
    }
    return false;
}

// DecodeRune unpacks the first UTF-8 encoding in p and returns the rune and
// its width in bytes. If p is empty it returns ([RuneError], 0). Otherwise, if
// the encoding is invalid, it returns (RuneError, 1). Both are impossible
// results for correct, non-empty UTF-8.
//
// An encoding is invalid if it is incorrect UTF-8, encodes a rune that is
// out of range, or is not the shortest possible UTF-8 encoding for the
// value. No other validation is performed.
so_R_rune_int utf8_DecodeRune(so_Slice p) {
    // Inlineable fast path for ASCII characters; see #48195.
    // This implementation is weird but effective at rendering the
    // function inlineable.
    for (so_int _ = 0; _ < so_len(p); _++) {
        so_byte b = so_at(so_byte, p, _);
        if (b < utf8_RuneSelf) {
            return (so_R_rune_int){.val = (so_rune)(b), .val2 = 1};
        }
        break;
    }
    return decodeRuneSlow(p);
}

static so_R_rune_int decodeRuneSlow(so_Slice p) {
    so_int n = so_len(p);
    if (n < 1) {
        return (so_R_rune_int){.val = utf8_RuneError, .val2 = 0};
    }
    so_byte p0 = so_at(so_byte, p, 0);
    uint8_t x = first[p0];
    if (x >= as) {
        // The following code simulates an additional check for x == xx and
        // handling the ASCII and invalid cases accordingly. This mask-and-or
        // approach prevents an additional branch.
        // Create 0x0000 or 0xFFFF.
        so_rune mask = (((so_rune)(x) << 31) >> 31);
        so_rune r = (((so_rune)(so_at(so_byte, p, 0)) & ~mask) | (utf8_RuneError & mask));
        return (so_R_rune_int){.val = r, .val2 = 1};
    }
    so_int sz = (so_int)(x & 7);
    acceptRange accept = acceptRanges[(x >> 4)];
    if (n < sz) {
        return (so_R_rune_int){.val = utf8_RuneError, .val2 = 1};
    }
    so_byte b1 = so_at(so_byte, p, 1);
    if (b1 < accept.lo || accept.hi < b1) {
        return (so_R_rune_int){.val = utf8_RuneError, .val2 = 1};
    }
    if (sz <= 2) {
        // <= instead of == to help the compiler eliminate some bounds checks
        so_rune r = (((so_rune)(p0 & mask2) << 6) | (so_rune)(b1 & maskx));
        return (so_R_rune_int){.val = r, .val2 = 2};
    }
    so_byte b2 = so_at(so_byte, p, 2);
    if (b2 < locb || hicb < b2) {
        return (so_R_rune_int){.val = utf8_RuneError, .val2 = 1};
    }
    if (sz <= 3) {
        so_rune r = ((((so_rune)(p0 & mask3) << 12) | ((so_rune)(b1 & maskx) << 6)) | (so_rune)(b2 & maskx));
        return (so_R_rune_int){.val = r, .val2 = 3};
    }
    so_byte b3 = so_at(so_byte, p, 3);
    if (b3 < locb || hicb < b3) {
        return (so_R_rune_int){.val = utf8_RuneError, .val2 = 1};
    }
    so_rune r = (((((so_rune)(p0 & mask4) << 18) | ((so_rune)(b1 & maskx) << 12)) | ((so_rune)(b2 & maskx) << 6)) | (so_rune)(b3 & maskx));
    return (so_R_rune_int){.val = r, .val2 = 4};
}

// DecodeRuneInString is like [DecodeRune] but its input is a string. If s is
// empty it returns ([RuneError], 0). Otherwise, if the encoding is invalid, it
// returns (RuneError, 1). Both are impossible results for correct, non-empty
// UTF-8.
//
// An encoding is invalid if it is incorrect UTF-8, encodes a rune that is
// out of range, or is not the shortest possible UTF-8 encoding for the
// value. No other validation is performed.
so_R_rune_int utf8_DecodeRuneInString(so_String s) {
    // Inlineable fast path for ASCII characters; see #48195.
    // This implementation is a bit weird but effective at rendering the
    // function inlineable.
    if (so_string_ne(s, so_str("")) && so_at(so_byte, s, 0) < utf8_RuneSelf) {
        return (so_R_rune_int){.val = (so_rune)(so_at(so_byte, s, 0)), .val2 = 1};
    } else {
        return decodeRuneInStringSlow(s);
    }
}

static so_R_rune_int decodeRuneInStringSlow(so_String s) {
    so_int n = so_len(s);
    if (n < 1) {
        return (so_R_rune_int){.val = utf8_RuneError, .val2 = 0};
    }
    so_byte s0 = so_at(so_byte, s, 0);
    uint8_t x = first[s0];
    if (x >= as) {
        // The following code simulates an additional check for x == xx and
        // handling the ASCII and invalid cases accordingly. This mask-and-or
        // approach prevents an additional branch.
        // Create 0x0000 or 0xFFFF.
        so_rune mask = (((so_rune)(x) << 31) >> 31);
        so_rune r = (((so_rune)(so_at(so_byte, s, 0)) & ~mask) | (utf8_RuneError & mask));
        return (so_R_rune_int){.val = r, .val2 = 1};
    }
    so_int sz = (so_int)(x & 7);
    acceptRange accept = acceptRanges[(x >> 4)];
    if (n < sz) {
        return (so_R_rune_int){.val = utf8_RuneError, .val2 = 1};
    }
    so_byte s1 = so_at(so_byte, s, 1);
    if (s1 < accept.lo || accept.hi < s1) {
        return (so_R_rune_int){.val = utf8_RuneError, .val2 = 1};
    }
    if (sz <= 2) {
        // <= instead of == to help the compiler eliminate some bounds checks
        so_rune r = (((so_rune)(s0 & mask2) << 6) | (so_rune)(s1 & maskx));
        return (so_R_rune_int){.val = r, .val2 = 2};
    }
    so_byte s2 = so_at(so_byte, s, 2);
    if (s2 < locb || hicb < s2) {
        return (so_R_rune_int){.val = utf8_RuneError, .val2 = 1};
    }
    if (sz <= 3) {
        so_rune r = ((((so_rune)(s0 & mask3) << 12) | ((so_rune)(s1 & maskx) << 6)) | (so_rune)(s2 & maskx));
        return (so_R_rune_int){.val = r, .val2 = 3};
    }
    so_byte s3 = so_at(so_byte, s, 3);
    if (s3 < locb || hicb < s3) {
        return (so_R_rune_int){.val = utf8_RuneError, .val2 = 1};
    }
    so_rune r = (((((so_rune)(s0 & mask4) << 18) | ((so_rune)(s1 & maskx) << 12)) | ((so_rune)(s2 & maskx) << 6)) | (so_rune)(s3 & maskx));
    return (so_R_rune_int){.val = r, .val2 = 4};
}

// DecodeLastRune unpacks the last UTF-8 encoding in p and returns the rune and
// its width in bytes. If p is empty it returns ([RuneError], 0). Otherwise, if
// the encoding is invalid, it returns (RuneError, 1). Both are impossible
// results for correct, non-empty UTF-8.
//
// An encoding is invalid if it is incorrect UTF-8, encodes a rune that is
// out of range, or is not the shortest possible UTF-8 encoding for the
// value. No other validation is performed.
so_R_rune_int utf8_DecodeLastRune(so_Slice p) {
    so_int end = so_len(p);
    if (end == 0) {
        return (so_R_rune_int){.val = utf8_RuneError, .val2 = 0};
    }
    so_int start = end - 1;
    so_rune r = (so_rune)(so_at(so_byte, p, start));
    if (r < utf8_RuneSelf) {
        return (so_R_rune_int){.val = r, .val2 = 1};
    }
    // guard against O(n^2) behavior when traversing
    // backwards through strings with long sequences of
    // invalid UTF-8.
    so_int lim = so_max(end - utf8_UTFMax, 0);
    for (start--; start >= lim; start--) {
        if (utf8_RuneStart(so_at(so_byte, p, start))) {
            break;
        }
    }
    if (start < 0) {
        start = 0;
    }
    so_R_rune_int _res1 = utf8_DecodeRune(so_slice(so_byte, p, start, end));
    r = _res1.val;
    so_int size = _res1.val2;
    if (start + size != end) {
        return (so_R_rune_int){.val = utf8_RuneError, .val2 = 1};
    }
    return (so_R_rune_int){.val = r, .val2 = size};
}

// DecodeLastRuneInString is like [DecodeLastRune] but its input is a string. If
// s is empty it returns ([RuneError], 0). Otherwise, if the encoding is invalid,
// it returns (RuneError, 1). Both are impossible results for correct,
// non-empty UTF-8.
//
// An encoding is invalid if it is incorrect UTF-8, encodes a rune that is
// out of range, or is not the shortest possible UTF-8 encoding for the
// value. No other validation is performed.
so_R_rune_int utf8_DecodeLastRuneInString(so_String s) {
    so_int end = so_len(s);
    if (end == 0) {
        return (so_R_rune_int){.val = utf8_RuneError, .val2 = 0};
    }
    so_int start = end - 1;
    so_rune r = (so_rune)(so_at(so_byte, s, start));
    if (r < utf8_RuneSelf) {
        return (so_R_rune_int){.val = r, .val2 = 1};
    }
    // guard against O(n^2) behavior when traversing
    // backwards through strings with long sequences of
    // invalid UTF-8.
    so_int lim = so_max(end - utf8_UTFMax, 0);
    for (start--; start >= lim; start--) {
        if (utf8_RuneStart(so_at(so_byte, s, start))) {
            break;
        }
    }
    if (start < 0) {
        start = 0;
    }
    so_R_rune_int _res1 = utf8_DecodeRuneInString(so_string_slice(s, start, end));
    r = _res1.val;
    so_int size = _res1.val2;
    if (start + size != end) {
        return (so_R_rune_int){.val = utf8_RuneError, .val2 = 1};
    }
    return (so_R_rune_int){.val = r, .val2 = size};
}

// RuneLen returns the number of bytes in the UTF-8 encoding of the rune.
// It returns -1 if the rune is not a valid value to encode in UTF-8.
so_int utf8_RuneLen(so_rune r) {
    if (r < 0) {
        return -1;
    } else if (r <= rune1Max) {
        return 1;
    } else if (r <= rune2Max) {
        return 2;
    } else if (surrogateMin <= r && r <= surrogateMax) {
        return -1;
    } else if (r <= rune3Max) {
        return 3;
    } else if (r <= utf8_MaxRune) {
        return 4;
    }
    return -1;
}

// EncodeRune writes into p (which must be large enough) the UTF-8 encoding of the rune.
// If the rune is out of range, it writes the encoding of [RuneError].
// It returns the number of bytes written.
so_int utf8_EncodeRune(so_Slice p, so_rune r) {
    // This function is inlineable for fast handling of ASCII.
    if ((uint32_t)(r) <= rune1Max) {
        so_at(so_byte, p, 0) = (so_byte)(r);
        return 1;
    }
    return encodeRuneNonASCII(p, r);
}

static so_int encodeRuneNonASCII(so_Slice p, so_rune r) {
    // Negative values are erroneous. Making it unsigned addresses the problem.
    uint32_t i = (uint32_t)(r);
    if (i <= rune2Max) {
        // eliminate bounds checks
        (void)so_at(so_byte, p, 1);
        so_at(so_byte, p, 0) = (t2 | (so_byte)(r >> 6));
        so_at(so_byte, p, 1) = (tx | ((so_byte)(r) & maskx));
        return 2;
    } else if ((i < surrogateMin) || (surrogateMax < i && i <= rune3Max)) {
        // eliminate bounds checks
        (void)so_at(so_byte, p, 2);
        so_at(so_byte, p, 0) = (t3 | (so_byte)(r >> 12));
        so_at(so_byte, p, 1) = (tx | ((so_byte)(r >> 6) & maskx));
        so_at(so_byte, p, 2) = (tx | ((so_byte)(r) & maskx));
        return 3;
    } else if (i > rune3Max && i <= (uint32_t)(utf8_MaxRune)) {
        // eliminate bounds checks
        (void)so_at(so_byte, p, 3);
        so_at(so_byte, p, 0) = (t4 | (so_byte)(r >> 18));
        so_at(so_byte, p, 1) = (tx | ((so_byte)(r >> 12) & maskx));
        so_at(so_byte, p, 2) = (tx | ((so_byte)(r >> 6) & maskx));
        so_at(so_byte, p, 3) = (tx | ((so_byte)(r) & maskx));
        return 4;
    } else {
        // eliminate bounds checks
        (void)so_at(so_byte, p, 2);
        so_at(so_byte, p, 0) = runeErrorByte0;
        so_at(so_byte, p, 1) = runeErrorByte1;
        so_at(so_byte, p, 2) = runeErrorByte2;
        return 3;
    }
}

// AppendRune appends the UTF-8 encoding of r to the end of p and
// returns the extended buffer. If the rune is out of range,
// it appends the encoding of [RuneError].
//
// Requires spare p capacity: at least [UTFMax] bytes.
so_Slice utf8_AppendRune(so_Slice p, so_rune r) {
    // This function is inlineable for fast handling of ASCII.
    if ((uint32_t)(r) <= rune1Max) {
        return so_append(so_byte, p, (so_byte)(r));
    }
    return appendRuneNonASCII(p, r);
}

static so_Slice appendRuneNonASCII(so_Slice p, so_rune r) {
    // Negative values are erroneous. Making it unsigned addresses the problem.
    uint32_t i = (uint32_t)(r);
    if (i <= rune2Max) {
        return so_append(so_byte, p, (t2 | (so_byte)(r >> 6)), (tx | ((so_byte)(r) & maskx)));
    } else if ((i < surrogateMin) || (surrogateMax < i && i <= rune3Max)) {
        return so_append(so_byte, p, (t3 | (so_byte)(r >> 12)), (tx | ((so_byte)(r >> 6) & maskx)), (tx | ((so_byte)(r) & maskx)));
    } else if (i > rune3Max && i <= (uint32_t)(utf8_MaxRune)) {
        return so_append(so_byte, p, (t4 | (so_byte)(r >> 18)), (tx | ((so_byte)(r >> 12) & maskx)), (tx | ((so_byte)(r >> 6) & maskx)), (tx | ((so_byte)(r) & maskx)));
    } else {
        return so_append(so_byte, p, runeErrorByte0, runeErrorByte1, runeErrorByte2);
    }
}

// RuneCount returns the number of runes in p. Erroneous and short
// encodings are treated as single runes of width 1 byte.
so_int utf8_RuneCount(so_Slice p) {
    so_int np = so_len(p);
    so_int n = 0;
    for (; n < np; n++) {
        {
            so_byte c = so_at(so_byte, p, n);
            if (c >= utf8_RuneSelf) {
                // non-ASCII slow path
                return n + utf8_RuneCountInString(so_bytes_string(so_slice(so_byte, p, n, p.len)));
            }
        }
    }
    return n;
}

// RuneCountInString is like [RuneCount] but its input is a string.
so_int utf8_RuneCountInString(so_String s) {
    so_int n = 0;
    for (so_int _i = 0, _iw = 0; _i < so_len(s); _i += _iw) {
        _iw = 0;
        so_utf8_decode(s, _i, &_iw);
        n++;
    }
    return n;
}

// RuneStart reports whether the byte could be the first byte of an encoded,
// possibly invalid rune. Second and subsequent bytes always have the top two
// bits set to 10.
bool utf8_RuneStart(so_byte b) {
    return (b & 0xC0) != 0x80;
}

static uintptr_t wordSlice(so_Slice s) {
    if (ptrSize == 4) {
        return ((((uintptr_t)(so_at(so_byte, s, 0)) | ((uintptr_t)(so_at(so_byte, s, 1)) << 8)) | ((uintptr_t)(so_at(so_byte, s, 2)) << 16)) | ((uintptr_t)(so_at(so_byte, s, 3)) << 24));
    }
    return (uintptr_t)((((((((uint64_t)(so_at(so_byte, s, 0)) | ((uint64_t)(so_at(so_byte, s, 1)) << 8)) | ((uint64_t)(so_at(so_byte, s, 2)) << 16)) | ((uint64_t)(so_at(so_byte, s, 3)) << 24)) | ((uint64_t)(so_at(so_byte, s, 4)) << 32)) | ((uint64_t)(so_at(so_byte, s, 5)) << 40)) | ((uint64_t)(so_at(so_byte, s, 6)) << 48)) | ((uint64_t)(so_at(so_byte, s, 7)) << 56));
}

static uintptr_t wordString(so_String s) {
    if (ptrSize == 4) {
        return ((((uintptr_t)(so_at(so_byte, s, 0)) | ((uintptr_t)(so_at(so_byte, s, 1)) << 8)) | ((uintptr_t)(so_at(so_byte, s, 2)) << 16)) | ((uintptr_t)(so_at(so_byte, s, 3)) << 24));
    }
    return (uintptr_t)((((((((uint64_t)(so_at(so_byte, s, 0)) | ((uint64_t)(so_at(so_byte, s, 1)) << 8)) | ((uint64_t)(so_at(so_byte, s, 2)) << 16)) | ((uint64_t)(so_at(so_byte, s, 3)) << 24)) | ((uint64_t)(so_at(so_byte, s, 4)) << 32)) | ((uint64_t)(so_at(so_byte, s, 5)) << 40)) | ((uint64_t)(so_at(so_byte, s, 6)) << 48)) | ((uint64_t)(so_at(so_byte, s, 7)) << 56));
}

// Valid reports whether p consists entirely of valid UTF-8-encoded runes.
bool utf8_Valid(so_Slice p) {
    // This optimization avoids the need to recompute the capacity
    // when generating code for slicing p, bringing it to parity with
    // ValidString, which was 20% faster on long ASCII strings.
    p = so_slice3(so_byte, p, 0, so_len(p), so_len(p));
    for (; so_len(p) > 0;) {
        so_byte p0 = so_at(so_byte, p, 0);
        if (p0 < utf8_RuneSelf) {
            p = so_slice(so_byte, p, 1, p.len);
            // If there's one ASCII byte, there are probably more.
            // Advance quickly through ASCII-only data.
            // Note: using > instead of >= here is intentional. That avoids
            // needing pointing-past-the-end fixup on the slice operations.
            if (so_len(p) > ptrSize && (wordSlice(p) & hiBits) == 0) {
                p = so_slice(so_byte, p, ptrSize, p.len);
                if (so_len(p) > 2 * ptrSize && ((wordSlice(p) | wordSlice(so_slice(so_byte, p, ptrSize, p.len))) & hiBits) == 0) {
                    p = so_slice(so_byte, p, 2 * ptrSize, p.len);
                    for (; so_len(p) > 4 * ptrSize && (((wordSlice(p) | wordSlice(so_slice(so_byte, p, ptrSize, p.len))) | (wordSlice(so_slice(so_byte, p, 2 * ptrSize, p.len)) | wordSlice(so_slice(so_byte, p, 3 * ptrSize, p.len)))) & hiBits) == 0;) {
                        p = so_slice(so_byte, p, 4 * ptrSize, p.len);
                    }
                }
            }
            continue;
        }
        uint8_t x = first[p0];
        so_int size = (so_int)(x & 7);
        acceptRange accept = acceptRanges[(x >> 4)];
        if (size == 2) {
            if (so_len(p) < 2 || so_at(so_byte, p, 1) < accept.lo || accept.hi < so_at(so_byte, p, 1)) {
                return false;
            }
            p = so_slice(so_byte, p, 2, p.len);
        } else if (size == 3) {
            if (so_len(p) < 3 || so_at(so_byte, p, 1) < accept.lo || accept.hi < so_at(so_byte, p, 1) || so_at(so_byte, p, 2) < locb || hicb < so_at(so_byte, p, 2)) {
                return false;
            }
            p = so_slice(so_byte, p, 3, p.len);
        } else if (size == 4) {
            if (so_len(p) < 4 || so_at(so_byte, p, 1) < accept.lo || accept.hi < so_at(so_byte, p, 1) || so_at(so_byte, p, 2) < locb || hicb < so_at(so_byte, p, 2) || so_at(so_byte, p, 3) < locb || hicb < so_at(so_byte, p, 3)) {
                return false;
            }
            p = so_slice(so_byte, p, 4, p.len);
        } else {
            // illegal starter byte
            return false;
        }
    }
    return true;
}

// ValidString reports whether s consists entirely of valid UTF-8-encoded runes.
bool utf8_ValidString(so_String s) {
    for (; so_len(s) > 0;) {
        so_byte s0 = so_at(so_byte, s, 0);
        if (s0 < utf8_RuneSelf) {
            s = so_string_slice(s, 1, s.len);
            // If there's one ASCII byte, there are probably more.
            // Advance quickly through ASCII-only data.
            // Note: using > instead of >= here is intentional. That avoids
            // needing pointing-past-the-end fixup on the slice operations.
            if (so_len(s) > ptrSize && (wordString(s) & hiBits) == 0) {
                s = so_string_slice(s, ptrSize, s.len);
                if (so_len(s) > 2 * ptrSize && ((wordString(s) | wordString(so_string_slice(s, ptrSize, s.len))) & hiBits) == 0) {
                    s = so_string_slice(s, 2 * ptrSize, s.len);
                    for (; so_len(s) > 4 * ptrSize && (((wordString(s) | wordString(so_string_slice(s, ptrSize, s.len))) | (wordString(so_string_slice(s, 2 * ptrSize, s.len)) | wordString(so_string_slice(s, 3 * ptrSize, s.len)))) & hiBits) == 0;) {
                        s = so_string_slice(s, 4 * ptrSize, s.len);
                    }
                }
            }
            continue;
        }
        uint8_t x = first[s0];
        so_int size = (so_int)(x & 7);
        acceptRange accept = acceptRanges[(x >> 4)];
        if (size == 2) {
            if (so_len(s) < 2 || so_at(so_byte, s, 1) < accept.lo || accept.hi < so_at(so_byte, s, 1)) {
                return false;
            }
            s = so_string_slice(s, 2, s.len);
        } else if (size == 3) {
            if (so_len(s) < 3 || so_at(so_byte, s, 1) < accept.lo || accept.hi < so_at(so_byte, s, 1) || so_at(so_byte, s, 2) < locb || hicb < so_at(so_byte, s, 2)) {
                return false;
            }
            s = so_string_slice(s, 3, s.len);
        } else if (size == 4) {
            if (so_len(s) < 4 || so_at(so_byte, s, 1) < accept.lo || accept.hi < so_at(so_byte, s, 1) || so_at(so_byte, s, 2) < locb || hicb < so_at(so_byte, s, 2) || so_at(so_byte, s, 3) < locb || hicb < so_at(so_byte, s, 3)) {
                return false;
            }
            s = so_string_slice(s, 4, s.len);
        } else {
            // illegal starter byte
            return false;
        }
    }
    return true;
}

// ValidRune reports whether r can be legally encoded as UTF-8.
// Code points that are out of range or a surrogate half are illegal.
bool utf8_ValidRune(so_rune r) {
    if ((0 <= r) && (r < surrogateMin)) {
        return true;
    } else if ((surrogateMax < r) && (r <= utf8_MaxRune)) {
        return true;
    }
    return false;
}
