// base features. Macros and types every file uses.
#pragma once
#include <SDL3/SDL_stdinc.h>
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

// String is a C-string compatible string.
// The length does not include the null-terminator.
typedef struct {
  size_t len;
  const char* cstr;
} string;

typedef string error;

// str interprets a C-string as a String.
static inline string str(const char* s) {
  return (string){.cstr = s, .len = strlen(s)};
}

// strC concatenates two strings.
// Implicitly passes bump allocator
#define strCat(a, b) String_cat(scratch, a, b)

static inline string String_cat(Bump* scratch, string a, string b) {
  size_t total = a.len + b.len;
  char* buf = em_bump_alloc(scratch, total + 1);
  if (!buf)
    return (string){};

  memcpy(buf, a.cstr, a.len);
  memcpy(buf + a.len, b.cstr, b.len);
  buf[total] = '\0';

  return (string){.cstr = buf, .len = total};
}

// ----- TIME -----

constexpr Sint64 Time_Nanosecond = 1;
constexpr Sint64 Time_Microsecond = 1000 * Time_Nanosecond;
constexpr Sint64 Time_Millisecond = 1000 * Time_Microsecond;
constexpr Sint64 Time_Second = 1000 * Time_Millisecond;
constexpr Sint64 Time_Minute = 60 * Time_Second;
constexpr Sint64 Time_Hour = 60 * Time_Minute;
