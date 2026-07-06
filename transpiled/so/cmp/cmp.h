#pragma once
#include "so/builtin/builtin.h"
#include "so/c/c.h"
#include "so/mem/mem.h"

// -- Embeds --

#include "so/builtin/builtin.h"

// FuncFor returns the appropriate comparison function for type T.
// If T is not supported, returns NULL.
#define cmp_FuncFor(T)         \
    _Generic((T){0},           \
        uint8_t: cmp_u8,       \
        uint16_t: cmp_u16,     \
        uint32_t: cmp_u32,     \
        uint64_t: cmp_u64,     \
        int8_t: cmp_i8,        \
        int16_t: cmp_i16,      \
        int32_t: cmp_i32,      \
        int64_t: cmp_i64,      \
        float: cmp_f32,        \
        double: cmp_f64,       \
        so_String: cmp_string, \
        default: NULL)

#define cmp_isNaN(x) ((x) != (x))

#define SO_DEFINE_CMP(name, type)                 \
    static inline so_int name(void* x, void* y) { \
        type vx = *(type*)x, vy = *(type*)y;      \
        bool xNaN = cmp_isNaN(vx);                \
        bool yNaN = cmp_isNaN(vy);                \
        if (xNaN && !yNaN) return -1;             \
        if (!xNaN && yNaN) return +1;             \
        if (xNaN && yNaN) return 0;               \
        return (vx > vy) - (vx < vy);             \
    }

SO_DEFINE_CMP(cmp_i8, int8_t)
SO_DEFINE_CMP(cmp_i16, int16_t)
SO_DEFINE_CMP(cmp_i32, int32_t)
SO_DEFINE_CMP(cmp_i64, int64_t)
SO_DEFINE_CMP(cmp_u8, uint8_t)
SO_DEFINE_CMP(cmp_u16, uint16_t)
SO_DEFINE_CMP(cmp_u32, uint32_t)
SO_DEFINE_CMP(cmp_u64, uint64_t)
SO_DEFINE_CMP(cmp_f32, float)
SO_DEFINE_CMP(cmp_f64, double)

#undef SO_DEFINE_CMP

static inline so_int cmp_string(void* x, void* y) {
    const so_String* s1 = (const so_String*)x;
    const so_String* s2 = (const so_String*)y;
    so_int n = s1->len < s2->len ? s1->len : s2->len;
    int cmp = n > 0 ? memcmp(s1->ptr, s2->ptr, (size_t)n) : 0;
    if (cmp != 0) return cmp;
    if (s1->len < s2->len) return -1;
    if (s1->len > s2->len) return +1;
    return 0;
}

// -- Types --

// Func is a comparison function that returns a negative value if a < b,
// zero if a == b, and a positive value if a > b.
typedef so_int (*cmp_Func)(void*, void*);

// -- Functions and methods --

// Compare returns
//
//	-1 if x is less than y,
//	 0 if x equals y,
//	+1 if x is greater than y.
//
// For floating-point types, a NaN is considered less than any non-NaN,
// a NaN is considered equal to a NaN, and -0.0 is equal to 0.0.
//
#define cmp_Compare(T, x_, y_) ({ \
    cmp_Func _fn = cmp_FuncFor(T); \
    c_Assert(_fn != NULL, "cmp: unsupported ordered type"); \
    _fn(&x_, &y_); \
})

// Equal reports whether x and y are equal.
// For floating-point types, a NaN is considered equal to a NaN, and -0.0 is equal to 0.0.
// For non-ordered types, compares by raw byte value (memcmp).
//
#define cmp_Equal(T, x_, y_) ({ \
    cmp_Func _fn = cmp_FuncFor(T); \
    bool _eq = false; \
    if (_fn != NULL) { \
        _eq = _fn(&x_, &y_) == 0; \
    } else { \
        _eq = mem_Compare(&x_, &y_, c_Sizeof(T)) == 0; \
    } \
    _eq; \
})

// Less reports whether x is less than y.
// For floating-point types, a NaN is considered less than any non-NaN,
// and -0.0 is not less than (is equal to) 0.0.
//
#define cmp_Less(T, x_, y_) ({ \
    cmp_Func _fn = cmp_FuncFor(T); \
    c_Assert(_fn != NULL, "cmp: unsupported ordered type"); \
    _fn(&x_, &y_) < 0; \
})
