// base features. Macros and types every file uses.
#pragma once
#include <SDL3/SDL.h>
#include <assert.h>
#include <stddef.h>
#include "easy_memory.h"

// ----- MACRO MAGIC -----

// DEFER
// https://gustedt.wordpress.com/2026/02/15/defer-available-in-gcc-and-clang/
// Tldr: Clang has a defer feature. GCC can use it with with macro magic.
#if __has_include(<stddefer.h>)
#include <stddefer.h>
#if defined(__clang__)
#if __is_identifier(_Defer)
#error "clang may need the option -fdefer-ts for the _Defer feature"
#endif
#endif
#elif __GNUC__ > 8
#define defer _Defer
#define _Defer _Defer_A(__COUNTER__)
#define _Defer_A(N) _Defer_B(N)
#define _Defer_B(N) _Defer_C(_Defer_func_##N, _Defer_var_##N)
#define _Defer_C(F, V)                                               \
  auto void F(int*);                                                 \
  __attribute__((__cleanup__(F), __deprecated__, __unused__)) int V; \
  __attribute__((__always_inline__, __deprecated__,                  \
                 __unused__)) inline auto void                       \
  F(__attribute__((__unused__)) int* V)
#else
#error "The _Defer feature seems not available"
#endif

// Result type for a function.
#define ResultDef(name, T, E) \
  typedef struct {            \
    bool ok;                  \
    T result;                 \
    E err;                    \
  } name##Result

#define Ok(FunctionName, v) ((FunctionName##Result){.ok = true, .result = (v)})
#define OkAnd(FunctionName, v, e) \
  ((FunctionName##Result){.ok = true, .result = (v), .err = (e)})

// Error and zero value
#define Err(FunctionName, e) ((FunctionName##Result){.ok = false, .err = (e)})

// Error and default value.
#define ErrAnd(FunctionName, e, v) \
  ((FunctionName##Result){.ok = false, .err = (e), .result = (v)})

// allocate on arena
#define new(T) ((T*)em_alloc(em, sizeof(T)))
// delete from arena.
#define delete(v) em_free(v);

// allocate on scratch bump allocator.
// You probably should just allocate on the stack.
#define new_tmp(T) ((T*)em_bump_alloc(scratch, sizeof(T)))

/* Accessors */

// ----- STRING TYPE -----
#include <assert.h>
#include <string.h>

#define ArrayDecl(T, name) \
  typedef struct {         \
    size_t len;            \
    size_t cap;            \
    T* items;              \
  } name;

// make an array xs using allocator em, with capacity.
#define make(em, xs, capacity)                                 \
  do {                                                         \
    xs.len = 0;                                                \
    xs.cap = (capacity);                                       \
    xs.items = em_alloc((em), sizeof(*xs.items) * (capacity)); \
    assert(xs.items);                                          \
  } while (0)

#define append(xs, x)                                                  \
  do {                                                                 \
    assert((xs).len < (xs).cap && "Tried appending to a full array."); \
    (xs).items[(xs).len++] = (x);                                      \
  } while (0)

// append array xs2 into dst
#define appendA(dst, xs2)                                                 \
  do {                                                                    \
    assert((dst).len + (xs2).len <= (dst).cap && "Not enough capacity."); \
    memcpy(&(dst).items[(dst).len], (xs2).items,                          \
           (xs2).len * sizeof(*(dst).items));                             \
    (dst).len += (xs2).len;                                               \
  } while (0)
// String is a C-string compatible string.
// The length does not include the null-terminator.
ArrayDecl(char, string);

// str interprets a C-string as a String. (string view)
static inline string str(char* s) {
  return (string){.items = s, .len = SDL_strlen(s)};
}

// strC concatenates two strings.
// Implicitly passes bump allocator
static inline string strCat(EM* em, string a, string b) {
  size_t total = a.len + b.len + 1;  // +1 for null terminator.
  string s;
  make(em, s, total);
  appendA(s, a);
  appendA(s, b);
  append(s, 0);
  s.len--;  // do not count null terminator.
  return s;
}
typedef string error;

// ----- TIME -----

constexpr Sint64 Time_Nanosecond = 1;
constexpr Sint64 Time_Microsecond = 1000 * Time_Nanosecond;
constexpr Sint64 Time_Millisecond = 1000 * Time_Microsecond;
constexpr Sint64 Time_Second = 1000 * Time_Millisecond;
constexpr Sint64 Time_Minute = 60 * Time_Second;
constexpr Sint64 Time_Hour = 60 * Time_Minute;

// Sleeps the thread for the specified duration.
auto Time_Sleep = SDL_DelayNS;
