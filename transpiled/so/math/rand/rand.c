#include "rand.h"

// -- Forward declarations --
static so_R_u64_u64 rand_PCG_next(void* self);
static uint64_t rand_Rand_uint64n(void* self, uint64_t n);
static uint32_t rand_Rand_uint32n(void* self, uint32_t n);

// -- Variables and constants --

// https://numpy.org/devdocs/reference/random/upgrading-pcg64.html
// https://github.com/imneme/pcg-cpp/commit/871d0494ee9c9a7b7c43f753e3d8ca47c26f8005
so_Error rand_ErrUnmarshalPCG = errors_New("rand: invalid PCG encoding");
static const bool is32bit = ((uint64_t)(~(so_uint)(0)) >> 32) == 0;

/*
 * Top-level convenience functions
 */
// globalSource is the source of random numbers for the top-level functions.
//
static _Thread_local rand_PCG globalSource = {0};

// globalRand is the Rand that top-level functions use.
//
static _Thread_local rand_Rand globalRand = {0};

// -- pcg.go --

// NewPCG returns a new PCG seeded with the given values.
rand_PCG rand_NewPCG(uint64_t seed1, uint64_t seed2) {
    return (rand_PCG){seed1, seed2};
}

// Seed resets the PCG to behave the same way as NewPCG(seed1, seed2).
void rand_PCG_Seed(void* self, uint64_t seed1, uint64_t seed2) {
    rand_PCG* p = self;
    p->hi = seed1;
    p->lo = seed2;
}

// AppendBinary implements the [encoding.BinaryAppender] interface.
// Requires at least 20 bytes of spare capacity in b.
so_R_slice_err rand_PCG_AppendBinary(void* self, so_Slice b) {
    rand_PCG* p = self;
    b = so_extend(so_byte, b, so_string_bytes(so_str("pcg:")));
    b = binary_BE_AppendUint64(binary_BigEndian, b, p->hi);
    b = binary_BE_AppendUint64(binary_BigEndian, b, p->lo);
    return (so_R_slice_err){.val = b, .err = (so_Error){0}};
}

// MarshalBinary implements the [encoding.BinaryMarshaler] interface.
// Requires a 20-byte buffer in b.
so_R_slice_err rand_PCG_MarshalBinary(void* self, so_Slice b) {
    rand_PCG* p = self;
    return rand_PCG_AppendBinary(p, so_slice(so_byte, b, 0, 0));
}

// UnmarshalBinary implements the [encoding.BinaryUnmarshaler] interface.
so_Error rand_PCG_UnmarshalBinary(void* self, so_Slice data) {
    rand_PCG* p = self;
    if (so_len(data) != 20 || so_string_ne(so_bytes_string(so_slice(so_byte, data, 0, 4)), so_str("pcg:"))) {
        return rand_ErrUnmarshalPCG;
    }
    p->hi = binary_BE_Uint64(binary_BigEndian, so_slice(so_byte, data, 4, data.len));
    p->lo = binary_BE_Uint64(binary_BigEndian, so_slice(so_byte, data, 4 + 8, data.len));
    return (so_Error){0};
}

static so_R_u64_u64 rand_PCG_next(void* self) {
    rand_PCG* p = self;
    // https://github.com/imneme/pcg-cpp/blob/428802d1a5/include/pcg_random.hpp#L161
    //
    // Numpy's PCG multiplies by the 64-bit value cheapMul
    // instead of the 128-bit value used here and in the official PCG code.
    // This does not seem worthwhile, at least for Go: not having any high
    // bits in the multiplier reduces the effect of low bits on the highest bits,
    // and it only saves 1 multiply out of 3.
    // (On 32-bit systems, it saves 1 out of 6, since Mul64 is doing 4.)
    const int64_t mulHi = 2549297995355413924;
    const int64_t mulLo = 4865540595714422341;
    const int64_t incHi = 6364136223846793005;
    const int64_t incLo = 1442695040888963407;
    // state = state * mul + inc
    so_R_u64_u64 _res1 = bits_Mul64(p->lo, mulLo);
    uint64_t hi = _res1.val;
    uint64_t lo = _res1.val2;
    hi += p->hi * mulLo + p->lo * mulHi;
    so_R_u64_u64 _res2 = bits_Add64(lo, incLo, 0);
    lo = _res2.val;
    uint64_t c = _res2.val2;
    so_R_u64_u64 _res3 = bits_Add64(hi, incHi, c);
    hi = _res3.val;
    p->lo = lo;
    p->hi = hi;
    return (so_R_u64_u64){.val = hi, .val2 = lo};
}

