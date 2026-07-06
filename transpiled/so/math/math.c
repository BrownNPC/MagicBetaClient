#include "math.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>

// -- Variables and constants --
static const int64_t uvnan = 0x7FF8000000000001;
static const int64_t uvinf = 0x7FF0000000000000;
static const int64_t uvneginf = 0xFFF0000000000000;
static const int64_t uvone = 0x3FF0000000000000;
static const int64_t mask = 0x7FF;
static const int64_t shift = 64 - 11 - 1;
static const int64_t bias = 1023;
static const int64_t signMask = ((int64_t)1 << 63);
static const int64_t fracMask = ((int64_t)1 << shift) - 1;

// pow10tab stores the pre-computed values 10**i for i < 32.
static double pow10tab[32] = {1e00, 1e01, 1e02, 1e03, 1e04, 1e05, 1e06, 1e07, 1e08, 1e09, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22, 1e23, 1e24, 1e25, 1e26, 1e27, 1e28, 1e29, 1e30, 1e31};

// pow10postab32 stores the pre-computed value for 10**(i*32) at index i.
static double pow10postab32[10] = {1e00, 1e32, 1e64, 1e96, 1e128, 1e160, 1e192, 1e224, 1e256, 1e288};

// pow10negtab32 stores the pre-computed value for 10**(-i*32) at index i.
static double pow10negtab32[11] = {1e-00, 1e-32, 1e-64, 1e-96, 1e-128, 1e-160, 1e-192, 1e-224, 1e-256, 1e-288, 1e-320};

// -- basic.go --

// Abs returns the absolute value of x.
//
// Special cases are:
//
//	Abs(±Inf) = +Inf
//	Abs(NaN) = NaN
double math_Abs(double x) {
    return fabs(x);
}

// FMA returns x * y + z, computed with only one rounding.
// (That is, FMA returns the fused multiply-add of x, y, and z.)
double math_FMA(double x, double y, double z) {
    return fma(x, y, z);
}

// Mod returns the floating-point remainder of x/y.
// The magnitude of the result is less than y and its
// sign agrees with that of x.
//
// Special cases are:
//
//	Mod(±Inf, y) = NaN
//	Mod(NaN, y) = NaN
//	Mod(x, 0) = NaN
//	Mod(x, ±Inf) = x
//	Mod(x, NaN) = NaN
double math_Mod(double x, double y) {
    return fmod(x, y);
}

// Remainder returns the IEEE 754 floating-point remainder of x/y.
//
// Special cases are:
//
//	Remainder(±Inf, y) = NaN
//	Remainder(NaN, y) = NaN
//	Remainder(x, 0) = NaN
//	Remainder(x, ±Inf) = x
//	Remainder(x, NaN) = NaN
double math_Remainder(double x, double y) {
    return remainder(x, y);
}

// -- bits.go --

// Inf returns positive infinity if sign >= 0, negative infinity if sign < 0.
double math_Inf(so_int sign) {
    uint64_t v = 0;
    if (sign >= 0) {
        v = uvinf;
    } else {
        v = uvneginf;
    }
    return math_Float64frombits(v);
}

// NaN returns an IEEE 754 “not-a-number” value.
double math_NaN(void) {
    return math_Float64frombits(uvnan);
}

// IsNaN reports whether f is an IEEE 754 “not-a-number” value.
bool math_IsNaN(double f) {
    // IEEE 754 says that only NaNs satisfy f != f.
    // To avoid the floating-point hardware, could use:
    //	x := Float64bits(f);
    //	return uint32(x>>shift)&mask == mask && x != uvinf && x != uvneginf
    return f != f;
}

// IsInf reports whether f is an infinity, according to sign.
// If sign > 0, IsInf reports whether f is positive infinity.
// If sign < 0, IsInf reports whether f is negative infinity.
// If sign == 0, IsInf reports whether f is either infinity.
bool math_IsInf(double f, so_int sign) {
    // Test for infinity by comparing against maximum float.
    // To avoid the floating-point hardware, could use:
    //	x := Float64bits(f);
    //	return sign >= 0 && x == uvinf || sign <= 0 && x == uvneginf;
    if (sign == 0) {
        f = math_Abs(f);
    } else if (sign < 0) {
        f = -f;
    }
    return f > DBL_MAX;
}

// -- classify.go --

// Signbit reports whether x is negative or negative zero.
bool math_Signbit(double x) {
    return signbit(x);
}

// -- const.go --

// -- dim.go --

