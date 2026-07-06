#include "strings.h"

// -- Types --

typedef struct asciiSet asciiSet;
typedef struct span span;

// asciiSet is a 32-byte value, where each bit represents the presence of a
// given ASCII character in the set. The 128-bits of the lower 16 bytes,
// starting with the least-significant bit of the lowest word to the
// most-significant bit of the highest word, map to the full range of all
// 128 ASCII characters. The 128-bits of the upper 16 bytes will be zeroed,
// ensuring that any non-ASCII character will be reported as not in the set.
// This allocates a total of 32 bytes even though the upper half
// is unused to avoid bounds checks in asciiSet.contains.
typedef struct asciiSet {
    uint32_t val[8];
    bool ok;
} asciiSet;

// span is used to record a slice of s of the form s[start:end].
// The start index is inclusive and the end index is exclusive.
typedef struct span {
    so_int start;
    so_int end;
} span;

// -- Forward declarations --
static asciiSet makeASCIISet(so_String chars);
static bool asciiSet_contains(void* self, so_byte c);
static so_int indexFunc(so_String s, strings_RunePredicate f, bool truth);
static so_int lastIndexFunc(so_String s, strings_RunePredicate f, bool truth);
static so_Slice genSplit(mem_Allocator a, so_String s, so_String sep, so_int sepSave, so_int n);
static so_Slice explode(mem_Allocator a, so_String s, so_int n);
static so_String trimLeftFunc(so_String s, strings_RunePredicate f);
static so_String trimRightFunc(so_String s, strings_RunePredicate f);
static so_String trimLeftByte(so_String s, so_byte c);
static so_String trimLeftASCII(so_String s, asciiSet* as);
static so_String trimLeftUnicode(so_String s, so_String cutset);
static so_String trimRightByte(so_String s, so_byte c);
static so_String trimRightASCII(so_String s, asciiSet* as);
static so_String trimRightUnicode(so_String s, so_String cutset);

// -- Variables and constants --

// maxInt is the maximum value of an int.
static const so_int maxInt = (so_int)((uint64_t)(~(so_uint)(0)) >> 1);