// Uint64 return a uniformly-distributed random uint64 value.
uint64_t rand_PCG_Uint64(void* self) {
    rand_PCG* p = self;
    so_R_u64_u64 _res1 = rand_PCG_next(p);
    uint64_t hi = _res1.val;
    uint64_t lo = _res1.val2;
    // XSL-RR would be
    //	hi, lo := p.next()
    //	return bits.RotateLeft64(lo^hi, -int(hi>>58))
    // but Numpy uses DXSM and O'Neill suggests doing the same.
    // See https://github.com/golang/go/issues/21835#issuecomment-739065688
    // and following comments.
    // DXSM "double xorshift multiply"
    // https://github.com/imneme/pcg-cpp/blob/428802d1a5/include/pcg_random.hpp#L1015
    // https://github.com/imneme/pcg-cpp/blob/428802d1a5/include/pcg_random.hpp#L176
    const int64_t cheapMul = 0xda942042e4dd58b5;
    hi ^= (hi >> 32);
    hi *= cheapMul;
    hi ^= (hi >> 48);
    hi *= (lo | 1);
    return hi;
}

// -- rand.go --

// New returns a new Rand that uses random values from src
// to generate other random values.
rand_Rand rand_New(rand_Source src) {
    return (rand_Rand){.src = src};
}

// Int64 returns a non-negative pseudo-random 63-bit integer as an int64.
int64_t rand_Rand_Int64(void* self) {
    rand_Rand* r = self;
    return (int64_t)(r->src.Uint64(r->src.self) & ~((uint64_t)1 << 63));
}

// Uint32 returns a pseudo-random 32-bit value as a uint32.
uint32_t rand_Rand_Uint32(void* self) {
    rand_Rand* r = self;
    return (uint32_t)(r->src.Uint64(r->src.self) >> 32);
}

// Uint64 returns a pseudo-random 64-bit value as a uint64.
uint64_t rand_Rand_Uint64(void* self) {
    rand_Rand* r = self;
    return r->src.Uint64(r->src.self);
}

// Int32 returns a non-negative pseudo-random 31-bit integer as an int32.
int32_t rand_Rand_Int32(void* self) {
    rand_Rand* r = self;
    return (int32_t)(r->src.Uint64(r->src.self) >> 33);
}

// Int returns a non-negative pseudo-random int.
so_int rand_Rand_Int(void* self) {
    rand_Rand* r = self;
    return (so_int)(((so_uint)(r->src.Uint64(r->src.self)) << 1) >> 1);
}

// Uint returns a pseudo-random uint.
so_uint rand_Rand_Uint(void* self) {
    rand_Rand* r = self;
    return (so_uint)(r->src.Uint64(r->src.self));
}

// Int64N returns, as an int64, a non-negative pseudo-random number
// in the half-open interval [0,n). It panics if n <= 0.
int64_t rand_Rand_Int64N(void* self, int64_t n) {
    rand_Rand* r = self;
    if (n <= 0) {
        so_panic("invalid argument to Int64N");
    }
    return (int64_t)(rand_Rand_uint64n(r, (uint64_t)(n)));
}

// Uint64N returns, as a uint64, a non-negative pseudo-random number
// in the half-open interval [0,n). It panics if n == 0.
uint64_t rand_Rand_Uint64N(void* self, uint64_t n) {
    rand_Rand* r = self;
    if (n == 0) {
        so_panic("invalid argument to Uint64N");
    }
    return rand_Rand_uint64n(r, n);
}

// uint64n is the no-bounds-checks version of Uint64N.
static uint64_t rand_Rand_uint64n(void* self, uint64_t n) {
    rand_Rand* r = self;
    if (is32bit && (uint64_t)((uint32_t)(n)) == n) {
        return (uint64_t)(rand_Rand_uint32n(r, (uint32_t)(n)));
    }
    if ((n & (n - 1)) == 0) {
        // n is power of two, can mask
        return (rand_Rand_Uint64(r) & (n - 1));
    }
    // Suppose we have a uint64 x uniform in the range [0,2⁶⁴)
    // and want to reduce it to the range [0,n) preserving exact uniformity.
    // We can simulate a scaling arbitrary precision x * (n/2⁶⁴) by
    // the high bits of a double-width multiply of x*n, meaning (x*n)/2⁶⁴.
    // Since there are 2⁶⁴ possible inputs x and only n possible outputs,
    // the output is necessarily biased if n does not divide 2⁶⁴.
    // In general (x*n)/2⁶⁴ = k for x*n in [k*2⁶⁴,(k+1)*2⁶⁴).
    // There are either floor(2⁶⁴/n) or ceil(2⁶⁴/n) possible products
    // in that range, depending on k.
    // But suppose we reject the sample and try again when
    // x*n is in [k*2⁶⁴, k*2⁶⁴+(2⁶⁴%n)), meaning rejecting fewer than n possible
    // outcomes out of the 2⁶⁴.
    // Now there are exactly floor(2⁶⁴/n) possible ways to produce
    // each output value k, so we've restored uniformity.
    // To get valid uint64 math, 2⁶⁴ % n = (2⁶⁴ - n) % n = -n % n,
    // so the direct implementation of this algorithm would be:
    //
    //	hi, lo := bits.Mul64(r.Uint64(), n)
    //	thresh := -n % n
    //	for lo < thresh {
    //		hi, lo = bits.Mul64(r.Uint64(), n)
    //	}
    //
    // That still leaves an expensive 64-bit division that we would rather avoid.
    // We know that thresh < n, and n is usually much less than 2⁶⁴, so we can
    // avoid the last four lines unless lo < n.
    //
    // See also:
    // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction
    // https://lemire.me/blog/2016/06/30/fast-random-shuffling
    so_R_u64_u64 _res1 = bits_Mul64(rand_Rand_Uint64(r), n);
    uint64_t hi = _res1.val;
    uint64_t lo = _res1.val2;
    if (lo < n) {
        uint64_t thresh = -n % n;
        for (; lo < thresh;) {
            so_R_u64_u64 _res2 = bits_Mul64(rand_Rand_Uint64(r), n);
            hi = _res2.val;
            lo = _res2.val2;
        }
    }
    return hi;
}

