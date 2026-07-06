#pragma once
#include "so/builtin/builtin.h"

// -- Types --

typedef struct unicode_RangeTable unicode_RangeTable;
typedef struct unicode_Range16 unicode_Range16;
typedef struct unicode_Range32 unicode_Range32;
typedef struct unicode_CaseRange unicode_CaseRange;

// to make the CaseRanges text shorter
typedef so_rune unicode_D[3];

// RangeTable defines a set of Unicode code points by listing the ranges of
// code points within the set. The ranges are listed in two slices
// to save space: a slice of 16-bit ranges and a slice of 32-bit ranges.
// The two slices must be in sorted order and non-overlapping.
// Also, R32 should contain only values >= 0x10000 (1<<16).
typedef struct unicode_RangeTable {
    so_Slice R16;
    so_Slice R32;
    so_int LatinOffset;
} unicode_RangeTable;

// Range16 represents of a range of 16-bit Unicode code points. The range runs from Lo to Hi
// inclusive and has the specified stride.
typedef struct unicode_Range16 {
    uint16_t Lo;
    uint16_t Hi;
    uint16_t Stride;
} unicode_Range16;

// Range32 represents of a range of Unicode code points and is used when one or
// more of the values will not fit in 16 bits. The range runs from Lo to Hi
// inclusive and has the specified stride. Lo and Hi must always be >= 1<<16.
typedef struct unicode_Range32 {
    uint32_t Lo;
    uint32_t Hi;
    uint32_t Stride;
} unicode_Range32;

// CaseRange represents a range of Unicode code points for simple (one
// code point to one code point) case conversion.
// The range runs from Lo to Hi inclusive, with a fixed stride of 1. Deltas
// are the number to add to the code point to reach the code point for a
// different case for that character. They may be negative. If zero, it
// means the character is in the corresponding case. There is a special
// case representing sequences of alternating corresponding Upper and Lower
// pairs. It appears with a fixed Delta of
//
//	{UpperLower, UpperLower, UpperLower}
//
// The constant UpperLower has an otherwise impossible delta value.
typedef struct unicode_CaseRange {
    uint32_t Lo;
    uint32_t Hi;
    unicode_D Delta;
} unicode_CaseRange;

// -- Variables and constants --

// Version is the Unicode edition from which the tables are derived.
static const so_String unicode_Version = so_str("15.0.0");
static const so_rune unicode_MaxRune = U'\U0010FFFF';
static const so_rune unicode_ReplacementChar = U'\uFFFD';
static const so_rune unicode_MaxASCII = U'\x7f';
static const so_rune unicode_MaxLatin1 = U'\u00FF';

// If the Delta field of a [CaseRange] is UpperLower, it means
// this CaseRange represents a sequence of the form (say)
// [Upper] [Lower] [Upper] [Lower].
static const so_rune unicode_UpperLower = unicode_MaxRune + 1;

// Latin is the set of Unicode characters in script Latin.
extern unicode_RangeTable* unicode_Latin;

// Letter is the set of Unicode letters, category L (Letter).
extern unicode_RangeTable* unicode_Letter;

// Lower is the set of Unicode characters in category Ll (Letter, lowercase).
extern unicode_RangeTable* unicode_Lower;

// Title is the set of Unicode characters in category Lt (Letter, titlecase).
extern unicode_RangeTable* unicode_Title;

// Upper is the set of Unicode characters in category Lu (Letter, uppercase).
extern unicode_RangeTable* unicode_Upper;

// Digit is the set of Unicode characters in category Nd (Number, decimal digit).
extern unicode_RangeTable* unicode_Digit;

// White_Space is the set of Unicode characters with property White_Space.
extern unicode_RangeTable* unicode_White_Space;

// CaseRanges is the table describing case mappings for all letters with
// non-self mappings.
extern so_Slice unicode_CaseRanges;

// Indices into the Delta arrays inside CaseRanges for case mapping.
static const int64_t unicode_UpperCase = 0;
static const int64_t unicode_LowerCase = 1;
static const int64_t unicode_TitleCase = 2;
static const int64_t unicode_MaxCase = 3;

// -- Functions and methods --

// In reports whether the rune is a member of one of the ranges.
bool unicode_In(so_rune r, so_Slice ranges);

// Is reports whether the rune is in the specified table of ranges.
bool unicode_Is(unicode_RangeTable* rangeTab, so_rune r);

// IsUpper reports whether the rune is an upper case letter.
bool unicode_IsUpper(so_rune r);

// IsLower reports whether the rune is a lower case letter.
bool unicode_IsLower(so_rune r);

// IsTitle reports whether the rune is a title case letter.
bool unicode_IsTitle(so_rune r);

// IsControl reports whether the rune is a control character.
bool unicode_IsControl(so_rune r);

// IsLetter reports whether the rune is a letter (category [L]).
bool unicode_IsLetter(so_rune r);

// IsDigit reports whether the rune is a decimal digit.
bool unicode_IsDigit(so_rune r);

// IsSpace reports whether the rune is a space character as defined
// by Unicode's White Space property; in the Latin-1 space
// this is
//
//	'\t', '\n', '\v', '\f', '\r', ' ', U+0085 (NEL), U+00A0 (NBSP).
//
// Other spacing characters are defined by [White_Space].
bool unicode_IsSpace(so_rune r);

// ToUpper maps the rune to upper case.
so_rune unicode_ToUpper(so_rune r);

// ToLower maps the rune to lower case.
so_rune unicode_ToLower(so_rune r);

// ToTitle maps the rune to title case.
so_rune unicode_ToTitle(so_rune r);

// To maps the rune to the specified case: [UpperCase], [LowerCase], or [TitleCase].
so_rune unicode_To(so_int _case, so_rune r);