// According to static analysis, spaces, dashes, zeros, equals, and tabs
// are the most commonly repeated string literal,
// often used for display on fixed-width terminal windows.
// Pre-declare constants for these for O(1) repetition in the common-case.
static const so_String repeatedSpaces = so_str("" "                                                                " "                                                                ");
static const so_String repeatedDashes = so_str("" "----------------------------------------------------------------" "----------------------------------------------------------------");
static const so_String repeatedZeroes = so_str("" "0000000000000000000000000000000000000000000000000000000000000000");
static const so_String repeatedEquals = so_str("" "================================================================" "================================================================");
static const so_String repeatedTabs = so_str("" "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
static uint8_t asciiSpace[256] = {[U'\t'] = 1, [U'\n'] = 1, [U'\v'] = 1, [U'\f'] = 1, [U'\r'] = 1, [U' '] = 1};

// -- builder.go --

// NewBuilder returns a new Builder that uses the provided allocator.
strings_Builder strings_NewBuilder(mem_Allocator a) {
    return (strings_Builder){.a = a};
}

// FixedBuilder returns a new Builder that uses the provided buffer
// as its internal buffer. The builder doesn't take ownership of the
// buffer and doesn't free it (Free is a no-op). The builder doesn't
// allocate additional memory, so writes that exceed the buffer's
// capacity will panic.
strings_Builder strings_FixedBuilder(so_Slice buf) {
    return (strings_Builder){.a = mem_NoAlloc, .buf = so_slice(so_byte, buf, 0, 0)};
}

// String returns the accumulated string.
so_String strings_Builder_String(void* self) {
    strings_Builder* b = self;
    return so_bytes_string(b->buf);
}

// Len returns the number of accumulated bytes; b.Len() == len(b.String()).
so_int strings_Builder_Len(void* self) {
    strings_Builder* b = self;
    return so_len(b->buf);
}

// Cap returns the capacity of the builder's underlying byte slice. It is the
// total space allocated for the string being built and includes any bytes
// already written.
so_int strings_Builder_Cap(void* self) {
    strings_Builder* b = self;
    return so_cap(b->buf);
}

// Reset resets the builder to be empty without freeing the underlying buffer.
void strings_Builder_Reset(void* self) {
    strings_Builder* b = self;
    b->buf = so_slice(so_byte, b->buf, 0, 0);
}

// Free frees the internal buffer and resets the builder.
// After Free, the builder can be reused with new writes.
void strings_Builder_Free(void* self) {
    strings_Builder* b = self;
    mem_FreeSlice(so_byte, (b->a), (b->buf));
    b->buf = (so_Slice){0};
}

// Grow grows b's capacity, if necessary, to guarantee space for
// another n bytes. After Grow(n), at least n bytes can be written to b
// without another allocation. If n is negative, Grow panics.
void strings_Builder_Grow(void* self, so_int n) {
    strings_Builder* b = self;
    if (n < 0) {
        so_panic("strings: negative grow");
    }
    strings_Builder_grow(b, n);
}

// WriteRune appends the UTF-8 encoding of Unicode code point r to b's buffer.
// It returns the length of r and a nil error.
so_R_int_err strings_Builder_WriteRune(void* self, so_rune r) {
    strings_Builder* b = self;
    strings_Builder_grow(b, utf8_UTFMax);
    so_int n = so_len(b->buf);
    b->buf = utf8_AppendRune(b->buf, r);
    return (so_R_int_err){.val = so_len(b->buf) - n, .err = (so_Error){0}};
}

// -- index.go --

// makeASCIISet creates a set of ASCII characters and reports whether all
// characters in chars are ASCII.
static asciiSet makeASCIISet(so_String chars) {
    asciiSet as = {0};
    for (so_int i = 0; i < so_len(chars); i++) {
        so_byte c = so_at(so_byte, chars, i);
        if (c >= utf8_RuneSelf) {
            return as;
        }
        as.val[c / 32] |= ((uint32_t)1 << (c % 32));
    }
    as.ok = true;
    return as;
}

// contains reports whether c is inside the set.
static bool asciiSet_contains(void* self, so_byte c) {
    asciiSet* as = self;
    return (as->val[c / 32] & ((uint32_t)1 << (c % 32))) != 0;
}

// Contains reports whether substr is within s.
bool strings_Contains(so_String s, so_String substr) {
    return strings_Index(s, substr) >= 0;
}

// ContainsAny reports whether any Unicode code points in chars are within s.
bool strings_ContainsAny(so_String s, so_String chars) {
    return strings_IndexAny(s, chars) >= 0;
}

// ContainsRune reports whether the Unicode code point r is within s.
bool strings_ContainsRune(so_String s, so_rune r) {
    return strings_IndexRune(s, r) >= 0;
}

// ContainsFunc reports whether any Unicode code points r within s satisfy f(r).
bool strings_ContainsFunc(so_String s, strings_RunePredicate f) {
    return strings_IndexFunc(s, f) >= 0;
}

// Index returns the index of the first instance of substr in s, or -1 if substr is not present in s.
so_int strings_Index(so_String s, so_String substr) {
    return stringslite_Index(s, substr);
}

// LastIndex returns the index of the last instance of substr in s, or -1 if substr is not present in s.
so_int strings_LastIndex(so_String s, so_String substr) {
    so_int n = so_len(substr);
    if (n == 0) {
        return so_len(s);
    } else if (n == 1) {
        return bytealg_LastIndexByteString(s, so_at(so_byte, substr, 0));
    } else if (n == so_len(s)) {
        if (so_string_eq(substr, s)) {
            return 0;
        }
        return -1;
    } else if (n > so_len(s)) {
        return -1;
    }
    // Rabin-Karp search from the end of the string
    so_R_u32_u32 _res1 = bytealg_HashStrRev(so_string_bytes(substr));
    uint32_t hashss = _res1.val;
    uint32_t pow = _res1.val2;
    so_int last = so_len(s) - n;
    uint32_t h = 0;
    for (so_int i = so_len(s) - 1; i >= last; i--) {
        h = h * bytealg_PrimeRK + (uint32_t)(so_at(so_byte, s, i));
    }
    if (h == hashss && so_string_eq(so_string_slice(s, last, s.len), substr)) {
        return last;
    }
    for (so_int i = last - 1; i >= 0; i--) {
        h *= bytealg_PrimeRK;
        h += (uint32_t)(so_at(so_byte, s, i));
        h -= pow * (uint32_t)(so_at(so_byte, s, i + n));
        if (h == hashss && so_string_eq(so_string_slice(s, i, i + n), substr)) {
            return i;
        }
    }
    return -1;
}

// IndexByte returns the index of the first instance of c in s, or -1 if c is not present in s.
so_int strings_IndexByte(so_String s, so_byte c) {
    return stringslite_IndexByte(s, c);
}

// IndexRune returns the index of the first instance of the Unicode code point
// r, or -1 if rune is not present in s.
// If r is [utf8.RuneError], it returns the first instance of any
// invalid UTF-8 byte sequence.
so_int strings_IndexRune(so_String s, so_rune r) {
    if (0 <= r && r < utf8_RuneSelf) {
        return strings_IndexByte(s, (so_byte)(r));
    } else if (r == utf8_RuneError) {
        for (so_int i = 0, _iw = 0; i < so_len(s); i += _iw) {
            _iw = 0;
            so_rune r = so_utf8_decode(s, i, &_iw);
            if (r == utf8_RuneError) {
                return i;
            }
        }
        return -1;
    } else if (!utf8_ValidRune(r)) {
        return -1;
    }
    // Search for rune r using the last byte of its UTF-8 encoded form.
    // The distribution of the last byte is more uniform compared to the
    // first byte which has a 78% chance of being [240, 243, 244].
    so_String rs = so_rune_string(r);
    so_int last = so_len(rs) - 1;
    so_int i = last;
    so_int fails = 0;
    bool fallback = false;
    for (; i < so_len(s);) {
        if (so_at(so_byte, s, i) != so_at(so_byte, rs, last)) {
            so_int o = strings_IndexByte(so_string_slice(s, i + 1, s.len), so_at(so_byte, rs, last));
            if (o < 0) {
                return -1;
            }
            i += o + 1;
        }
        // Step backwards comparing bytes.
        bool matched = true;
        for (so_int j = 1; j < so_len(rs); j++) {
            if (so_at(so_byte, s, i - j) != so_at(so_byte, rs, last - j)) {
                matched = false;
                break;
            }
        }
        if (matched) {
            return i - last;
        }
        fails++;
        i++;
        if (fails >= (4 + (i >> 4)) && i < so_len(s)) {
            fallback = true;
            break;
        }
    }
    if (!fallback) {
        return -1;
    }
    so_byte c0 = so_at(so_byte, rs, last);
    so_byte c1 = so_at(so_byte, rs, last - 1);
    for (; i < so_len(s); i++) {
        if (so_at(so_byte, s, i) == c0 && so_at(so_byte, s, i - 1) == c1) {
            bool found = true;
            for (so_int k = 2; k < so_len(rs); k++) {
                if (so_at(so_byte, s, i - k) != so_at(so_byte, rs, last - k)) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return i - last;
            }
        }
    }
    return -1;
}

// IndexAny returns the index of the first instance of any Unicode code point
// from chars in s, or -1 if no Unicode code point from chars is present in s.
so_int strings_IndexAny(so_String s, so_String chars) {
    if (so_string_eq(chars, so_str(""))) {
        // Avoid scanning all of s.
        return -1;
    }
    if (so_len(chars) == 1) {
        // Avoid scanning all of s.
        so_rune r = (so_rune)(so_at(so_byte, chars, 0));
        if (r >= utf8_RuneSelf) {
            r = utf8_RuneError;
        }
        return strings_IndexRune(s, r);
    }
    if (so_len(s) > 8) {
        {
            asciiSet as = makeASCIISet(chars);
            if (as.ok) {
                for (so_int i = 0; i < so_len(s); i++) {
                    if (asciiSet_contains(&as, so_at(so_byte, s, i))) {
                        return i;
                    }
                }
                return -1;
            }
        }
    }
    for (so_int i = 0, _iw = 0; i < so_len(s); i += _iw) {
        _iw = 0;
        so_rune c = so_utf8_decode(s, i, &_iw);
        if (strings_IndexRune(chars, c) >= 0) {
            return i;
        }
    }
    return -1;
}

// LastIndexByte returns the index of the last instance
// of c in s, or -1 if c is not present in s.
so_int strings_LastIndexByte(so_String s, so_byte c) {
    return bytealg_LastIndexByteString(s, c);
}

// IndexFunc returns the index into s of the first Unicode
// code point satisfying f(c), or -1 if none do.
so_int strings_IndexFunc(so_String s, strings_RunePredicate f) {
    return indexFunc(s, f, true);
}

// indexFunc is the same as IndexFunc except that if
// truth==false, the sense of the predicate function is
// inverted.
static so_int indexFunc(so_String s, strings_RunePredicate f, bool truth) {
    for (so_int i = 0, _iw = 0; i < so_len(s); i += _iw) {
        _iw = 0;
        so_rune r = so_utf8_decode(s, i, &_iw);
        if (f(r) == truth) {
            return i;
        }
    }
    return -1;
}

// lastIndexFunc is the same as LastIndexFunc except that if
// truth==false, the sense of the predicate function is
// inverted.
static so_int lastIndexFunc(so_String s, strings_RunePredicate f, bool truth) {
    for (so_int i = so_len(s); i > 0;) {
        so_R_rune_int _res1 = utf8_DecodeLastRuneInString(so_string_slice(s, 0, i));
        so_rune r = _res1.val;
        so_int size = _res1.val2;
        i -= size;
        if (f(r) == truth) {
            return i;
        }
    }
    return -1;
}

// -- map.go --

// ToUpper returns s with all Unicode letters mapped to their upper case.
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String strings_ToUpper(mem_Allocator a, so_String s) {
    bool isASCII = true, hasLower = false;
    for (so_int i = 0; i < so_len(s); i++) {
        so_byte c = so_at(so_byte, s, i);
        if (c >= utf8_RuneSelf) {
            isASCII = false;
            break;
        }
        hasLower = hasLower || ('a' <= c && c <= 'z');
    }
    if (isASCII) {
        // optimize for ASCII-only strings.
        if (!hasLower) {
            return stringslite_Clone(a, s);
        }
        strings_Builder b = (strings_Builder){.a = a};
        so_int pos = 0;
        strings_Builder_Grow(&b, so_len(s));
        for (so_int i = 0; i < so_len(s); i++) {
            so_byte c = so_at(so_byte, s, i);
            if ('a' <= c && c <= 'z') {
                c -= U'a' - U'A';
                if (pos < i) {
                    strings_Builder_WriteString(&b, so_string_slice(s, pos, i));
                }
                strings_Builder_WriteByte(&b, c);
                pos = i + 1;
            }
        }
        if (pos < so_len(s)) {
            strings_Builder_WriteString(&b, so_string_slice(s, pos, s.len));
        }
        return strings_Builder_String(&b);
    }
    return strings_Map(a, unicode_ToUpper, s);
}

// ToLower returns s with all Unicode letters mapped to their lower case.
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String strings_ToLower(mem_Allocator a, so_String s) {
    bool isASCII = true, hasUpper = false;
    for (so_int i = 0; i < so_len(s); i++) {
        so_byte c = so_at(so_byte, s, i);
        if (c >= utf8_RuneSelf) {
            isASCII = false;
            break;
        }
        hasUpper = hasUpper || ('A' <= c && c <= 'Z');
    }
    if (isASCII) {
        // optimize for ASCII-only strings.
        if (!hasUpper) {
            return stringslite_Clone(a, s);
        }
        strings_Builder b = (strings_Builder){.a = a};
        so_int pos = 0;
        strings_Builder_Grow(&b, so_len(s));
        for (so_int i = 0; i < so_len(s); i++) {
            so_byte c = so_at(so_byte, s, i);
            if ('A' <= c && c <= 'Z') {
                c += U'a' - U'A';
                if (pos < i) {
                    strings_Builder_WriteString(&b, so_string_slice(s, pos, i));
                }
                strings_Builder_WriteByte(&b, c);
                pos = i + 1;
            }
        }
        if (pos < so_len(s)) {
            strings_Builder_WriteString(&b, so_string_slice(s, pos, s.len));
        }
        return strings_Builder_String(&b);
    }
    return strings_Map(a, unicode_ToLower, s);
}

// Map returns a copy of the string s with all its characters modified
// according to the mapping function. If mapping returns a negative value, the character is
// dropped from the string with no replacement.
//
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String strings_Map(mem_Allocator a, strings_RuneFunc mapping, so_String s) {
    // The output buffer b is initialized on demand, the first
    // time a character differs.
    strings_Builder b = (strings_Builder){.a = a};
    for (so_int i = 0, _iw = 0; i < so_len(s); i += _iw) {
        _iw = 0;
        so_rune c = so_utf8_decode(s, i, &_iw);
        so_rune r = mapping(c);
        if (r == c && c != utf8_RuneError) {
            continue;
        }
        so_int width = 0;
        if (c == utf8_RuneError) {
            so_R_rune_int _res1 = utf8_DecodeRuneInString(so_string_slice(s, i, s.len));
            c = _res1.val;
            width = _res1.val2;
            if (width != 1 && r == c) {
                continue;
            }
        } else {
            width = utf8_RuneLen(c);
        }
        strings_Builder_Grow(&b, so_len(s) + utf8_UTFMax);
        strings_Builder_WriteString(&b, so_string_slice(s, 0, i));
        if (r >= 0) {
            strings_Builder_WriteRune(&b, r);
        }
        s = so_string_slice(s, i + width, s.len);
        break;
    }
    // Fast path for unchanged input
    if (strings_Builder_Cap(&b) == 0) {
        // didn't call b.Grow above
        return stringslite_Clone(a, s);
    }
    for (so_int _ = 0, __w = 0; _ < so_len(s); _ += __w) {
        __w = 0;
        so_rune c = so_utf8_decode(s, _, &__w);
        so_rune r = mapping(c);
        if (r >= 0) {
            // common case
            // Due to inlining, it is more performant to determine if WriteByte should be
            // invoked rather than always call WriteRune
            if (r < utf8_RuneSelf) {
                strings_Builder_WriteByte(&b, (so_byte)(r));
            } else {
                // r is not an ASCII rune.
                strings_Builder_WriteRune(&b, r);
            }
        }
    }
    return strings_Builder_String(&b);
}

// -- reader.go --

// Len returns the number of bytes of the unread portion of the
// string.
so_int strings_Reader_Len(void* self) {
    strings_Reader* r = self;
    if (r->i >= (int64_t)(so_len(r->s))) {
        return 0;
    }
    return (so_int)((int64_t)(so_len(r->s)) - r->i);
}

// Size returns the original length of the underlying string.
// Size is the number of bytes available for reading via [Reader.ReadAt].
// The returned value is always the same and is not affected by calls
// to any other method.
int64_t strings_Reader_Size(void* self) {
    strings_Reader* r = self;
    return (int64_t)(so_len(r->s));
}

// Read implements the [io.Reader] interface.
so_R_int_err strings_Reader_Read(void* self, so_Slice b) {
    strings_Reader* r = self;
    if (r->i >= (int64_t)(so_len(r->s))) {
        return (so_R_int_err){.val = 0, .err = io_EOF};
    }
    r->prevRune = -1;
    so_int n = so_copy_string(b, so_string_slice(r->s, r->i, r->s.len));
    r->i += (int64_t)(n);
    return (so_R_int_err){.val = n, .err = (so_Error){0}};
}

// ReadAt implements the [io.ReaderAt] interface.
so_R_int_err strings_Reader_ReadAt(void* self, so_Slice b, int64_t off) {
    strings_Reader* r = self;
    // cannot modify state - see io.ReaderAt
    if (off < 0) {
        return (so_R_int_err){.val = 0, .err = io_ErrOffset};
    }
    if (off >= (int64_t)(so_len(r->s))) {
        return (so_R_int_err){.val = 0, .err = io_EOF};
    }
    so_int n = so_copy_string(b, so_string_slice(r->s, off, r->s.len));
    if (n < so_len(b)) {
        return (so_R_int_err){.val = n, .err = io_EOF};
    }
    return (so_R_int_err){.val = n, .err = (so_Error){0}};
}

// ReadByte implements the [io.ByteReader] interface.
so_R_byte_err strings_Reader_ReadByte(void* self) {
    strings_Reader* r = self;
    r->prevRune = -1;
    if (r->i >= (int64_t)(so_len(r->s))) {
        return (so_R_byte_err){.val = 0, .err = io_EOF};
    }
    so_byte b = so_at(so_byte, r->s, r->i);
    r->i++;
    return (so_R_byte_err){.val = b, .err = (so_Error){0}};
}

// UnreadByte implements the [io.ByteScanner] interface.
so_Error strings_Reader_UnreadByte(void* self) {
    strings_Reader* r = self;
    if (r->i <= 0) {
        return io_ErrUnread;
    }
    r->prevRune = -1;
    r->i--;
    return (so_Error){0};
}

// ReadRune implements the [io.RuneReader] interface.
io_RuneSizeResult strings_Reader_ReadRune(void* self) {
    strings_Reader* r = self;
    if (r->i >= (int64_t)(so_len(r->s))) {
        r->prevRune = -1;
        return (io_RuneSizeResult){.Rune = 0, .Size = 0, .Err = io_EOF};
    }
    r->prevRune = (so_int)(r->i);
    {
        so_byte c = so_at(so_byte, r->s, r->i);
        if (c < utf8_RuneSelf) {
            r->i++;
            return (io_RuneSizeResult){.Rune = (so_rune)(c), .Size = 1, .Err = (so_Error){0}};
        }
    }
    so_R_rune_int _res1 = utf8_DecodeRuneInString(so_string_slice(r->s, r->i, r->s.len));
    so_rune ch = _res1.val;
    so_int size = _res1.val2;
    r->i += (int64_t)(size);
    return (io_RuneSizeResult){.Rune = ch, .Size = size, .Err = (so_Error){0}};
}

// UnreadRune implements the [io.RuneScanner] interface.
so_Error strings_Reader_UnreadRune(void* self) {
    strings_Reader* r = self;
    if (r->i <= 0) {
        return io_ErrUnread;
    }
    if (r->prevRune < 0) {
        return io_ErrUnread;
    }
    r->i = (int64_t)(r->prevRune);
    r->prevRune = -1;
    return (so_Error){0};
}

// Seek implements the [io.Seeker] interface.
so_R_i64_err strings_Reader_Seek(void* self, int64_t offset, so_int whence) {
    strings_Reader* r = self;
    r->prevRune = -1;
    int64_t abs = 0;
    if (whence == io_SeekStart) {
        abs = offset;
    } else if (whence == io_SeekCurrent) {
        abs = r->i + offset;
    } else if (whence == io_SeekEnd) {
        abs = (int64_t)(so_len(r->s)) + offset;
    } else {
        return (so_R_i64_err){.val = 0, .err = io_ErrWhence};
    }
    if (abs < 0) {
        return (so_R_i64_err){.val = 0, .err = io_ErrOffset};
    }
    r->i = abs;
    return (so_R_i64_err){.val = abs, .err = (so_Error){0}};
}

// WriteTo implements the [io.WriterTo] interface.
so_R_i64_err strings_Reader_WriteTo(void* self, io_Writer w) {
    strings_Reader* r = self;
    so_Error err = {0};
    r->prevRune = -1;
    if (r->i >= (int64_t)(so_len(r->s))) {
        return (so_R_i64_err){.val = 0, .err = (so_Error){0}};
    }
    so_String s = so_string_slice(r->s, r->i, r->s.len);
    so_R_int_err _res1 = io_WriteString(w, s);
    so_int m = _res1.val;
    err = _res1.err;
    if (m > so_len(s)) {
        return (so_R_i64_err){.val = 0, .err = io_ErrInvalidWrite};
    }
    r->i += (int64_t)(m);
    int64_t n = (int64_t)(m);
    if (m != so_len(s) && err.self == NULL) {
        err = io_ErrShortWrite;
    }
    return (so_R_i64_err){.val = n, .err = err};
}

// Reset resets the [Reader] to be reading from s.
void strings_Reader_Reset(void* self, so_String s) {
    strings_Reader* r = self;
    *r = (strings_Reader){.s = s, .prevRune = -1};
}

// NewReader returns a new [Reader] reading from s.
// It is similar to [bytes.NewBufferString] but more efficient and non-writable.
strings_Reader strings_NewReader(so_String s) {
    return (strings_Reader){.s = s, .prevRune = -1};
}

// -- repeat.go --

// Repeat returns a new string consisting of count copies of the string s.
//
// It panics if count is negative or if the result of (len(s) * count)
// overflows.
//
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String strings_Repeat(mem_Allocator a, so_String s, so_int count) {
    if (count == 0) {
        return so_str("");
    } else if (count == 1) {
        return stringslite_Clone(a, s);
    }
    // Since we cannot return an error on overflow,
    // we should panic if the repeat will generate an overflow.
    // See golang.org/issue/16237.
    if (count < 0) {
        so_panic("strings: negative repeat count");
    }
    so_R_uint_uint _res1 = bits_Mul((so_uint)(so_len(s)), (so_uint)(count));
    so_uint hi = _res1.val;
    so_uint lo = _res1.val2;
    if (hi > 0 || lo > (so_uint)(maxInt)) {
        so_panic("strings: repeat overflow");
    }
    // lo = len(s) * count
    so_int n = (so_int)(lo);
    if (so_len(s) == 0) {
        return so_str("");
    }
    // Optimize for commonly repeated strings of relatively short length.
    if (so_at(so_byte, s, 0) == ' ' || so_at(so_byte, s, 0) == '-' || so_at(so_byte, s, 0) == '0' || so_at(so_byte, s, 0) == '=' || so_at(so_byte, s, 0) == '\t') {
        if (n <= so_len(repeatedSpaces) && strings_HasPrefix(repeatedSpaces, s)) {
            return stringslite_Clone(a, so_string_slice(repeatedSpaces, 0, n));
        } else if (n <= so_len(repeatedDashes) && strings_HasPrefix(repeatedDashes, s)) {
            return stringslite_Clone(a, so_string_slice(repeatedDashes, 0, n));
        } else if (n <= so_len(repeatedZeroes) && strings_HasPrefix(repeatedZeroes, s)) {
            return stringslite_Clone(a, so_string_slice(repeatedZeroes, 0, n));
        } else if (n <= so_len(repeatedEquals) && strings_HasPrefix(repeatedEquals, s)) {
            return stringslite_Clone(a, so_string_slice(repeatedEquals, 0, n));
        } else if (n <= so_len(repeatedTabs) && strings_HasPrefix(repeatedTabs, s)) {
            return stringslite_Clone(a, so_string_slice(repeatedTabs, 0, n));
        }
    }
    // Past a certain chunk size it is counterproductive to use
    // larger chunks as the source of the write, as when the source
    // is too large we are basically just thrashing the CPU D-cache.
    // So if the result length is larger than an empirically-found
    // limit (8KB), we stop growing the source string once the limit
    // is reached and keep reusing the same source string - that
    // should therefore be always resident in the L1 cache - until we
    // have completed the construction of the result.
    // This yields significant speedups (up to +100%) in cases where
    // the result length is large (roughly, over L2 cache size).
    const int64_t chunkLimit = 8 * 1024;
    so_int chunkMax = n;
    if (n > chunkLimit) {
        chunkMax = chunkLimit / so_len(s) * so_len(s);
        if (chunkMax == 0) {
            chunkMax = so_len(s);
        }
    }
    strings_Builder b = (strings_Builder){.a = a};
    strings_Builder_Grow(&b, n);
    strings_Builder_WriteString(&b, s);
    for (; strings_Builder_Len(&b) < n;) {
        so_int chunk = n - strings_Builder_Len(&b);
        if (strings_Builder_Len(&b) < chunk) {
            chunk = strings_Builder_Len(&b);
        }
        if (chunkMax < chunk) {
            chunk = chunkMax;
        }
        strings_Builder_WriteString(&b, so_string_slice(strings_Builder_String(&b), 0, chunk));
    }
    return strings_Builder_String(&b);
}

// -- split.go --

// Split slices s into all substrings separated by sep and returns a slice of
// the substrings between those separators.
//
// If s does not contain sep and sep is not empty, Split returns a
// slice of length 1 whose only element is s.
//
// If sep is empty, Split splits after each UTF-8 sequence. If both s
// and sep are empty, Split returns an empty slice.
//
// It is equivalent to [SplitN] with a count of -1.
//
// To split around the first instance of a separator, see [Cut].
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
// The substrings in the slice are references to the original string s.
so_Slice strings_Split(mem_Allocator a, so_String s, so_String sep) {
    return genSplit(a, s, sep, 0, -1);
}

// SplitN slices s into substrings separated by sep and returns a slice of
// the substrings between those separators.
//
// The count determines the number of substrings to return:
//   - n > 0: at most n substrings; the last substring will be the unsplit remainder;
//   - n == 0: the result is nil (zero substrings);
//   - n < 0: all substrings.
//
// Edge cases for s and sep (for example, empty strings) are handled
// as described in the documentation for [Split].
//
// To split around the first instance of a separator, see [Cut].
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
// The substrings in the slice are references to the original string s.
so_Slice strings_SplitN(mem_Allocator a, so_String s, so_String sep, so_int n) {
    return genSplit(a, s, sep, 0, n);
}

// SplitAfter slices s into all substrings after each instance of sep and
// returns a slice of those substrings.
//
// If s does not contain sep and sep is not empty, SplitAfter returns
// a slice of length 1 whose only element is s.
//
// If sep is empty, SplitAfter splits after each UTF-8 sequence. If
// both s and sep are empty, SplitAfter returns an empty slice.
//
// It is equivalent to [SplitAfterN] with a count of -1.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
// The substrings in the slice are references to the original string s.
so_Slice strings_SplitAfter(mem_Allocator a, so_String s, so_String sep) {
    return genSplit(a, s, sep, so_len(sep), -1);
}

// Fields splits the string s around each instance of one or more consecutive white space
// characters, as defined by [unicode.IsSpace], returning a slice of substrings of s or an
// empty slice if s contains only white space. Every element of the returned slice is
// non-empty. Unlike [Split], leading and trailing runs of white space characters
// are discarded.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
// The substrings in the slice are references to the original string s.
so_Slice strings_Fields(mem_Allocator a, so_String s) {
    // First count the fields.
    // This is an exact count if s is ASCII, otherwise it is an approximation.
    so_int n = 0;
    so_int wasSpace = 1;
    // setBits is used to track which bits are set in the bytes of s.
    uint8_t setBits = (uint8_t)(0);
    for (so_int i = 0; i < so_len(s); i++) {
        so_byte r = so_at(so_byte, s, i);
        setBits |= r;
        so_int isSpace = (so_int)(asciiSpace[r]);
        n += (wasSpace & ~isSpace);
        wasSpace = isSpace;
    }
    if (setBits >= utf8_RuneSelf) {
        // Some runes in the input string are not ASCII.
        return strings_FieldsFunc(a, s, unicode_IsSpace);
    }
    // ASCII fast path
    so_Slice res = mem_AllocSlice(so_String, (a), (n), (n));
    so_int na = 0;
    so_int fieldStart = 0;
    so_int i = 0;
    // Skip spaces in the front of the input.
    for (; i < so_len(s) && asciiSpace[so_at(so_byte, s, i)] != 0;) {
        i++;
    }
    fieldStart = i;
    for (; i < so_len(s);) {
        if (asciiSpace[so_at(so_byte, s, i)] == 0) {
            i++;
            continue;
        }
        so_at(so_String, res, na) = so_string_slice(s, fieldStart, i);
        na++;
        i++;
        // Skip spaces in between fields.
        for (; i < so_len(s) && asciiSpace[so_at(so_byte, s, i)] != 0;) {
            i++;
        }
        fieldStart = i;
    }
    if (fieldStart < so_len(s)) {
        // Last field might end at EOF.
        so_at(so_String, res, na) = so_string_slice(s, fieldStart, s.len);
    }
    return res;
}

// FieldsFunc splits the string s at each run of Unicode code points c satisfying f(c)
// and returns an array of slices of s. If all code points in s satisfy f(c) or the
// string is empty, an empty slice is returned. Every element of the returned slice is
// non-empty. Unlike [Split], leading and trailing runs of code points satisfying f(c)
// are discarded.
//
// FieldsFunc makes no guarantees about the order in which it calls f(c)
// and assumes that f always returns the same value for a given c.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
// The substrings in the slice are references to the original string s.
so_Slice strings_FieldsFunc(mem_Allocator a, so_String s, strings_RunePredicate f) {
    so_Slice spans = so_make_slice(span, 0, 32);
    // Find the field start and end indices.
    // Doing this in a separate pass (rather than slicing the string s
    // and collecting the result substrings right away) is significantly
    // more efficient, possibly due to cache effects.
    // valid span start if >= 0
    so_int start = -1;
    for (so_int end = 0, _endw = 0; end < so_len(s); end += _endw) {
        _endw = 0;
        so_rune rune = so_utf8_decode(s, end, &_endw);
        if (f(rune)) {
            if (start >= 0) {
                spans = so_append(span, spans, ((span){start, end}));
                // Set start to a negative value.
                // Note: using -1 here consistently and reproducibly
                // slows down this code by a several percent on amd64.
                start = ~start;
            }
        } else {
            if (start < 0) {
                start = end;
            }
        }
    }
    // Last field might end at EOF.
    if (start >= 0) {
        spans = so_append(span, spans, ((span){start, so_len(s)}));
    }
    // Create strings from recorded field indices.
    so_Slice res = mem_AllocSlice(so_String, (a), (so_len(spans)), (so_len(spans)));
    for (so_int i = 0; i < so_len(spans); i++) {
        span sp = so_at(span, spans, i);
        so_at(so_String, res, i) = so_string_slice(s, sp.start, sp.end);
    }
    return res;
}

// Generic split: splits after each instance of sep,
// including sepSave bytes of sep in the subarrays.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
// The substrings in the slice are references to the original string s.
static so_Slice genSplit(mem_Allocator a, so_String s, so_String sep, so_int sepSave, so_int n) {
    if (n == 0) {
        return (so_Slice){0};
    }
    if (so_string_eq(sep, so_str(""))) {
        return explode(a, s, n);
    }
    if (n < 0) {
        n = strings_Count(s, sep) + 1;
    }
    if (n > so_len(s) + 1) {
        n = so_len(s) + 1;
    }
    so_Slice res = mem_AllocSlice(so_String, (a), (n), (n));
    n--;
    so_int i = 0;
    for (; i < n;) {
        so_int m = strings_Index(s, sep);
        if (m < 0) {
            break;
        }
        so_at(so_String, res, i) = so_string_slice(s, 0, m + sepSave);
        s = so_string_slice(s, m + so_len(sep), s.len);
        i++;
    }
    so_at(so_String, res, i) = s;
    return so_slice(so_String, res, 0, i + 1);
}

// explode splits s into a slice of UTF-8 strings,
// one string per Unicode character up to a maximum of n (n < 0 means no limit).
// Invalid UTF-8 bytes are sliced individually.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
// The substrings in the slice are references to the original string s.
static so_Slice explode(mem_Allocator a, so_String s, so_int n) {
    so_int l = utf8_RuneCountInString(s);
    if (n < 0 || n > l) {
        n = l;
    }
    so_Slice res = mem_AllocSlice(so_String, (a), (n), (n));
    for (so_int i = 0; i < n - 1; i++) {
        so_R_rune_int _res1 = utf8_DecodeRuneInString(s);
        so_int size = _res1.val2;
        so_at(so_String, res, i) = so_string_slice(s, 0, size);
        s = so_string_slice(s, size, s.len);
    }
    if (n > 0) {
        so_at(so_String, res, n - 1) = s;
    }
    return res;
}

// -- strings.go --

// Clone returns a fresh copy of s.
//
// It guarantees to make a copy of s into a new allocation,
// which can be important when retaining only a small substring
// of a much larger string. Using Clone can help such programs
// use less memory. Of course, since using Clone makes a copy,
// overuse of Clone can make programs use more memory.
//
// Clone should typically be used only rarely, and only when
// profiling indicates that it is needed.
//
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String strings_Clone(mem_Allocator a, so_String s) {
    return stringslite_Clone(a, s);
}

// Compare returns an integer comparing two strings lexicographically.
// The result will be 0 if a == b, -1 if a < b, and +1 if a > b.
//
// Use Compare when you need to perform a three-way comparison (with
// [slices.SortFunc], for example). It is usually clearer and always faster
// to use the built-in string comparison operators ==, <, >, and so on.
so_int strings_Compare(so_String a, so_String b) {
    return bytealg_Compare(so_string_bytes(a), so_string_bytes(b));
}

// Count counts the number of non-overlapping instances of substr in s.
// If substr is an empty string, Count returns 1 + the number of Unicode code points in s.
so_int strings_Count(so_String s, so_String substr) {
    // special case
    if (so_len(substr) == 0) {
        return utf8_RuneCountInString(s) + 1;
    }
    if (so_len(substr) == 1) {
        return bytealg_CountString(s, so_at(so_byte, substr, 0));
    }
    so_int n = 0;
    for (;;) {
        so_int i = strings_Index(s, substr);
        if (i == -1) {
            return n;
        }
        n++;
        s = so_string_slice(s, i + so_len(substr), s.len);
    }
}

// Cut slices s around the first instance of sep,
// returning the text before and after sep.
// If sep does not appear in s, cut returns s, "".
so_R_str_str strings_Cut(so_String s, so_String sep) {
    return stringslite_Cut(s, sep);
}

// CutPrefix returns s without the provided leading prefix string
// and reports whether it found the prefix.
// If s doesn't start with prefix, CutPrefix returns s, false.
// If prefix is the empty string, CutPrefix returns s, true.
so_R_str_bool strings_CutPrefix(so_String s, so_String prefix) {
    return stringslite_CutPrefix(s, prefix);
}

// CutSuffix returns s without the provided ending suffix string
// and reports whether it found the suffix.
// If s doesn't end with suffix, CutSuffix returns s, false.
// If suffix is the empty string, CutSuffix returns s, true.
so_R_str_bool strings_CutSuffix(so_String s, so_String suffix) {
    return stringslite_CutSuffix(s, suffix);
}

// Join concatenates the elements of its first argument to create a single string.
// The separator string sep is placed between elements in the resulting string.
// Panics if the result is too large to fit in a string.
//
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String strings_Join(mem_Allocator a, so_Slice elems, so_String sep) {
    if (so_len(elems) == 0) {
        return so_str("");
    } else if (so_len(elems) == 1) {
        return stringslite_Clone(a, so_at(so_String, elems, 0));
    }
    so_int n = 0;
    if (so_len(sep) > 0) {
        if (so_len(sep) >= maxInt / (so_len(elems) - 1)) {
            so_panic("strings: join separator too large");
        }
        n += so_len(sep) * (so_len(elems) - 1);
    }
    for (so_int _ = 0; _ < so_len(elems); _++) {
        so_String elem = so_at(so_String, elems, _);
        if (so_len(elem) > maxInt - n) {
            so_panic("strings: join overflow");
        }
        n += so_len(elem);
    }
    strings_Builder b = (strings_Builder){.a = a};
    strings_Builder_Grow(&b, n);
    strings_Builder_WriteString(&b, so_at(so_String, elems, 0));
    for (so_int _ = 0; _ < so_len(so_slice(so_String, elems, 1, elems.len)); _++) {
        so_String s = so_at(so_String, so_slice(so_String, elems, 1, elems.len), _);
        strings_Builder_WriteString(&b, sep);
        strings_Builder_WriteString(&b, s);
    }
    return strings_Builder_String(&b);
}

// HasPrefix reports whether the string s begins with prefix.
bool strings_HasPrefix(so_String s, so_String prefix) {
    return stringslite_HasPrefix(s, prefix);
}

// HasSuffix reports whether the string s ends with suffix.
bool strings_HasSuffix(so_String s, so_String suffix) {
    return stringslite_HasSuffix(s, suffix);
}

// Replace returns a copy of the string s with the first n
// non-overlapping instances of old replaced by new.
//
// If old is empty, it matches at the beginning of the string
// and after each UTF-8 sequence, yielding up to k+1 replacements
// for a k-rune string.
//
// If n < 0, there is no limit on the number of replacements.
//
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String strings_Replace(mem_Allocator a, so_String s, so_String old, so_String new, so_int n) {
    if (so_string_eq(old, new) || n == 0) {
        return stringslite_Clone(a, s);
    }
    // Compute number of replacements.
    {
        so_int m = strings_Count(s, old);
        if (m == 0) {
            return stringslite_Clone(a, s);
        } else if (n < 0 || m < n) {
            n = m;
        }
    }
    // Apply replacements to buffer.
    strings_Builder b = (strings_Builder){.a = a};
    strings_Builder_Grow(&b, so_len(s) + n * (so_len(new) - so_len(old)));
    so_int start = 0;
    if (so_len(old) > 0) {
        for (so_int _i = 0; _i < n; _i++) {
            so_int j = start + strings_Index(so_string_slice(s, start, s.len), old);
            strings_Builder_WriteString(&b, so_string_slice(s, start, j));
            strings_Builder_WriteString(&b, new);
            start = j + so_len(old);
        }
    } else {
        // len(old) == 0
        strings_Builder_WriteString(&b, new);
        for (so_int _i = 0; _i < n - 1; _i++) {
            so_R_rune_int _res1 = utf8_DecodeRuneInString(so_string_slice(s, start, s.len));
            so_int wid = _res1.val2;
            so_int j = start + wid;
            strings_Builder_WriteString(&b, so_string_slice(s, start, j));
            strings_Builder_WriteString(&b, new);
            start = j;
        }
    }
    strings_Builder_WriteString(&b, so_string_slice(s, start, s.len));
    return strings_Builder_String(&b);
}

// ReplaceAll returns a copy of the string s with all
// non-overlapping instances of old replaced by new.
// If old is empty, it matches at the beginning of the string
// and after each UTF-8 sequence, yielding up to k+1 replacements
// for a k-rune string.
//
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String strings_ReplaceAll(mem_Allocator a, so_String s, so_String old, so_String new) {
    return strings_Replace(a, s, old, new, -1);
}

// -- trim.go --

// Trim returns a slice of the string s with all leading and
// trailing Unicode code points contained in cutset removed.
so_String strings_Trim(so_String s, so_String cutset) {
    if (so_string_eq(s, so_str("")) || so_string_eq(cutset, so_str(""))) {
        return s;
    }
    if (so_len(cutset) == 1 && so_at(so_byte, cutset, 0) < utf8_RuneSelf) {
        return trimLeftByte(trimRightByte(s, so_at(so_byte, cutset, 0)), so_at(so_byte, cutset, 0));
    }
    {
        asciiSet as = makeASCIISet(cutset);
        if (as.ok) {
            return trimLeftASCII(trimRightASCII(s, &as), &as);
        }
    }
    return trimLeftUnicode(trimRightUnicode(s, cutset), cutset);
}

// TrimLeft returns a slice of the string s with all leading
// Unicode code points contained in cutset removed.
//
// To remove a prefix, use [TrimPrefix] instead.
so_String strings_TrimLeft(so_String s, so_String cutset) {
    if (so_string_eq(s, so_str("")) || so_string_eq(cutset, so_str(""))) {
        return s;
    }
    if (so_len(cutset) == 1 && so_at(so_byte, cutset, 0) < utf8_RuneSelf) {
        return trimLeftByte(s, so_at(so_byte, cutset, 0));
    }
    {
        asciiSet as = makeASCIISet(cutset);
        if (as.ok) {
            return trimLeftASCII(s, &as);
        }
    }
    return trimLeftUnicode(s, cutset);
}

// TrimRight returns a slice of the string s, with all trailing
// Unicode code points contained in cutset removed.
//
// To remove a suffix, use [TrimSuffix] instead.
so_String strings_TrimRight(so_String s, so_String cutset) {
    if (so_string_eq(s, so_str("")) || so_string_eq(cutset, so_str(""))) {
        return s;
    }
    if (so_len(cutset) == 1 && so_at(so_byte, cutset, 0) < utf8_RuneSelf) {
        return trimRightByte(s, so_at(so_byte, cutset, 0));
    }
    {
        asciiSet as = makeASCIISet(cutset);
        if (as.ok) {
            return trimRightASCII(s, &as);
        }
    }
    return trimRightUnicode(s, cutset);
}

// TrimSpace returns a slice (substring) of the string s,
// with all leading and trailing white space removed,
// as defined by Unicode.
so_String strings_TrimSpace(so_String s) {
    // Fast path for ASCII: look for the first ASCII non-space byte.
    for (so_int lo = 0; lo < so_len(s); lo++) {
        so_byte c = so_at(so_byte, s, lo);
        if (c >= utf8_RuneSelf) {
            // If we run into a non-ASCII byte, fall back to the
            // slower unicode-aware method on the remaining bytes.
            return strings_TrimFunc(so_string_slice(s, lo, s.len), unicode_IsSpace);
        }
        if (asciiSpace[c] != 0) {
            continue;
        }
        s = so_string_slice(s, lo, s.len);
        // Now look for the first ASCII non-space byte from the end.
        for (so_int hi = so_len(s) - 1; hi >= 0; hi--) {
            so_byte c = so_at(so_byte, s, hi);
            if (c >= utf8_RuneSelf) {
                return trimRightFunc(so_string_slice(s, 0, hi + 1), unicode_IsSpace);
            }
            if (asciiSpace[c] == 0) {
                // At this point, s[:hi+1] starts and ends with ASCII
                // non-space bytes, so we're done. Non-ASCII cases have
                // already been handled above.
                return so_string_slice(s, 0, hi + 1);
            }
        }
    }
    return so_str("");
}

// TrimPrefix returns s without the provided leading prefix string.
// If s doesn't start with prefix, s is returned unchanged.
so_String strings_TrimPrefix(so_String s, so_String prefix) {
    return stringslite_TrimPrefix(s, prefix);
}

// TrimSuffix returns s without the provided trailing suffix string.
// If s doesn't end with suffix, s is returned unchanged.
so_String strings_TrimSuffix(so_String s, so_String suffix) {
    return stringslite_TrimSuffix(s, suffix);
}

// TrimFunc returns a slice of the string s with all leading
// and trailing Unicode code points c satisfying f(c) removed.
so_String strings_TrimFunc(so_String s, strings_RunePredicate f) {
    return trimRightFunc(trimLeftFunc(s, f), f);
}

// trimLeftFunc returns a slice of the string s with all leading
// Unicode code points c satisfying f(c) removed.
static so_String trimLeftFunc(so_String s, strings_RunePredicate f) {
    so_int i = indexFunc(s, f, false);
    if (i == -1) {
        return so_str("");
    }
    return so_string_slice(s, i, s.len);
}

// trimRightFunc returns a slice of the string s with all trailing
// Unicode code points c satisfying f(c) removed.
static so_String trimRightFunc(so_String s, strings_RunePredicate f) {
    so_int i = lastIndexFunc(s, f, false);
    if (i >= 0) {
        so_R_rune_int _res1 = utf8_DecodeRuneInString(so_string_slice(s, i, s.len));
        so_int wid = _res1.val2;
        i += wid;
    } else {
        i++;
    }
    return so_string_slice(s, 0, i);
}

static so_String trimLeftByte(so_String s, so_byte c) {
    for (; so_len(s) > 0 && so_at(so_byte, s, 0) == c;) {
        s = so_string_slice(s, 1, s.len);
    }
    return s;
}

static so_String trimLeftASCII(so_String s, asciiSet* as) {
    for (; so_len(s) > 0;) {
        if (!asciiSet_contains(as, so_at(so_byte, s, 0))) {
            break;
        }
        s = so_string_slice(s, 1, s.len);
    }
    return s;
}

static so_String trimLeftUnicode(so_String s, so_String cutset) {
    for (; so_len(s) > 0;) {
        so_R_rune_int _res1 = utf8_DecodeRuneInString(s);
        so_rune r = _res1.val;
        so_int n = _res1.val2;
        if (!strings_ContainsRune(cutset, r)) {
            break;
        }
        s = so_string_slice(s, n, s.len);
    }
    return s;
}

static so_String trimRightByte(so_String s, so_byte c) {
    for (; so_len(s) > 0 && so_at(so_byte, s, so_len(s) - 1) == c;) {
        s = so_string_slice(s, 0, so_len(s) - 1);
    }
    return s;
}

static so_String trimRightASCII(so_String s, asciiSet* as) {
    for (; so_len(s) > 0;) {
        if (!asciiSet_contains(as, so_at(so_byte, s, so_len(s) - 1))) {
            break;
        }
        s = so_string_slice(s, 0, so_len(s) - 1);
    }
    return s;
}

static so_String trimRightUnicode(so_String s, so_String cutset) {
    for (; so_len(s) > 0;) {
        so_rune r = (so_rune)(so_at(so_byte, s, so_len(s) - 1));
        so_int n = 1;
        if (r >= utf8_RuneSelf) {
            so_R_rune_int _res1 = utf8_DecodeLastRuneInString(s);
            r = _res1.val;
            n = _res1.val2;
        }
        if (!strings_ContainsRune(cutset, r)) {
            break;
        }
        s = so_string_slice(s, 0, so_len(s) - n);
    }
    return s;
}
