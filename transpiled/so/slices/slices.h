#pragma once
#include "so/builtin/builtin.h"
#include "so/c/c.h"
#include "so/cmp/cmp.h"
#include "so/math/bits/bits.h"
#include "so/mem/mem.h"

// -- Embeds --

// TryAppend appends elements to a heap-allocated slice, growing it if needed.
// Returns a result with the updated slice or an error if reallocation fails.
// If the allocator is nil, uses the system allocator.
#define slices_TryAppend(T, a, s, ...) ({                               \
    T _vals[] = {__VA_ARGS__};                                          \
    so_int _n = (so_int)(sizeof(_vals) / sizeof(T));                    \
    slices_tryExtend((a), (s),                                          \
                     (so_Slice){(so_byte*)_vals, _n, _n},               \
                     (so_int)sizeof(T), (so_int)alignof(so_typeof(T))); \
})

// Append appends elements to a heap-allocated slice, growing it if needed.
// Returns the updated slice or panics on allocation failure.
// If the allocator is nil, uses the system allocator.
#define slices_Append(T, a, s, ...) ({                               \
    T _vals[] = {__VA_ARGS__};                                       \
    so_int _n = (so_int)(sizeof(_vals) / sizeof(T));                 \
    slices_extend((a), (s),                                          \
                  (so_Slice){(so_byte*)_vals, _n, _n},               \
                  (so_int)sizeof(T), (so_int)alignof(so_typeof(T))); \
})

// TryExtend appends all elements from another slice, growing if needed.
// Returns a result with the updated slice or an error if reallocation fails.
// If the allocator is nil, uses the system allocator.
#define slices_TryExtend(T, a, s, other)                     \
    slices_tryExtend((a), (s), (other), (so_int)(sizeof(T)), \
                     (so_int)(alignof(so_typeof(T))))

// Extend appends all elements from another slice, growing if needed.
// Returns the updated slice or panics on allocation failure.
// If the allocator is nil, uses the system allocator.
#define slices_Extend(T, a, s, other)                     \
    slices_extend((a), (s), (other), (so_int)(sizeof(T)), \
                  (so_int)(alignof(so_typeof(T))))

// Header returns the Slice header for a given slice.
#define slices_Header(T, s) (s)

// -- Types --

typedef struct slices_Sorter slices_Sorter;

// Sorter provides comparison and swapping
// operations for a slice of any type.
typedef struct slices_Sorter {
    so_Slice slice;
    so_int esize;
    cmp_Func compare;
} slices_Sorter;

// -- Functions and methods --

// nextcap computes the capacity for a grown slice using Go's growth
// formula: 2x for small slices (< 256 elements), transitioning to ~1.25x
// for larger ones.
//
static inline so_int slices_nextcap(so_int newLen, so_int oldCap) {
    so_int newCap = oldCap;
    so_int doubleCap = newCap + newCap;
    if (newLen > doubleCap) {
        return newLen;
    }
    const int64_t threshold = 256;
    if (oldCap < threshold) {
        return doubleCap;
    }
    for (;;) {
        newCap += ((newCap + 3 * threshold) >> 2);
        if (newCap >= newLen) {
            break;
        }
    }
    return newCap;
}

// grow grows a slice's backing allocation to hold at least newLen elements.
// Returns a result with the updated slice or an error if reallocation fails.
// If the allocator is nil, uses the system allocator.
//
static inline so_R_slice_err slices_grow(mem_Allocator a, so_Slice s, so_int newLen, so_int elemSize, so_int elemAlign) {
    if (a.self == NULL) {
        a = mem_System;
    }
    if (newLen <= s.cap) {
        return (so_R_slice_err){.val = s, .err = (so_Error){0}};
    }
    so_int newCap = slices_nextcap(newLen, s.cap);
    so_int oldSize = s.cap * elemSize;
    so_int newSize = newCap * elemSize;
    void* newPtr = NULL;
    so_Error err = {0};
    if (s.cap == 0) {
        so_R_ptr_err _res1 = a.Alloc(a.self, newSize, elemAlign);
        newPtr = _res1.val;
        err = _res1.err;
    } else {
        so_R_ptr_err _res2 = a.Realloc(a.self, s.ptr, oldSize, newSize, elemAlign);
        newPtr = _res2.val;
        err = _res2.err;
    }
    if (err.self != NULL) {
        return (so_R_slice_err){.val = s, .err = err};
    }
    s.ptr = (so_byte*)newPtr;
    s.cap = newCap;
    return (so_R_slice_err){.val = s, .err = (so_Error){0}};
}

