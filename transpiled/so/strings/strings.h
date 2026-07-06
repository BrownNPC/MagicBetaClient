#pragma once
#include "so/builtin/builtin.h"
#include "so/bytealg/bytealg.h"
#include "so/io/io.h"
#include "so/math/bits/bits.h"
#include "so/mem/mem.h"
#include "so/stringslite/stringslite.h"
#include "so/unicode/unicode.h"
#include "so/unicode/utf8/utf8.h"

// -- Types --

typedef struct strings_Builder strings_Builder;
typedef struct strings_Reader strings_Reader;

// A Builder is used to efficiently build a string using [Builder.Write] methods.
// It minimizes memory copying. The zero value is ready to use (with default allocator).
// Do not copy a non-zero Builder.
//
// The caller is responsible for freeing the builder's resources
// with [Builder.Free] when done using it.
typedef struct strings_Builder {
    mem_Allocator a;
    so_Slice buf;
} strings_Builder;

// RunePredicate reports whether the rune satisfies a condition.
typedef bool (*strings_RunePredicate)(so_rune);

// RuneFunc maps a rune to another rune. If mapping returns
// a negative value, the rune is dropped from the result.
typedef so_rune (*strings_RuneFunc)(so_rune);

// A Reader implements the [io.Reader], [io.ReaderAt], [io.ByteReader], [io.ByteScanner],
// [io.RuneReader], [io.RuneScanner], [io.Seeker], and [io.WriterTo] interfaces by reading
// from a string.
// The zero value for Reader operates like a Reader of an empty string.
typedef struct strings_Reader {
    so_String s;
    int64_t i;
    so_int prevRune;
} strings_Reader;

// -- Functions and methods --

// NewBuilder returns a new Builder that uses the provided allocator.
strings_Builder strings_NewBuilder(mem_Allocator a);

// FixedBuilder returns a new Builder that uses the provided buffer
// as its internal buffer. The builder doesn't take ownership of the
// buffer and doesn't free it (Free is a no-op). The builder doesn't
// allocate additional memory, so writes that exceed the buffer's
// capacity will panic.
strings_Builder strings_FixedBuilder(so_Slice buf);

// String returns the accumulated string.
so_String strings_Builder_String(void* self);

// Len returns the number of accumulated bytes; b.Len() == len(b.String()).
so_int strings_Builder_Len(void* self);

// Cap returns the capacity of the builder's underlying byte slice. It is the
// total space allocated for the string being built and includes any bytes
// already written.
so_int strings_Builder_Cap(void* self);

// Reset resets the builder to be empty without freeing the underlying buffer.
void strings_Builder_Reset(void* self);

// Free frees the internal buffer and resets the builder.
// After Free, the builder can be reused with new writes.
void strings_Builder_Free(void* self);

// grow copies the buffer to a new, larger buffer so that there are at least n
// bytes of capacity beyond len(b.buf).
//
static inline void strings_Builder_grow(void* self, so_int n) {
    strings_Builder* b = self;
    if (so_cap(b->buf) - so_len(b->buf) >= n) {
        return;
    }
    so_int newCap = 2 * so_cap(b->buf) + n;
    b->buf = mem_ReallocSlice(so_byte, (b->a), (b->buf), (so_len(b->buf)), (newCap));
}

// Grow grows b's capacity, if necessary, to guarantee space for
// another n bytes. After Grow(n), at least n bytes can be written to b
// without another allocation. If n is negative, Grow panics.
void strings_Builder_Grow(void* self, so_int n);

// Write appends the contents of p to b's buffer.
// Write always returns len(p), nil.
//
static inline so_R_int_err strings_Builder_Write(void* self, so_Slice p) {
    strings_Builder* b = self;
    strings_Builder_grow(b, so_len(p));
    b->buf = so_extend(so_byte, b->buf, (p));
    return (so_R_int_err){.val = so_len(p), .err = (so_Error){0}};
}

// WriteByte appends the byte c to b's buffer.
// The returned error is always nil.
//
static inline so_Error strings_Builder_WriteByte(void* self, so_byte c) {
    strings_Builder* b = self;
    strings_Builder_grow(b, 1);
    b->buf = so_append(so_byte, b->buf, c);
    return (so_Error){0};
}

// WriteRune appends the UTF-8 encoding of Unicode code point r to b's buffer.
// It returns the length of r and a nil error.
so_R_int_err strings_Builder_WriteRune(void* self, so_rune r);