// Dim returns the maximum of x-y or 0.
//
// Special cases are:
//
//	Dim(+Inf, +Inf) = NaN
//	Dim(-Inf, -Inf) = NaN
//	Dim(x, NaN) = Dim(NaN, x) = NaN
double math_Dim(double x, double y) {
    // The special cases result in NaN after the subtraction:
    //      +Inf - +Inf = NaN
    //      -Inf - -Inf = NaN
    //       NaN - y    = NaN
    //         x - NaN  = NaN
    double v = x - y;
    if (v <= 0) {
        // v is negative or 0
        return 0;
    }
    // v is positive or NaN
    return v;
}

// Max returns the larger of x or y.
//
// Special cases are:
//
//	Max(x, +Inf) = Max(+Inf, x) = +Inf
//	Max(x, NaN) = Max(NaN, x) = NaN
//	Max(+0, ±0) = Max(±0, +0) = +0
//	Max(-0, -0) = -0
//
// Note that this differs from the built-in function max when called
// with NaN and +Inf.
double math_Max(double x, double y) {
    return fmax(x, y);
}

// Min returns the smaller of x or y.
//
// Special cases are:
//
//	Min(x, -Inf) = Min(-Inf, x) = -Inf
//	Min(x, NaN) = Min(NaN, x) = NaN
//	Min(-0, ±0) = Min(±0, -0) = -0
//
// Note that this differs from the built-in function min when called
// with NaN and -Inf.
double math_Min(double x, double y) {
    return fmin(x, y);
}

// -- erf.go --

// Erf returns the error function of x.
//
// Special cases are:
//
//	Erf(+Inf) = 1
//	Erf(-Inf) = -1
//	Erf(NaN) = NaN
double math_Erf(double x) {
    return erf(x);
}

// Erfc returns the complementary error function of x.
//
// Special cases are:
//
//	Erfc(+Inf) = 0
//	Erfc(-Inf) = 2
//	Erfc(NaN) = NaN
double math_Erfc(double x) {
    return erfc(x);
}

// Gamma returns the Gamma function of x.
//
// Special cases are:
//
//	Gamma(+Inf) = +Inf
//	Gamma(+0) = +Inf
//	Gamma(-0) = -Inf
//	Gamma(x) = NaN for integer x < 0
//	Gamma(-Inf) = NaN
//	Gamma(NaN) = NaN
double math_Gamma(double x) {
    return tgamma(x);
}

// Lgamma returns the natural logarithm of [Gamma](x).
//
// Special cases are:
//
//	Lgamma(+Inf) = +Inf
//	Lgamma(0) = +Inf
//	Lgamma(-integer) = +Inf
//	Lgamma(-Inf) = -Inf
//	Lgamma(NaN) = NaN
double math_Lgamma(double x) {
    return lgamma(x);
}

// -- exp.go --

// Exp returns e**x, the base-e exponential of x.
//
// Special cases are:
//
//	Exp(+Inf) = +Inf
//	Exp(NaN) = NaN
//
// Very large values overflow to 0 or +Inf.
// Very small values underflow to 1.
double math_Exp(double x) {
    return exp(x);
}

// Exp2 returns 2**x, the base-2 exponential of x.
//
// Special cases are the same as [Exp].
double math_Exp2(double x) {
    return exp2(x);
}

// Expm1 returns e**x - 1, the base-e exponential of x minus 1.
// It is more accurate than [Exp](x) - 1 when x is near zero.
//
// Special cases are:
//
//	Expm1(+Inf) = +Inf
//	Expm1(-Inf) = -1
//	Expm1(NaN) = NaN
//
// Very large values overflow to -1 or +Inf.
double math_Expm1(double x) {
    return expm1(x);
}

// Log returns the natural logarithm of x.
//
// Special cases are:
//
//	Log(+Inf) = +Inf
//	Log(0) = -Inf
//	Log(x < 0) = NaN
//	Log(NaN) = NaN
double math_Log(double x) {
    return log(x);
}

// Log1p returns the natural logarithm of 1 plus its argument x.
// It is more accurate than [Log](1 + x) when x is near zero.
//
// Special cases are:
//
//	Log1p(+Inf) = +Inf
//	Log1p(±0) = ±0
//	Log1p(-1) = -Inf
//	Log1p(x < -1) = NaN
//	Log1p(NaN) = NaN
double math_Log1p(double x) {
    return log1p(x);
}

// Log10 returns the decimal logarithm of x.
// The special cases are the same as for [Log].
double math_Log10(double x) {
    return log10(x);
}

// Log2 returns the binary logarithm of x.
// The special cases are the same as for [Log].
double math_Log2(double x) {
    return log2(x);
}