// uint32n is an identical computation to uint64n
// but optimized for 32-bit systems.
static uint32_t rand_Rand_uint32n(void* self, uint32_t n) {
    rand_Rand* r = self;
    if ((n & (n - 1)) == 0) {
        // n is power of two, can mask
        return ((uint32_t)(rand_Rand_Uint64(r)) & (n - 1));
    }
    // On 64-bit systems we still use the uint64 code below because
    // the probability of a random uint64 lo being < a uint32 n is near zero,
    // meaning the unbiasing loop almost never runs.
    // On 32-bit systems, here we need to implement that same logic in 32-bit math,
    // both to preserve the exact output sequence observed on 64-bit machines
    // and to preserve the optimization that the unbiasing loop almost never runs.
    //
    // We want to compute
    // 	hi, lo := bits.Mul64(r.Uint64(), n)
    // In terms of 32-bit halves, this is:
    // 	x1:x0 := r.Uint64()
    // 	0:hi, lo1:lo0 := bits.Mul64(x1:x0, 0:n)
    // Writing out the multiplication in terms of bits.Mul32 allows
    // using direct hardware instructions and avoiding
    // the computations involving these zeros.
    uint64_t x = rand_Rand_Uint64(r);
    so_R_u32_u32 _res1 = bits_Mul32((uint32_t)(x), n);
    uint32_t lo1a = _res1.val;
    uint32_t lo0 = _res1.val2;
    so_R_u32_u32 _res2 = bits_Mul32((uint32_t)(x >> 32), n);
    uint32_t hi = _res2.val;
    uint32_t lo1b = _res2.val2;
    so_R_u32_u32 _res3 = bits_Add32(lo1a, lo1b, 0);
    uint32_t lo1 = _res3.val;
    uint32_t c = _res3.val2;
    hi += c;
    if (lo1 == 0 && lo0 < (uint32_t)(n)) {
        uint64_t n64 = (uint64_t)(n);
        uint32_t thresh = (uint32_t)(-n64 % n64);
        for (; lo1 == 0 && lo0 < thresh;) {
            uint64_t x = rand_Rand_Uint64(r);
            so_R_u32_u32 _res4 = bits_Mul32((uint32_t)(x), n);
            lo1a = _res4.val;
            lo0 = _res4.val2;
            so_R_u32_u32 _res5 = bits_Mul32((uint32_t)(x >> 32), n);
            hi = _res5.val;
            lo1b = _res5.val2;
            so_R_u32_u32 _res6 = bits_Add32(lo1a, lo1b, 0);
            lo1 = _res6.val;
            c = _res6.val2;
            hi += c;
        }
    }
    return hi;
}

// Int32N returns, as an int32, a non-negative pseudo-random number
// in the half-open interval [0,n). It panics if n <= 0.
int32_t rand_Rand_Int32N(void* self, int32_t n) {
    rand_Rand* r = self;
    if (n <= 0) {
        so_panic("invalid argument to Int32N");
    }
    return (int32_t)(rand_Rand_uint64n(r, (uint64_t)(n)));
}

// Uint32N returns, as a uint32, a non-negative pseudo-random number
// in the half-open interval [0,n). It panics if n == 0.
uint32_t rand_Rand_Uint32N(void* self, uint32_t n) {
    rand_Rand* r = self;
    if (n == 0) {
        so_panic("invalid argument to Uint32N");
    }
    return (uint32_t)(rand_Rand_uint64n(r, (uint64_t)(n)));
}

