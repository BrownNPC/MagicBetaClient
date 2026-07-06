#pragma once
#include "so/builtin/builtin.h"
#include "so/errors/errors.h"
#include "so/math/bits/bits.h"

// -- Variables and constants --

// Maximum length of a boolean string ("true" or "false").
static const int64_t strconv_MaxBoolLen = 5;
extern so_Error strconv_ErrRange;
extern so_Error strconv_ErrSyntax;
extern so_Error strconv_ErrBase;
extern so_Error strconv_ErrBitSize;
extern so_Error strconv_ErrUnknown;

// IntSize is the size in bits of an int or uint value.
static const int64_t strconv_IntSize = ((int64_t)32 << ((uint64_t)(~(so_uint)(0)) >> 63));

// Maximum length of a float string in 'e'/'E' format with default precision.
// For 'f' format, the length depends on the magnitude of the number.
static const int64_t strconv_MaxFloat32Len = 14;
static const int64_t strconv_MaxFloat64Len = 24;

// Maximum length of an integer string in various bases, for 64-bit integers.
static const int64_t strconv_MaxUintBase2Len = 64;
static const int64_t strconv_MaxUintBase8Len = 22;
static const int64_t strconv_MaxUintBase10Len = 20;
static const int64_t strconv_MaxUintBase16Len = 16;
static const int64_t strconv_MaxIntBase2Len = strconv_MaxUintBase2Len + 1;
static const int64_t strconv_MaxIntBase8Len = strconv_MaxUintBase8Len + 1;
static const int64_t strconv_MaxIntBase10Len = strconv_MaxUintBase10Len + 1;
static const int64_t strconv_MaxIntBase16Len = strconv_MaxUintBase16Len + 1;

// -- Functions and methods --

// ParseBool returns the boolean value represented by the string.
// It accepts 1, t, T, TRUE, true, True, 0, f, F, FALSE, false, False.
// Any other value returns an error.
so_R_bool_err strconv_ParseBool(so_String str);

// FormatBool returns "true" or "false" according to the value of b.
so_String strconv_FormatBool(bool b);

// AppendBool appends "true" or "false", according to the value of b,
// to dst and returns the extended buffer.
so_Slice strconv_AppendBool(so_Slice dst, bool b);

// ParseFloat converts the string s to a floating-point number
// with the precision specified by bitSize: 32 for float32, or 64 for float64.
// When bitSize=32, the result still has type float64, but it will be
// convertible to float32 without changing its value.
//
// ParseFloat accepts decimal and hexadecimal floating-point numbers
// as defined by the Go syntax for [floating-point literals].
// If s is well-formed and near a valid floating-point number,
// ParseFloat returns the nearest floating-point number rounded
// using IEEE754 unbiased rounding.
// (Parsing a hexadecimal floating-point value only rounds when
// there are more bits in the hexadecimal representation than
// will fit in the mantissa.)
//
// The errors that ParseFloat returns have concrete type *NumError
// and include err.Num = s.
//
// If s is not syntactically well-formed, ParseFloat returns err.Err = ErrSyntax.
//
// If s is syntactically well-formed but is more than 1/2 ULP
// away from the largest floating point number of the given size,
// ParseFloat returns f = ±Inf, err.Err = ErrRange.
//
// ParseFloat recognizes the string "NaN", and the (possibly signed) strings "Inf" and "Infinity"
// as their respective special floating point values. It ignores case when matching.
//
// [floating-point literals]: https://go.dev/ref/spec#Floating-point_literals
so_R_f64_err strconv_ParseFloat(so_String s, so_int bitSize);

// ParseUint is like [ParseInt] but for unsigned numbers.
//
// A sign prefix is not permitted.
so_R_u64_err strconv_ParseUint(so_String s, so_int base, so_int bitSize);