// Logb returns the binary exponent of x.
//
// Special cases are:
//
//	Logb(±Inf) = +Inf
//	Logb(0) = -Inf
//	Logb(NaN) = NaN
double math_Logb(double x) {
    return logb(x);
}

// Ilogb returns the binary exponent of x as an integer.
//
// Special cases are:
//
//	Ilogb(±Inf) = MaxInt32
//	Ilogb(0) = MinInt32
//	Ilogb(NaN) = MaxInt32
so_int math_Ilogb(double x) {
    return (so_int)(ilogb(x));
}

// -- extern.go --

// -- float.go --

// Copysign returns a value with the magnitude of f
// and the sign of sign.
double math_Copysign(double f, double sign) {
    // const signBit = 1 << 63
    // return Float64frombits(Float64bits(f)&^signBit | Float64bits(sign)&signBit)
    return copysign(f, sign);
}

// Frexp breaks f into a normalized fraction
// and an integral power of two.
// It returns frac and exp satisfying f == frac × 2**exp,
// with the absolute value of frac in the interval [½, 1).
//
// Special cases are:
//
//	Frexp(±0) = ±0, 0
//	Frexp(±Inf) = ±Inf, 0
//	Frexp(NaN) = NaN, 0
so_R_f64_int math_Frexp(double f) {
    int exp = 0;
    double frac = frexp(f, &exp);
    return (so_R_f64_int){.val = frac, .val2 = (so_int)(exp)};
}

// Ldexp is the inverse of [Frexp].
// It returns frac × 2**exp.
//
// Special cases are:
//
//	Ldexp(±0, exp) = ±0
//	Ldexp(±Inf, exp) = ±Inf
//	Ldexp(NaN, exp) = NaN
double math_Ldexp(double frac, so_int exp) {
    return ldexp(frac, (int)(exp));
}

// Modf returns integer and fractional floating-point numbers
// that sum to f. Both values have the same sign as f.
//
// Special cases are:
//
//	Modf(±Inf) = ±Inf, NaN
//	Modf(NaN) = NaN, NaN
so_R_f64_f64 math_Modf(double f) {
    double intp = 0;
    double frac = modf(f, &intp);
    return (so_R_f64_f64){.val = intp, .val2 = frac};
}

// Nextafter32 returns the next representable float32 value after x towards y.
//
// Special cases are:
//
//	Nextafter32(x, x)   = x
//	Nextafter32(NaN, y) = NaN
//	Nextafter32(x, NaN) = NaN
float math_Nextafter32(float x, float y) {
    return nextafterf(x, y);
}

// Nextafter returns the next representable float64 value after x towards y.
//
// Special cases are:
//
//	Nextafter(x, x)   = x
//	Nextafter(NaN, y) = NaN
//	Nextafter(x, NaN) = NaN
double math_Nextafter(double x, double y) {
    return nextafter(x, y);
}

// -- hyper.go --

// Asinh returns the inverse hyperbolic sine of x.
//
// Special cases are:
//
//	Asinh(±0) = ±0
//	Asinh(±Inf) = ±Inf
//	Asinh(NaN) = NaN
double math_Asinh(double x) {
    return asinh(x);
}

// Acosh returns the inverse hyperbolic cosine of x.
//
// Special cases are:
//
//	Acosh(+Inf) = +Inf
//	Acosh(x) = NaN if x < 1
//	Acosh(NaN) = NaN
double math_Acosh(double x) {
    return acosh(x);
}

// Atanh returns the inverse hyperbolic tangent of x.
//
// Special cases are:
//
//	Atanh(1) = +Inf
//	Atanh(±0) = ±0
//	Atanh(-1) = -Inf
//	Atanh(x) = NaN if x < -1 or x > 1
//	Atanh(NaN) = NaN
double math_Atanh(double x) {
    return atanh(x);
}

// Sinh returns the hyperbolic sine of x.
//
// Special cases are:
//
//	Sinh(±0) = ±0
//	Sinh(±Inf) = ±Inf
//	Sinh(NaN) = NaN
double math_Sinh(double x) {
    return sinh(x);
}

// Cosh returns the hyperbolic cosine of x.
//
// Special cases are:
//
//	Cosh(±0) = 1
//	Cosh(±Inf) = +Inf
//	Cosh(NaN) = NaN
double math_Cosh(double x) {
    return cosh(x);
}

// Tanh returns the hyperbolic tangent of x.
//
// Special cases are:
//
//	Tanh(±0) = ±0
//	Tanh(±Inf) = ±1
//	Tanh(NaN) = NaN
double math_Tanh(double x) {
    return tanh(x);
}

// -- nearest.go --