// IntN returns, as an int, a non-negative pseudo-random number
// in the half-open interval [0,n). It panics if n <= 0.
so_int rand_Rand_IntN(void* self, so_int n) {
    rand_Rand* r = self;
    if (n <= 0) {
        so_panic("invalid argument to IntN");
    }
    return (so_int)(rand_Rand_uint64n(r, (uint64_t)(n)));
}

// UintN returns, as a uint, a non-negative pseudo-random number
// in the half-open interval [0,n). It panics if n == 0.
so_uint rand_Rand_UintN(void* self, so_uint n) {
    rand_Rand* r = self;
    if (n == 0) {
        so_panic("invalid argument to UintN");
    }
    return (so_uint)(rand_Rand_uint64n(r, (uint64_t)(n)));
}

// Float64 returns, as a float64, a pseudo-random number
// in the half-open interval [0.0,1.0).
double rand_Rand_Float64(void* self) {
    rand_Rand* r = self;
    // There are exactly 1<<53 float64s in [0,1). Use Intn(1<<53) / (1<<53).
    const int64_t n = ((int64_t)1 << 53);
    return (double)((rand_Rand_Uint64(r) << 11) >> 11) / (double)(n);
}

// Float32 returns, as a float32, a pseudo-random number
// in the half-open interval [0.0,1.0).
float rand_Rand_Float32(void* self) {
    rand_Rand* r = self;
    // There are exactly 1<<24 float32s in [0,1). Use Intn(1<<24) / (1<<24).
    const int64_t n = ((int64_t)1 << 24);
    return (float)((rand_Rand_Uint32(r) << 8) >> 8) / (float)(n);
}

// Int64 returns a non-negative pseudo-random 63-bit integer as an int64
// from the default Source.
int64_t rand_Int64(void) {
    return rand_Rand_Int64(&globalRand);
}

// Uint32 returns a pseudo-random 32-bit value as a uint32
// from the default Source.
uint32_t rand_Uint32(void) {
    return rand_Rand_Uint32(&globalRand);
}

// Uint64N returns, as a uint64, a pseudo-random number in the half-open interval [0,n)
// from the default Source.
// It panics if n == 0.
uint64_t rand_Uint64N(uint64_t n) {
    return rand_Rand_Uint64N(&globalRand, n);
}

// Uint32N returns, as a uint32, a pseudo-random number in the half-open interval [0,n)
// from the default Source.
// It panics if n == 0.
uint32_t rand_Uint32N(uint32_t n) {
    return rand_Rand_Uint32N(&globalRand, n);
}

// Uint64 returns a pseudo-random 64-bit value as a uint64
// from the default Source.
uint64_t rand_Uint64(void) {
    return rand_Rand_Uint64(&globalRand);
}

// Int32 returns a non-negative pseudo-random 31-bit integer as an int32
// from the default Source.
int32_t rand_Int32(void) {
    return rand_Rand_Int32(&globalRand);
}

// Int returns a non-negative pseudo-random int from the default Source.
so_int rand_Int(void) {
    return rand_Rand_Int(&globalRand);
}

// Uint returns a pseudo-random uint from the default Source.
so_uint rand_Uint(void) {
    return rand_Rand_Uint(&globalRand);
}

// Int64N returns, as an int64, a pseudo-random number in the half-open interval [0,n)
// from the default Source.
// It panics if n <= 0.
int64_t rand_Int64N(int64_t n) {
    return rand_Rand_Int64N(&globalRand, n);
}

// Int32N returns, as an int32, a pseudo-random number in the half-open interval [0,n)
// from the default Source.
// It panics if n <= 0.
int32_t rand_Int32N(int32_t n) {
    return rand_Rand_Int32N(&globalRand, n);
}

// IntN returns, as an int, a pseudo-random number in the half-open interval [0,n)
// from the default Source.
// It panics if n <= 0.
so_int rand_IntN(so_int n) {
    return rand_Rand_IntN(&globalRand, n);
}

// UintN returns, as a uint, a pseudo-random number in the half-open interval [0,n)
// from the default Source.
// It panics if n == 0.
so_uint rand_UintN(so_uint n) {
    return rand_Rand_UintN(&globalRand, n);
}

// Float64 returns, as a float64, a pseudo-random number in the half-open interval [0.0,1.0)
// from the default Source.
double rand_Float64(void) {
    return rand_Rand_Float64(&globalRand);
}

// Float32 returns, as a float32, a pseudo-random number in the half-open interval [0.0,1.0)
// from the default Source.
float rand_Float32(void) {
    return rand_Rand_Float32(&globalRand);
}

static void __attribute__((constructor)) rand_init() {
    globalSource = rand_NewPCG(runtime_Seed(), runtime_Seed());
    globalRand = rand_New((rand_Source){.self = &globalSource, .Uint64 = rand_PCG_Uint64});
}
