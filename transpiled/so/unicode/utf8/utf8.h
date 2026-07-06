#pragma once
#include "so/builtin/builtin.h"

// -- Variables and constants --

// The conditions RuneError==unicode.ReplacementChar and
// MaxRune==unicode.MaxRune are verified in the tests.
// Defining them locally avoids this package depending on package unicode.
// Numbers fundamental to the encoding.
static const so_rune utf8_RuneError = U'\uFFFD';
static const int64_t utf8_RuneSelf = 0x80;
static const so_rune utf8_MaxRune = U'\U0010FFFF';
static const int64_t utf8_UTFMax = 4;

// -- Functions and methods --

// FullRune reports whether the bytes in p begin with a full UTF-8 encoding of a rune.
// An invalid encoding is considered a full Rune since it will convert as a width-1 error rune.
bool utf8_FullRune(so_Slice p);

// FullRuneInString is like FullRune but its input is a string.
bool utf8_FullRuneInString(so_String s);

// DecodeRune unpacks the first UTF-8 encoding in p and returns the rune and
// its width in bytes. If p is empty it returns ([RuneError], 0). Otherwise, if
// the encoding is invalid, it returns (RuneError, 1). Both are impossible
// results for correct, non-empty UTF-8.
//
// An encoding is invalid if it is incorrect UTF-8, encodes a rune that is
// out of range, or is not the shortest possible UTF-8 encoding for the
// value. No other validation is performed.
so_R_rune_int utf8_DecodeRune(so_Slice p);

// DecodeRuneInString is like [DecodeRune] but its input is a string. If s is
// empty it returns ([RuneError], 0). Otherwise, if the encoding is invalid, it
// returns (RuneError, 1). Both are impossible results for correct, non-empty
// UTF-8.
//
// An encoding is invalid if it is incorrect UTF-8, encodes a rune that is
// out of range, or is not the shortest possible UTF-8 encoding for the
// value. No other validation is performed.
so_R_rune_int utf8_DecodeRuneInString(so_String s);

// DecodeLastRune unpacks the last UTF-8 encoding in p and returns the rune and
// its width in bytes. If p is empty it returns ([RuneError], 0). Otherwise, if
// the encoding is invalid, it returns (RuneError, 1). Both are impossible
// results for correct, non-empty UTF-8.
//
// An encoding is invalid if it is incorrect UTF-8, encodes a rune that is
// out of range, or is not the shortest possible UTF-8 encoding for the
// value. No other validation is performed.
so_R_rune_int utf8_DecodeLastRune(so_Slice p);

// DecodeLastRuneInString is like [DecodeLastRune] but its input is a string. If
// s is empty it returns ([RuneError], 0). Otherwise, if the encoding is invalid,
// it returns (RuneError, 1). Both are impossible results for correct,
// non-empty UTF-8.
//
// An encoding is invalid if it is incorrect UTF-8, encodes a rune that is
// out of range, or is not the shortest possible UTF-8 encoding for the
// value. No other validation is performed.
so_R_rune_int utf8_DecodeLastRuneInString(so_String s);

// RuneLen returns the number of bytes in the UTF-8 encoding of the rune.
// It returns -1 if the rune is not a valid value to encode in UTF-8.
so_int utf8_RuneLen(so_rune r);

// EncodeRune writes into p (which must be large enough) the UTF-8 encoding of the rune.
// If the rune is out of range, it writes the encoding of [RuneError].
// It returns the number of bytes written.
so_int utf8_EncodeRune(so_Slice p, so_rune r);

// AppendRune appends the UTF-8 encoding of r to the end of p and
// returns the extended buffer. If the rune is out of range,
// it appends the encoding of [RuneError].
//
// Requires spare p capacity: at least [UTFMax] bytes.
so_Slice utf8_AppendRune(so_Slice p, so_rune r);

// RuneCount returns the number of runes in p. Erroneous and short
// encodings are treated as single runes of width 1 byte.
so_int utf8_RuneCount(so_Slice p);

// RuneCountInString is like [RuneCount] but its input is a string.
so_int utf8_RuneCountInString(so_String s);

// RuneStart reports whether the byte could be the first byte of an encoded,
// possibly invalid rune. Second and subsequent bytes always have the top two
// bits set to 10.
bool utf8_RuneStart(so_byte b);

// Valid reports whether p consists entirely of valid UTF-8-encoded runes.
bool utf8_Valid(so_Slice p);

// ValidString reports whether s consists entirely of valid UTF-8-encoded runes.
bool utf8_ValidString(so_String s);

// ValidRune reports whether r can be legally encoded as UTF-8.
// Code points that are out of range or a surrogate half are illegal.
bool utf8_ValidRune(so_rune r);