// ParseInt interprets a string s in the given base (0, 2 to 36) and
// bit size (0 to 64) and returns the corresponding value i.
//
// The string may begin with a leading sign: "+" or "-".
//
// If the base argument is 0, the true base is implied by the string's
// prefix following the sign (if present): 2 for "0b", 8 for "0" or "0o",
// 16 for "0x", and 10 otherwise. Also, for argument base 0 only,
// underscore characters are permitted as defined by the Go syntax for
// [integer literals].
//
// The bitSize argument specifies the integer type
// that the result must fit into. Bit sizes 0, 8, 16, 32, and 64
// correspond to int, int8, int16, int32, and int64.
// If bitSize is below 0 or above 64, an error is returned.
//
// The errors that ParseInt returns have concrete type [*NumError]
// and include err.Num = s. If s is empty or contains invalid
// digits, err.Err = [ErrSyntax] and the returned value is 0;
// if the value corresponding to s cannot be represented by a
// signed integer of the given size, err.Err = [ErrRange] and the
// returned value is the maximum magnitude integer of the
// appropriate bitSize and sign.
//
// [integer literals]: https://go.dev/ref/spec#Integer_literals
so_R_i64_err strconv_ParseInt(so_String s, so_int base, so_int bitSize);

// Atoi is equivalent to ParseInt(s, 10, 0), converted to type int.
so_R_int_err strconv_Atoi(so_String s);

// FormatFloat converts the floating-point number f to a string,
// according to the format fmt and precision prec. It rounds the
// result assuming that the original was obtained from a floating-point
// value of bitSize bits (32 for float32, 64 for float64).
//
// The format fmt is one of
//   - 'b' (-ddddp±ddd, a binary exponent),
//   - 'e' (-d.dddde±dd, a decimal exponent),
//   - 'E' (-d.ddddE±dd, a decimal exponent),
//   - 'f' (-ddd.dddd, no exponent),
//   - 'g' ('e' for large exponents, 'f' otherwise),
//   - 'G' ('E' for large exponents, 'f' otherwise),
//   - 'x' (-0xd.ddddp±ddd, a hexadecimal fraction and binary exponent), or
//   - 'X' (-0Xd.ddddP±ddd, a hexadecimal fraction and binary exponent).
//
// The precision prec controls the number of digits (excluding the exponent)
// printed by the 'e', 'E', 'f', 'g', 'G', 'x', and 'X' formats.
// For 'e', 'E', 'f', 'x', and 'X', it is the number of digits after the decimal point.
// For 'g' and 'G' it is the maximum number of significant digits (trailing
// zeros are removed).
// The special precision -1 uses the smallest number of digits
// necessary such that ParseFloat will return f exactly.
// The exponent is written as a decimal integer;
// for all formats other than 'b', it will be at least two digits.
//
// dst length must be at least prec+4 bytes when prec >= 0,
// and at least [MaxFloat64Len] bytes when prec < 0.
so_String strconv_FormatFloat(so_Slice dst, double f, so_byte fmt, so_int prec, so_int bitSize);

// AppendFloat appends the string form of the floating-point number f,
// as generated by [FormatFloat], to dst and returns the extended buffer.
//
// dst free capacity must be at least prec+4 bytes when prec >= 0,
// and at least [MaxFloat64Len] bytes when prec < 0.
so_Slice strconv_AppendFloat(so_Slice dst, double f, so_byte fmt, so_int prec, so_int bitSize);

// FormatUint returns the string representation of i in the given base,
// for 2 <= base <= 36. The result uses the lower-case letters 'a' to 'z'
// for digit values >= 10.
// dst must have enough length to hold the result (see MaxUintBase*Len constants).
so_String strconv_FormatUint(so_Slice dst, uint64_t i, so_int base);

// FormatInt returns the string representation of i in the given base,
// for 2 <= base <= 36. The result uses the lower-case letters 'a' to 'z'
// for digit values >= 10.
// dst must have enough length to hold the result (see MaxIntBase*Len constants).
so_String strconv_FormatInt(so_Slice dst, int64_t i, so_int base);

// Itoa is equivalent to [FormatInt](int64(i), 10).
// dst length must be at least [MaxIntBase10Len] bytes.
so_String strconv_Itoa(so_Slice dst, so_int i);

// AppendInt appends the string form of the integer i,
// as generated by [FormatInt], to dst and returns the extended buffer.
// dst must have enough capacity to hold the result (see MaxIntBase*Len constants).
so_Slice strconv_AppendInt(so_Slice dst, int64_t i, so_int base);

// AppendUint appends the string form of the unsigned integer i,
// as generated by [FormatUint], to dst and returns the extended buffer.
// dst must have enough capacity to hold the result (see MaxUintBase*Len constants).
so_Slice strconv_AppendUint(so_Slice dst, uint64_t i, so_int base);