// WriteString appends the contents of s to b's buffer.
// It returns the length of s and a nil error.
//
static inline so_R_int_err strings_Builder_WriteString(void* self, so_String s) {
    strings_Builder* b = self;
    strings_Builder_grow(b, so_len(s));
    b->buf = so_extend(so_byte, b->buf, so_string_bytes(s));
    return (so_R_int_err){.val = so_len(s), .err = (so_Error){0}};
}

// Contains reports whether substr is within s.
bool strings_Contains(so_String s, so_String substr);

// ContainsAny reports whether any Unicode code points in chars are within s.
bool strings_ContainsAny(so_String s, so_String chars);

// ContainsRune reports whether the Unicode code point r is within s.
bool strings_ContainsRune(so_String s, so_rune r);

// ContainsFunc reports whether any Unicode code points r within s satisfy f(r).
bool strings_ContainsFunc(so_String s, strings_RunePredicate f);

// Index returns the index of the first instance of substr in s, or -1 if substr is not present in s.
so_int strings_Index(so_String s, so_String substr);

// LastIndex returns the index of the last instance of substr in s, or -1 if substr is not present in s.
so_int strings_LastIndex(so_String s, so_String substr);

// IndexByte returns the index of the first instance of c in s, or -1 if c is not present in s.
so_int strings_IndexByte(so_String s, so_byte c);

// IndexRune returns the index of the first instance of the Unicode code point
// r, or -1 if rune is not present in s.
// If r is [utf8.RuneError], it returns the first instance of any
// invalid UTF-8 byte sequence.
so_int strings_IndexRune(so_String s, so_rune r);

// IndexAny returns the index of the first instance of any Unicode code point
// from chars in s, or -1 if no Unicode code point from chars is present in s.
so_int strings_IndexAny(so_String s, so_String chars);

// LastIndexByte returns the index of the last instance
// of c in s, or -1 if c is not present in s.
so_int strings_LastIndexByte(so_String s, so_byte c);

// IndexFunc returns the index into s of the first Unicode
// code point satisfying f(c), or -1 if none do.
so_int strings_IndexFunc(so_String s, strings_RunePredicate f);

// ToUpper returns s with all Unicode letters mapped to their upper case.
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String strings_ToUpper(mem_Allocator a, so_String s);

// ToLower returns s with all Unicode letters mapped to their lower case.
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String strings_ToLower(mem_Allocator a, so_String s);

// Map returns a copy of the string s with all its characters modified
// according to the mapping function. If mapping returns a negative value, the character is
// dropped from the string with no replacement.
//
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String strings_Map(mem_Allocator a, strings_RuneFunc mapping, so_String s);

// Len returns the number of bytes of the unread portion of the
// string.
so_int strings_Reader_Len(void* self);

// Size returns the original length of the underlying string.
// Size is the number of bytes available for reading via [Reader.ReadAt].
// The returned value is always the same and is not affected by calls
// to any other method.
int64_t strings_Reader_Size(void* self);

// Read implements the [io.Reader] interface.
so_R_int_err strings_Reader_Read(void* self, so_Slice b);

// ReadAt implements the [io.ReaderAt] interface.
so_R_int_err strings_Reader_ReadAt(void* self, so_Slice b, int64_t off);

// ReadByte implements the [io.ByteReader] interface.
so_R_byte_err strings_Reader_ReadByte(void* self);

// UnreadByte implements the [io.ByteScanner] interface.
so_Error strings_Reader_UnreadByte(void* self);

// ReadRune implements the [io.RuneReader] interface.
io_RuneSizeResult strings_Reader_ReadRune(void* self);

// UnreadRune implements the [io.RuneScanner] interface.
so_Error strings_Reader_UnreadRune(void* self);

// Seek implements the [io.Seeker] interface.
so_R_i64_err strings_Reader_Seek(void* self, int64_t offset, so_int whence);

// WriteTo implements the [io.WriterTo] interface.
so_R_i64_err strings_Reader_WriteTo(void* self, io_Writer w);

// Reset resets the [Reader] to be reading from s.
void strings_Reader_Reset(void* self, so_String s);

// NewReader returns a new [Reader] reading from s.
// It is similar to [bytes.NewBufferString] but more efficient and non-writable.
strings_Reader strings_NewReader(so_String s);