// Floor returns the greatest integer value less than or equal to x.
//
// Special cases are:
//
//	Floor(±0) = ±0
//	Floor(±Inf) = ±Inf
//	Floor(NaN) = NaN
double math_Floor(double x) {
    return floor(x);
}

// Ceil returns the least integer value greater than or equal to x.
//
// Special cases are:
//
//	Ceil(±0) = ±0
//	Ceil(±Inf) = ±Inf
//	Ceil(NaN) = NaN
double math_Ceil(double x) {
    return ceil(x);
}

// Trunc returns the integer value of x.
//
// Special cases are:
//
//	Trunc(±0) = ±0
//	Trunc(±Inf) = ±Inf
//	Trunc(NaN) = NaN
double math_Trunc(double x) {
    return trunc(x);
}

// Round returns the nearest integer, rounding half away from zero.
//
// Special cases are:
//
//	Round(±0) = ±0
//	Round(±Inf) = ±Inf
//	Round(NaN) = NaN
double math_Round(double x) {
    return round(x);
}

// RoundToEven returns the nearest integer, rounding ties to even.
//
// Special cases are:
//
//	RoundToEven(±0) = ±0
//	RoundToEven(±Inf) = ±Inf
//	RoundToEven(NaN) = NaN
double math_RoundToEven(double x) {
    // RoundToEven is a faster implementation of:
    //
    // func RoundToEven(x float64) float64 {
    //   t := math.Trunc(x)
    //   odd := math.Remainder(t, 2) != 0
    //   if d := math.Abs(x - t); d > 0.5 || (d == 0.5 && odd) {
    //     return t + math.Copysign(1, x)
    //   }
    //   return t
    // }
    uint64_t bits = math_Float64bits(x);
    so_uint e = ((so_uint)(bits >> shift) & mask);
    if (e >= bias) {
        // Round abs(x) >= 1.
        // - Large numbers without fractional components, infinity, and NaN are unchanged.
        // - Add 0.499.. or 0.5 before truncating depending on whether the truncated
        //   number is even or odd (respectively).
        const int64_t halfMinusULP = ((int64_t)1 << (shift - 1)) - 1;
        e -= bias;
        bits += ((halfMinusULP + ((bits >> (shift - e)) & 1)) >> e);
        bits &= ~(fracMask >> e);
    } else if (e == bias - 1 && ((bits & fracMask) != 0)) {
        // Round 0.5 < abs(x) < 1.
        // +-1
        bits = ((bits & signMask) | uvone);
    } else {
        // Round abs(x) <= 0.5 including denormals.
        // +-0
        bits &= signMask;
    }
    return math_Float64frombits(bits);
}

// -- power.go --

// Cbrt returns the cube root of x.
//
// Special cases are:
//
//	Cbrt(±0) = ±0
//	Cbrt(±Inf) = ±Inf
//	Cbrt(NaN) = NaN
double math_Cbrt(double x) {
    return cbrt(x);
}

// Hypot returns [Sqrt](p*p + q*q), taking care to avoid
// unnecessary overflow and underflow.
//
// Special cases are:
//
//	Hypot(±Inf, q) = +Inf
//	Hypot(p, ±Inf) = +Inf
//	Hypot(NaN, q) = NaN
//	Hypot(p, NaN) = NaN
double math_Hypot(double p, double q) {
    return hypot(p, q);
}

// Pow returns x**y, the base-x exponential of y.
//
// Special cases are (in order):
//
//	Pow(x, ±0) = 1 for any x
//	Pow(1, y) = 1 for any y
//	Pow(x, 1) = x for any x
//	Pow(NaN, y) = NaN
//	Pow(x, NaN) = NaN
//	Pow(±0, y) = ±Inf for y an odd integer < 0
//	Pow(±0, -Inf) = +Inf
//	Pow(±0, +Inf) = +0
//	Pow(±0, y) = +Inf for finite y < 0 and not an odd integer
//	Pow(±0, y) = ±0 for y an odd integer > 0
//	Pow(±0, y) = +0 for finite y > 0 and not an odd integer
//	Pow(-1, ±Inf) = 1
//	Pow(x, +Inf) = +Inf for |x| > 1
//	Pow(x, -Inf) = +0 for |x| > 1
//	Pow(x, +Inf) = +0 for |x| < 1
//	Pow(x, -Inf) = +Inf for |x| < 1
//	Pow(+Inf, y) = +Inf for y > 0
//	Pow(+Inf, y) = +0 for y < 0
//	Pow(-Inf, y) = Pow(-0, -y)
//	Pow(x, y) = NaN for finite x < 0 and finite non-integer y
double math_Pow(double x, double y) {
    return pow(x, y);
}