// tryExtend appends all elements from another slice, growing if needed.
// Returns a result with the updated slice or an error if reallocation fails.
// If the allocator is nil, uses the system allocator.
//
static inline so_R_slice_err slices_tryExtend(mem_Allocator a, so_Slice s, so_Slice other, so_int elemSize, so_int elemAlign) {
    so_R_slice_err res = slices_grow(a, s, s.len + other.len, elemSize, elemAlign);
    if (res.err.self != NULL) {
        return res;
    }
    s = res.val;
    mem_Copy(c_PtrAdd(so_byte, (s.ptr), (s.len * elemSize)), other.ptr, other.len * elemSize);
    s.len += other.len;
    return (so_R_slice_err){.val = s, .err = (so_Error){0}};
}

// extend appends all elements from another slice, growing if needed.
// Returns the updated slice or panics on allocation failure.
// If the allocator is nil, uses the system allocator.
//
static inline so_Slice slices_extend(mem_Allocator a, so_Slice s, so_Slice other, so_int elemSize, so_int elemAlign) {
    so_R_slice_err res = slices_tryExtend(a, s, other, elemSize, elemAlign);
    if (res.err.self != NULL) {
        so_panic(so_error_cstr(res.err));
    }
    return res.val;
}

// Make allocates a slice of type T with given length using allocator a.
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
//
#define slices_Make(T, a_, len_) ({ \
    mem_AllocSlice(T, (a_), (len_), (len_)); \
})

// MakeCap allocates a slice of type T with given length and capacity using allocator a.
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
//
#define slices_MakeCap(T, a_, len_, cap_) ({ \
    mem_AllocSlice(T, (a_), (len_), (cap_)); \
})

// Free frees a previously allocated slice.
// If the allocator is nil, uses the system allocator.
//
#define slices_Free(T, a_, s_) do { \
    mem_FreeSlice(T, (a_), (s_)); \
} while (0)

// Clone returns a shallow copy of the slice.
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
//
#define slices_Clone(T, a_, s_) ({ \
    so_Slice _s = s_; \
    so_int _slen = so_len(s_); \
    so_int _elemSize = c_Sizeof(T); \
    so_Slice _newSlice = mem_AllocSlice(T, (a_), (_slen), (_slen)); \
    mem_Copy(unsafe_SliceData(_newSlice), unsafe_SliceData(_s), _slen * _elemSize); \
    _newSlice; \
})

// Equal reports whether two slices are equal: the same length and all
// elements equal. Empty and nil slices are considered equal.
//
#define slices_Equal(T, s1_, s2_) ({ \
    so_Slice _s1 = s1_, _s2 = s2_; \
    bool _eq = so_len(_s1) == so_len(_s2); \
    for (so_int i = 0; i < so_len(_s1) && _eq; i++) { \
        T _v1 = so_at(T, _s1, i), _v2 = so_at(T, _s2, i); \
        _eq = cmp_Equal(T, (_v1), (_v2)); \
    } \
    _eq; \
})

// Contains reports whether v is present in s.
//
#define slices_Contains(T, s_, v_) ({ \
    slices_Index(T, (s_), (v_)) >= 0; \
})

// Index returns the index of the first occurrence of v in s,
// or -1 if not present.
//
#define slices_Index(T, s_, v_) ({ \
    so_Slice _s = s_; \
    T _v = v_; \
    so_int _idx = -1; \
    for (so_int _j = 0; _j < so_len(_s); _j++) { \
        T _sj = so_at(T, _s, _j); \
        if (cmp_Equal(T, (_sj), (_v))) { \
            _idx = _j; \
            break; \
        } \
    } \
    _idx; \
})

// NewSorter creates a Sorter for a given slice with a custom compare function.
// If compare is nil, compares by raw byte value (memcmp).
//
#define slices_NewSorter(T, s_, compare_) ({ \
    (slices_Sorter){.slice = slices_Header(T, (s_)), .esize = c_Sizeof(T), .compare = compare_}; \
})

// Compare compares the elements at indices i and j.
// Returns a negative value if s[i] < s[j], zero if they are equal,
// and a positive value if s[i] > s[j].
so_int slices_Sorter_Compare(slices_Sorter s, so_int i, so_int j);