// Repeat returns a new string consisting of count copies of the string s.
//
// It panics if count is negative or if the result of (len(s) * count)
// overflows.
//
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String strings_Repeat(mem_Allocator a, so_String s, so_int count);

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
so_Slice strings_Split(mem_Allocator a, so_String s, so_String sep);

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
so_Slice strings_SplitN(mem_Allocator a, so_String s, so_String sep, so_int n);

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
so_Slice strings_SplitAfter(mem_Allocator a, so_String s, so_String sep);

// Fields splits the string s around each instance of one or more consecutive white space
// characters, as defined by [unicode.IsSpace], returning a slice of substrings of s or an
// empty slice if s contains only white space. Every element of the returned slice is
// non-empty. Unlike [Split], leading and trailing runs of white space characters
// are discarded.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
// The substrings in the slice are references to the original string s.
so_Slice strings_Fields(mem_Allocator a, so_String s);

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
so_Slice strings_FieldsFunc(mem_Allocator a, so_String s, strings_RunePredicate f);

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
so_String strings_Clone(mem_Allocator a, so_String s);

// Compare returns an integer comparing two strings lexicographically.
// The result will be 0 if a == b, -1 if a < b, and +1 if a > b.
//
// Use Compare when you need to perform a three-way comparison (with
// [slices.SortFunc], for example). It is usually clearer and always faster
// to use the built-in string comparison operators ==, <, >, and so on.
so_int strings_Compare(so_String a, so_String b);

// Count counts the number of non-overlapping instances of substr in s.
// If substr is an empty string, Count returns 1 + the number of Unicode code points in s.
so_int strings_Count(so_String s, so_String substr);

// Cut slices s around the first instance of sep,
// returning the text before and after sep.
// If sep does not appear in s, cut returns s, "".
so_R_str_str strings_Cut(so_String s, so_String sep);

// CutPrefix returns s without the provided leading prefix string
// and reports whether it found the prefix.
// If s doesn't start with prefix, CutPrefix returns s, false.
// If prefix is the empty string, CutPrefix returns s, true.
so_R_str_bool strings_CutPrefix(so_String s, so_String prefix);

// CutSuffix returns s without the provided ending suffix string
// and reports whether it found the suffix.
// If s doesn't end with suffix, CutSuffix returns s, false.
// If suffix is the empty string, CutSuffix returns s, true.
so_R_str_bool strings_CutSuffix(so_String s, so_String suffix);

// Join concatenates the elements of its first argument to create a single string.
// The separator string sep is placed between elements in the resulting string.
// Panics if the result is too large to fit in a string.
//
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String strings_Join(mem_Allocator a, so_Slice elems, so_String sep);

// HasPrefix reports whether the string s begins with prefix.
bool strings_HasPrefix(so_String s, so_String prefix);

// HasSuffix reports whether the string s ends with suffix.
bool strings_HasSuffix(so_String s, so_String suffix);

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
so_String strings_Replace(mem_Allocator a, so_String s, so_String old, so_String new, so_int n);

// ReplaceAll returns a copy of the string s with all
// non-overlapping instances of old replaced by new.
// If old is empty, it matches at the beginning of the string
// and after each UTF-8 sequence, yielding up to k+1 replacements
// for a k-rune string.
//
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String strings_ReplaceAll(mem_Allocator a, so_String s, so_String old, so_String new);

// Trim returns a slice of the string s with all leading and
// trailing Unicode code points contained in cutset removed.
so_String strings_Trim(so_String s, so_String cutset);

// TrimLeft returns a slice of the string s with all leading
// Unicode code points contained in cutset removed.
//
// To remove a prefix, use [TrimPrefix] instead.
so_String strings_TrimLeft(so_String s, so_String cutset);

// TrimRight returns a slice of the string s, with all trailing
// Unicode code points contained in cutset removed.
//
// To remove a suffix, use [TrimSuffix] instead.
so_String strings_TrimRight(so_String s, so_String cutset);

// TrimSpace returns a slice (substring) of the string s,
// with all leading and trailing white space removed,
// as defined by Unicode.
so_String strings_TrimSpace(so_String s);

// TrimPrefix returns s without the provided leading prefix string.
// If s doesn't start with prefix, s is returned unchanged.
so_String strings_TrimPrefix(so_String s, so_String prefix);

// TrimSuffix returns s without the provided trailing suffix string.
// If s doesn't end with suffix, s is returned unchanged.
so_String strings_TrimSuffix(so_String s, so_String suffix);

// TrimFunc returns a slice of the string s with all leading
// and trailing Unicode code points c satisfying f(c) removed.
so_String strings_TrimFunc(so_String s, strings_RunePredicate f);