// Pow10 returns 10**n, the base-10 exponential of n.
//
// Special cases are:
//
//	Pow10(n) =    0 for n < -323
//	Pow10(n) = +Inf for n > 308
double math_Pow10(so_int n) {
    if (0 <= n && n <= 308) {
        return pow10postab32[(so_uint)(n) / 32] * pow10tab[(so_uint)(n) % 32];
    }
    if (-323 <= n && n <= 0) {
        return pow10negtab32[(so_uint)(-n) / 32] / pow10tab[(so_uint)(-n) % 32];
    }
    // n < -323 || 308 < n
    if (n > 0) {
        return math_Inf(1);
    }
    // n < -323
    return 0;
}

// Sqrt returns the square root of x.
//
// Special cases are:
//
//	Sqrt(+Inf) = +Inf
//	Sqrt(±0) = ±0
//	Sqrt(x < 0) = NaN
//	Sqrt(NaN) = NaN
double math_Sqrt(double x) {
    return sqrt(x);
}

// -- trig.go --

// Asin returns the arcsine, in radians, of x.
//
// Special cases are:
//
//	Asin(±0) = ±0
//	Asin(x) = NaN if x < -1 or x > 1
double math_Asin(double x) {
    return asin(x);
}

// Acos returns the arccosine, in radians, of x.
//
// Special case is:
//
//	Acos(x) = NaN if x < -1 or x > 1
double math_Acos(double x) {
    return acos(x);
}

// Atan returns the arctangent, in radians, of x.
//
// Special cases are:
//
//	Atan(±0) = ±0
//	Atan(±Inf) = ±Pi/2
double math_Atan(double x) {
    return atan(x);
}

// Atan2 returns the arc tangent of y/x, using
// the signs of the two to determine the quadrant
// of the return value.
//
// Special cases are (in order):
//
//	Atan2(y, NaN) = NaN
//	Atan2(NaN, x) = NaN
//	Atan2(+0, x>=0) = +0
//	Atan2(-0, x>=0) = -0
//	Atan2(+0, x<=-0) = +Pi
//	Atan2(-0, x<=-0) = -Pi
//	Atan2(y>0, 0) = +Pi/2
//	Atan2(y<0, 0) = -Pi/2
//	Atan2(+Inf, +Inf) = +Pi/4
//	Atan2(-Inf, +Inf) = -Pi/4
//	Atan2(+Inf, -Inf) = 3Pi/4
//	Atan2(-Inf, -Inf) = -3Pi/4
//	Atan2(y, +Inf) = 0
//	Atan2(y>0, -Inf) = +Pi
//	Atan2(y<0, -Inf) = -Pi
//	Atan2(+Inf, x) = +Pi/2
//	Atan2(-Inf, x) = -Pi/2
double math_Atan2(double y, double x) {
    return atan2(y, x);
}

// Cos returns the cosine of the radian argument x.
//
// Special cases are:
//
//	Cos(±Inf) = NaN
//	Cos(NaN) = NaN
double math_Cos(double x) {
    return cos(x);
}

// Sin returns the sine of the radian argument x.
//
// Special cases are:
//
//	Sin(±0) = ±0
//	Sin(±Inf) = NaN
//	Sin(NaN) = NaN
double math_Sin(double x) {
    return sin(x);
}

// Tan returns the tangent of the radian argument x.
//
// Special cases are:
//
//	Tan(±0) = ±0
//	Tan(±Inf) = NaN
//	Tan(NaN) = NaN
double math_Tan(double x) {
    return tan(x);
}

// -- unsafe.go --

// Float32bits returns the IEEE 754 binary representation of f,
// with the sign bit of f and the result in the same bit position.
// Float32bits(Float32frombits(x)) == x.
uint32_t math_Float32bits(float f) {
    return *(uint32_t*)((void*)(&f));
}

// Float32frombits returns the floating-point number corresponding
// to the IEEE 754 binary representation b, with the sign bit of b
// and the result in the same bit position.
// Float32frombits(Float32bits(x)) == x.
float math_Float32frombits(uint32_t b) {
    return *(float*)((void*)(&b));
}

// Float64bits returns the IEEE 754 binary representation of f,
// with the sign bit of f and the result in the same bit position,
// and Float64bits(Float64frombits(x)) == x.
uint64_t math_Float64bits(double f) {
    return *(uint64_t*)((void*)(&f));
}

// Float64frombits returns the floating-point number corresponding
// to the IEEE 754 binary representation b, with the sign bit of b
// and the result in the same bit position.
// Float64frombits(Float64bits(x)) == x.
double math_Float64frombits(uint64_t b) {
    return *(double*)((void*)(&b));
}
