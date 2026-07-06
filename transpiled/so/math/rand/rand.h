#pragma once
#include "so/builtin/builtin.h"
#include "so/encoding/binary/binary.h"
#include "so/errors/errors.h"
#include "so/math/bits/bits.h"
#include "so/runtime/runtime.h"

// -- Types --

typedef struct rand_PCG rand_PCG;
typedef struct rand_Rand rand_Rand;

// A PCG is a PCG generator with 128 bits of internal state.
// A zero PCG is equivalent to NewPCG(0, 0).
typedef struct rand_PCG {
    uint64_t hi;
    uint64_t lo;
} rand_PCG;

// A Source is a source of uniformly-distributed
// pseudo-random uint64 values in the range [0, 1<<64).
//
// A Source is not safe for concurrent use by multiple threads.
typedef struct rand_Source {
    void* self;
    uint64_t (*Uint64)(void* self);
} rand_Source;

// A Rand is a source of random numbers.
typedef struct rand_Rand {
    rand_Source src;
} rand_Rand;

// -- Variables and constants --

// https://numpy.org/devdocs/reference/random/upgrading-pcg64.html
// https://github.com/imneme/pcg-cpp/commit/871d0494ee9c9a7b7c43f753e3d8ca47c26f8005
extern so_Error rand_ErrUnmarshalPCG;

// -- Functions and methods --

// NewPCG returns a new PCG seeded with the given values.
rand_PCG rand_NewPCG(uint64_t seed1, uint64_t seed2);

// Seed resets the PCG to behave the same way as NewPCG(seed1, seed2).
void rand_PCG_Seed(void* self, uint64_t seed1, uint64_t seed2);

// AppendBinary implements the [encoding.BinaryAppender] interface.
// Requires at least 20 bytes of spare capacity in b.
so_R_slice_err rand_PCG_AppendBinary(void* self, so_Slice b);

// MarshalBinary implements the [encoding.BinaryMarshaler] interface.
// Requires a 20-byte buffer in b.
so_R_slice_err rand_PCG_MarshalBinary(void* self, so_Slice b);

// UnmarshalBinary implements the [encoding.BinaryUnmarshaler] interface.
so_Error rand_PCG_UnmarshalBinary(void* self, so_Slice data);

// Uint64 return a uniformly-distributed random uint64 value.
uint64_t rand_PCG_Uint64(void* self);

// New returns a new Rand that uses random values from src
// to generate other random values.
rand_Rand rand_New(rand_Source src);

// Int64 returns a non-negative pseudo-random 63-bit integer as an int64.
int64_t rand_Rand_Int64(void* self);

// Uint32 returns a pseudo-random 32-bit value as a uint32.
uint32_t rand_Rand_Uint32(void* self);

// Uint64 returns a pseudo-random 64-bit value as a uint64.
uint64_t rand_Rand_Uint64(void* self);

// Int32 returns a non-negative pseudo-random 31-bit integer as an int32.
int32_t rand_Rand_Int32(void* self);

// Int returns a non-negative pseudo-random int.
so_int rand_Rand_Int(void* self);

// Uint returns a pseudo-random uint.
so_uint rand_Rand_Uint(void* self);

// Int64N returns, as an int64, a non-negative pseudo-random number
// in the half-open interval [0,n). It panics if n <= 0.
int64_t rand_Rand_Int64N(void* self, int64_t n);

// Uint64N returns, as a uint64, a non-negative pseudo-random number
// in the half-open interval [0,n). It panics if n == 0.
uint64_t rand_Rand_Uint64N(void* self, uint64_t n);

// Int32N returns, as an int32, a non-negative pseudo-random number
// in the half-open interval [0,n). It panics if n <= 0.
int32_t rand_Rand_Int32N(void* self, int32_t n);

// Uint32N returns, as a uint32, a non-negative pseudo-random number
// in the half-open interval [0,n). It panics if n == 0.
uint32_t rand_Rand_Uint32N(void* self, uint32_t n);

// IntN returns, as an int, a non-negative pseudo-random number
// in the half-open interval [0,n). It panics if n <= 0.
so_int rand_Rand_IntN(void* self, so_int n);

// UintN returns, as a uint, a non-negative pseudo-random number
// in the half-open interval [0,n). It panics if n == 0.
so_uint rand_Rand_UintN(void* self, so_uint n);

// Float64 returns, as a float64, a pseudo-random number
// in the half-open interval [0.0,1.0).
double rand_Rand_Float64(void* self);

// Float32 returns, as a float32, a pseudo-random number
// in the half-open interval [0.0,1.0).
float rand_Rand_Float32(void* self);

// Int64 returns a non-negative pseudo-random 63-bit integer as an int64
// from the default Source.
int64_t rand_Int64(void);

// Uint32 returns a pseudo-random 32-bit value as a uint32
// from the default Source.
uint32_t rand_Uint32(void);

// Uint64N returns, as a uint64, a pseudo-random number in the half-open interval [0,n)
// from the default Source.
// It panics if n == 0.
uint64_t rand_Uint64N(uint64_t n);

// Uint32N returns, as a uint32, a pseudo-random number in the half-open interval [0,n)
// from the default Source.
// It panics if n == 0.
uint32_t rand_Uint32N(uint32_t n);

// Uint64 returns a pseudo-random 64-bit value as a uint64
// from the default Source.
uint64_t rand_Uint64(void);

// Int32 returns a non-negative pseudo-random 31-bit integer as an int32
// from the default Source.
int32_t rand_Int32(void);

// Int returns a non-negative pseudo-random int from the default Source.
so_int rand_Int(void);

// Uint returns a pseudo-random uint from the default Source.
so_uint rand_Uint(void);

// Int64N returns, as an int64, a pseudo-random number in the half-open interval [0,n)
// from the default Source.
// It panics if n <= 0.
int64_t rand_Int64N(int64_t n);

// Int32N returns, as an int32, a pseudo-random number in the half-open interval [0,n)
// from the default Source.
// It panics if n <= 0.
int32_t rand_Int32N(int32_t n);

// IntN returns, as an int, a pseudo-random number in the half-open interval [0,n)
// from the default Source.
// It panics if n <= 0.
so_int rand_IntN(so_int n);

// UintN returns, as a uint, a pseudo-random number in the half-open interval [0,n)
// from the default Source.
// It panics if n == 0.
so_uint rand_UintN(so_uint n);

// Float64 returns, as a float64, a pseudo-random number in the half-open interval [0.0,1.0)
// from the default Source.
double rand_Float64(void);

// Float32 returns, as a float32, a pseudo-random number in the half-open interval [0.0,1.0)
// from the default Source.
float rand_Float32(void);