// Less reports whether the element at index i
// should sort before the element at index j.
bool slices_Sorter_Less(slices_Sorter s, so_int i, so_int j);

// Swap swaps the elements at indices i and j.
void slices_Sorter_Swap(slices_Sorter s, so_int i, so_int j);

// Sort sorts a slice of any ordered type in ascending order.
//
#define slices_Sort(T, x_) do { \
    slices_Sorter _s = slices_NewSorter(T, (x_), (cmp_FuncFor(T))); \
    slices_SortWith(_s); \
} while (0)

// SortFunc sorts the slice x in ascending order as determined by the cmp
// function. This sort is not guaranteed to be stable.
// cmp(a, b) should return a negative number when a < b, a positive number when
// a > b and zero when a == b or a and b are incomparable in the sense of
// a strict weak ordering.
//
// SortFunc requires that cmp is a strict weak ordering.
// See https://en.wikipedia.org/wiki/Weak_ordering#Strict_weak_orderings.
// The function should return 0 for incomparable items.
//
#define slices_SortFunc(T, x_, compare_) do { \
    slices_Sorter _s = slices_NewSorter(T, (x_), (compare_)); \
    slices_SortWith(_s); \
} while (0)

// SortWith sorts the slice using the provided Sorter.
void slices_SortWith(slices_Sorter s);

// SortStableFunc sorts the slice x while keeping the original order of equal
// elements, using cmp to compare elements in the same way as [SortFunc].
//
#define slices_SortStableFunc(T, x_, compare_) do { \
    slices_Sorter _s = slices_NewSorter(T, (x_), (compare_)); \
    slices_SortStableWith(_s); \
} while (0)

// SortStableWith sorts the slice using the provided Sorter
// while keeping the original order of equal elements.
void slices_SortStableWith(slices_Sorter s);

// IsSorted reports whether x is sorted in ascending order.
//
#define slices_IsSorted(T, x_) ({ \
    slices_Sorter _s = slices_NewSorter(T, (x_), (cmp_FuncFor(T))); \
    slices_IsSortedWith(_s); \
})

// IsSortedFunc reports whether x is sorted in ascending order, with cmp as the
// comparison function as defined by [SortFunc].
//
#define slices_IsSortedFunc(T, x_, compare_) ({ \
    slices_Sorter _s = slices_NewSorter(T, (x_), (compare_)); \
    slices_IsSortedWith(_s); \
})

// IsSortedWith reports whether the slice is sorted
// according to the provided Sorter.
bool slices_IsSortedWith(slices_Sorter s);

// Min returns the minimal value in x. It panics if x is empty.
// For floating-point numbers, Min propagates NaNs (any NaN value in x
// forces the output to be NaN).
//
#define slices_Min(T, x_) ({ \
    slices_MinFunc(T, (x_), (cmp_FuncFor(T))); \
})

// MinFunc returns the minimal value in x, using cmp to compare elements.
// It panics if x is empty. If there is more than one minimal element
// according to the cmp function, MinFunc returns the first one.
//
#define slices_MinFunc(T, x_, compare_) ({ \
    so_Slice _x = x_; \
    if (so_len(_x) < 1) { \
        so_panic("slices: empty list"); \
    } \
    T _m = so_at(T, _x, 0); \
    for (so_int _j = 1; _j < so_len(_x); _j++) { \
        T _xj = so_at(T, _x, _j); \
        if (compare_(&_xj, &_m) < 0) { \
            _m = _xj; \
        } \
    } \
    _m; \
})

// Max returns the maximal value in x. It panics if x is empty.
// For floating-point E, Max propagates NaNs (any NaN value in x
// forces the output to be NaN).
//
#define slices_Max(T, x_) ({ \
    slices_MaxFunc(T, (x_), (cmp_FuncFor(T))); \
})

// MaxFunc returns the maximal value in x, using cmp to compare elements.
// It panics if x is empty. If there is more than one maximal element
// according to the cmp function, MaxFunc returns the first one.
//
#define slices_MaxFunc(T, x_, compare_) ({ \
    so_Slice _x = x_; \
    if (so_len(_x) < 1) { \
        so_panic("slices.MaxFunc: empty list"); \
    } \
    T _m = so_at(T, _x, 0); \
    for (so_int _j = 1; _j < so_len(_x); _j++) { \
        T _xj = so_at(T, _x, _j); \
        if (compare_(&_xj, &_m) > 0) { \
            _m = _xj; \
        } \
    } \
    _m; \
})
