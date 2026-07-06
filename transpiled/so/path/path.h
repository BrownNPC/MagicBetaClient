#pragma once
#include "so/builtin/builtin.h"
#include "so/bytealg/bytealg.h"
#include "so/errors/errors.h"
#include "so/mem/mem.h"
#include "so/stringslite/stringslite.h"
#include "so/unicode/utf8/utf8.h"

// -- Variables and constants --

// ErrBadPattern indicates a pattern was malformed.
extern so_Error path_ErrBadPattern;

// -- Functions and methods --

// Match reports whether name matches the shell pattern.
// The pattern syntax is:
//
//	pattern:
//		{ term }
//	term:
//		'*'         matches any sequence of non-/ characters
//		'?'         matches any single non-/ character
//		'[' [ '^' ] { character-range } ']'
//		            character class (must be non-empty)
//		c           matches character c (c != '*', '?', '\\', '[')
//		'\\' c      matches character c
//
//	character-range:
//		c           matches character c (c != '\\', '-', ']')
//		'\\' c      matches character c
//		lo '-' hi   matches character c for lo <= c <= hi
//
// Match requires pattern to match all of name, not just a substring.
// The only possible returned error is [ErrBadPattern], when pattern
// is malformed.
so_R_bool_err path_Match(so_String pattern, so_String name);

// Clean returns the shortest path name equivalent to path
// by purely lexical processing. It applies the following rules
// iteratively until no further processing can be done:
//
//  1. Replace multiple slashes with a single slash.
//  2. Eliminate each . path name element (the current directory).
//  3. Eliminate each inner .. path name element (the parent directory)
//     along with the non-.. element that precedes it.
//  4. Eliminate .. elements that begin a rooted path:
//     that is, replace "/.." by "/" at the beginning of a path.
//
// The returned path ends in a slash only if it is the root "/".
//
// If the result of this process is an empty string, Clean
// returns the string ".".
//
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String path_Clean(mem_Allocator a, so_String path);

// Split splits path immediately following the final slash,
// separating it into a directory and file name component.
// If there is no slash in path, Split returns an empty dir and
// file set to path.
//
// The returned values have the property that path = dir+file.
// Both returned values are views into the original path.
so_R_str_str path_Split(so_String path);

// Join joins any number of path elements into a single path,
// separating them with slashes. Empty elements are ignored.
// The result is Cleaned. However, if the argument list is
// empty or all its elements are empty, Join returns
// an empty string.
so_String path_Join(mem_Allocator a, so_Slice elem);

// Ext returns the file name extension used by path.
// The extension is the suffix beginning at the final dot
// in the final slash-separated element of path;
// it is empty if there is no dot.
//
// The returned string is a view into the original path.
so_String path_Ext(so_String path);

// Base returns the last element of path.
// Trailing slashes are removed before extracting the last element.
// If the path is empty, Base returns ".".
// If the path consists entirely of slashes, Base returns "/".
//
// The returned string is a view into the original path.
so_String path_Base(so_String path);

// IsAbs reports whether the path is absolute.
bool path_IsAbs(so_String path);

// Dir returns all but the last element of path, typically the path's directory.
// After dropping the final element using [Split], the path is Cleaned and trailing
// slashes are removed.
// If the path is empty, Dir returns ".".
// If the path consists entirely of slashes followed by non-slash bytes, Dir
// returns a single slash. In any other case, the returned path does not end in a
// slash.
//
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String path_Dir(mem_Allocator a, so_String path);
