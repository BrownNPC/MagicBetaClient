#include "path.h"

// -- Types --

typedef struct scanResult scanResult;
typedef struct matchResult matchResult;
typedef struct escResult escResult;
typedef struct lazybuf lazybuf;

typedef struct scanResult {
    bool star;
    so_String chunk;
    so_String rest;
} scanResult;

typedef struct matchResult {
    so_String rest;
    bool ok;
    so_Error err;
} matchResult;

typedef struct escResult {
    so_rune r;
    so_String nchunk;
    so_Error err;
} escResult;

// A lazybuf is a lazily constructed path buffer.
// It supports append, reading previously appended bytes,
// and retrieving the final string. It does not allocate a buffer
// to hold the output until that output diverges from s.
typedef struct lazybuf {
    mem_Allocator a;
    so_String s;
    so_Slice buf;
    so_int w;
} lazybuf;

// -- Forward declarations --
static scanResult scanChunk(so_String pattern);
static matchResult matchChunk(so_String chunk, so_String s);
static escResult getEsc(so_String chunk);
static so_byte lazybuf_index(void* self, so_int i);
static void lazybuf_append(void* self, so_byte c);
static so_String lazybuf_string(void* self);
static void lazybuf_free(void* self);

// -- Variables and constants --

// ErrBadPattern indicates a pattern was malformed.
so_Error path_ErrBadPattern = errors_New("path: syntax error in pattern");

// -- match.go --

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
so_R_bool_err path_Match(so_String pattern, so_String name) {
    for (; so_len(pattern) > 0;) {
        scanResult scan = scanChunk(pattern);
        bool star = scan.star;
        so_String chunk = scan.chunk;
        pattern = scan.rest;
        if (star && so_string_eq(chunk, so_str(""))) {
            // Trailing * matches rest of string unless it has a /.
            bool matched = bytealg_IndexByteString(name, '/') < 0;
            return (so_R_bool_err){.val = matched, .err = (so_Error){0}};
        }
        // Look for match at current position.
        matchResult match = matchChunk(chunk, name);
        // if we're the last chunk, make sure we've exhausted the name
        // otherwise we'll give a false result even if we could still match
        // using the star
        if (match.ok && (so_len(match.rest) == 0 || so_len(pattern) > 0)) {
            name = match.rest;
            continue;
        }
        if (match.err.self != NULL) {
            return (so_R_bool_err){.val = false, .err = match.err};
        }
        if (star) {
            // Look for match skipping i+1 bytes.
            // Cannot skip /.
            bool matched = false;
            for (so_int i = 0; i < so_len(name) && so_at(so_byte, name, i) != '/'; i++) {
                matchResult match = matchChunk(chunk, so_string_slice(name, i + 1, name.len));
                if (match.ok) {
                    // if we're the last chunk, make sure we exhausted the name
                    if (so_len(pattern) == 0 && so_len(match.rest) > 0) {
                        continue;
                    }
                    name = match.rest;
                    matched = true;
                    break;
                }
                if (match.err.self != NULL) {
                    return (so_R_bool_err){.val = false, .err = match.err};
                }
            }
            if (matched) {
                continue;
            }
        }
        // Before returning false with no error,
        // check that the remainder of the pattern is syntactically valid.
        for (; so_len(pattern) > 0;) {
            scanResult scan = scanChunk(pattern);
            {
                matchResult match = matchChunk(scan.chunk, so_str(""));
                if (match.err.self != NULL) {
                    return (so_R_bool_err){.val = false, .err = match.err};
                }
            }
            pattern = scan.rest;
        }
        return (so_R_bool_err){.val = false, .err = (so_Error){0}};
    }
    return (so_R_bool_err){.val = so_len(name) == 0, .err = (so_Error){0}};
}

// scanChunk gets the next segment of pattern, which is a non-star string
// possibly preceded by a star.
static scanResult scanChunk(so_String pattern) {
    scanResult res = {0};
    for (; so_len(pattern) > 0 && so_at(so_byte, pattern, 0) == '*';) {
        pattern = so_string_slice(pattern, 1, pattern.len);
        res.star = true;
    }
    bool inrange = false;
    for (so_int i = 0; i < so_len(pattern); i++) {
        if (so_at(so_byte, pattern, i) == ('\\')) {
            // error check handled in matchChunk: bad pattern.
            if (i + 1 < so_len(pattern)) {
                i++;
            }
        } else if (so_at(so_byte, pattern, i) == ('[')) {
            inrange = true;
        } else if (so_at(so_byte, pattern, i) == (']')) {
            inrange = false;
        } else if (so_at(so_byte, pattern, i) == ('*')) {
            if (!inrange) {
                res.chunk = so_string_slice(pattern, 0, i);
                res.rest = so_string_slice(pattern, i, pattern.len);
                return res;
            }
        }
    }
    res.chunk = pattern;
    return res;
}

// matchChunk checks whether chunk matches the beginning of s.
// If so, it returns the remainder of s (after the match).
// Chunk is all single-character operators: literals, char classes, and ?.
static matchResult matchChunk(so_String chunk, so_String s) {
    // failed records whether the match has failed.
    // After the match fails, the loop continues on processing chunk,
    // checking that the pattern is well-formed but no longer reading s.
    bool failed = false;
    for (; so_len(chunk) > 0;) {
        failed = failed || so_len(s) == 0;
        if (so_at(so_byte, chunk, 0) == ('[')) {
            // character class
            so_rune r = 0;
            if (!failed) {
                so_int n = 0;
                so_R_rune_int _res1 = utf8_DecodeRuneInString(s);
                r = _res1.val;
                n = _res1.val2;
                s = so_string_slice(s, n, s.len);
            }
            chunk = so_string_slice(chunk, 1, chunk.len);
            // possibly negated
            bool negated = false;
            if (so_len(chunk) > 0 && so_at(so_byte, chunk, 0) == '^') {
                negated = true;
                chunk = so_string_slice(chunk, 1, chunk.len);
            }
            // parse all ranges
            bool match = false;
            so_int nrange = 0;
            for (;;) {
                if (so_len(chunk) > 0 && so_at(so_byte, chunk, 0) == ']' && nrange > 0) {
                    chunk = so_string_slice(chunk, 1, chunk.len);
                    break;
                }
                so_rune lo = 0, hi = 0;
                escResult esc = getEsc(chunk);
                if (esc.err.self != NULL) {
                    return (matchResult){so_str(""), false, esc.err};
                }
                lo = esc.r;
                chunk = esc.nchunk;
                hi = lo;
                if (so_len(chunk) > 0 && so_at(so_byte, chunk, 0) == '-') {
                    escResult esc = getEsc(so_string_slice(chunk, 1, chunk.len));
                    if (esc.err.self != NULL) {
                        return (matchResult){so_str(""), false, esc.err};
                    }
                    hi = esc.r;
                    chunk = esc.nchunk;
                }
                match = match || (lo <= r && r <= hi);
                nrange++;
            }
            failed = failed || match == negated;
        } else if (so_at(so_byte, chunk, 0) == ('?')) {
            if (!failed) {
                failed = so_at(so_byte, s, 0) == '/';
                so_R_rune_int _res2 = utf8_DecodeRuneInString(s);
                so_int n = _res2.val2;
                s = so_string_slice(s, n, s.len);
            }
            chunk = so_string_slice(chunk, 1, chunk.len);
        } else if (so_at(so_byte, chunk, 0) == ('\\')) {
            chunk = so_string_slice(chunk, 1, chunk.len);
            if (so_len(chunk) == 0) {
                return (matchResult){so_str(""), false, path_ErrBadPattern};
            }
            if (!failed) {
                failed = so_at(so_byte, chunk, 0) != so_at(so_byte, s, 0);
                s = so_string_slice(s, 1, s.len);
            }
            chunk = so_string_slice(chunk, 1, chunk.len);
        } else {
            if (!failed) {
                failed = so_at(so_byte, chunk, 0) != so_at(so_byte, s, 0);
                s = so_string_slice(s, 1, s.len);
            }
            chunk = so_string_slice(chunk, 1, chunk.len);
        }
    }
    if (failed) {
        return (matchResult){so_str(""), false, (so_Error){0}};
    }
    return (matchResult){s, true, (so_Error){0}};
}

// getEsc gets a possibly-escaped character from chunk, for a character class.
static escResult getEsc(so_String chunk) {
    escResult res = {0};
    if (so_len(chunk) == 0 || so_at(so_byte, chunk, 0) == '-' || so_at(so_byte, chunk, 0) == ']') {
        res.err = path_ErrBadPattern;
        return res;
    }
    if (so_at(so_byte, chunk, 0) == '\\') {
        chunk = so_string_slice(chunk, 1, chunk.len);
        if (so_len(chunk) == 0) {
            res.err = path_ErrBadPattern;
            return res;
        }
    }
    so_R_rune_int _res1 = utf8_DecodeRuneInString(chunk);
    so_rune r = _res1.val;
    so_int n = _res1.val2;
    if (r == utf8_RuneError && n == 1) {
        res.err = path_ErrBadPattern;
    }
    res.r = r;
    res.nchunk = so_string_slice(chunk, n, chunk.len);
    if (so_len(res.nchunk) == 0) {
        res.err = path_ErrBadPattern;
    }
    return res;
}

// -- path.go --

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
so_String path_Clean(mem_Allocator a, so_String path) {
    if (so_string_eq(path, so_str(""))) {
        return stringslite_Clone(a, so_str("."));
    }
    bool rooted = so_at(so_byte, path, 0) == '/';
    so_int n = so_len(path);
    // Invariants:
    //	reading from path; r is index of next byte to process.
    //	writing to buf; w is index of next byte to write.
    //	dotdot is index in buf where .. must stop, either because
    //		it is the leading slash or it is a leading ../../.. prefix.
    lazybuf out = (lazybuf){.a = a, .s = path};
    so_int r = 0, dotdot = 0;
    if (rooted) {
        lazybuf_append(&out, '/');
        r = 1;
        dotdot = 1;
    }
    for (; r < n;) {
        if (so_at(so_byte, path, r) == '/') {
            // empty path element
            r++;
        } else if (so_at(so_byte, path, r) == '.' && (r + 1 == n || so_at(so_byte, path, r + 1) == '/')) {
            // . element
            r++;
        } else if (so_at(so_byte, path, r) == '.' && so_at(so_byte, path, r + 1) == '.' && (r + 2 == n || so_at(so_byte, path, r + 2) == '/')) {
            // .. element: remove to last /
            r += 2;
            if (out.w > dotdot) {
                // can backtrack
                out.w--;
                for (; out.w > dotdot && lazybuf_index(&out, out.w) != '/';) {
                    out.w--;
                }
            } else if (!rooted) {
                // cannot backtrack, but not rooted, so append .. element.
                if (out.w > 0) {
                    lazybuf_append(&out, '/');
                }
                lazybuf_append(&out, '.');
                lazybuf_append(&out, '.');
                dotdot = out.w;
            }
        } else {
            // real path element.
            // add slash if needed
            if ((rooted && out.w != 1) || (!rooted && out.w != 0)) {
                lazybuf_append(&out, '/');
            }
            // copy element
            for (; r < n && so_at(so_byte, path, r) != '/'; r++) {
                lazybuf_append(&out, so_at(so_byte, path, r));
            }
        }
    }
    // Turn empty string into "."
    if (out.w == 0) {
        lazybuf_free(&out);
        return stringslite_Clone(a, so_str("."));
    }
    return lazybuf_string(&out);
}

// Split splits path immediately following the final slash,
// separating it into a directory and file name component.
// If there is no slash in path, Split returns an empty dir and
// file set to path.
//
// The returned values have the property that path = dir+file.
// Both returned values are views into the original path.
so_R_str_str path_Split(so_String path) {
    so_int i = bytealg_LastIndexByteString(path, '/');
    return (so_R_str_str){.val = so_string_slice(path, 0, i + 1), .val2 = so_string_slice(path, i + 1, path.len)};
}

// Join joins any number of path elements into a single path,
// separating them with slashes. Empty elements are ignored.
// The result is Cleaned. However, if the argument list is
// empty or all its elements are empty, Join returns
// an empty string.
so_String path_Join(mem_Allocator a, so_Slice elem) {
    so_int size = 0;
    for (so_int _ = 0; _ < so_len(elem); _++) {
        so_String e = so_at(so_String, elem, _);
        size += so_len(e);
    }
    if (size == 0) {
        return so_str("");
    }
    so_Slice buf = mem_AllocSlice(so_byte, (a), (0), (size + so_len(elem) - 1));
    for (so_int _ = 0; _ < so_len(elem); _++) {
        so_String e = so_at(so_String, elem, _);
        if (so_len(buf) > 0 || so_string_ne(e, so_str(""))) {
            if (so_len(buf) > 0) {
                buf = so_append(so_byte, buf, '/');
            }
            buf = so_extend(so_byte, buf, so_string_bytes(e));
        }
    }
    so_String path = path_Clean(a, so_bytes_string(buf));
    mem_FreeSlice(so_byte, (a), (buf));
    return path;
}

// Ext returns the file name extension used by path.
// The extension is the suffix beginning at the final dot
// in the final slash-separated element of path;
// it is empty if there is no dot.
//
// The returned string is a view into the original path.
so_String path_Ext(so_String path) {
    for (so_int i = so_len(path) - 1; i >= 0 && so_at(so_byte, path, i) != '/'; i--) {
        if (so_at(so_byte, path, i) == '.') {
            return so_string_slice(path, i, path.len);
        }
    }
    return so_str("");
}

// Base returns the last element of path.
// Trailing slashes are removed before extracting the last element.
// If the path is empty, Base returns ".".
// If the path consists entirely of slashes, Base returns "/".
//
// The returned string is a view into the original path.
so_String path_Base(so_String path) {
    if (so_string_eq(path, so_str(""))) {
        return so_str(".");
    }
    // Strip trailing slashes.
    for (; so_len(path) > 0 && so_at(so_byte, path, so_len(path) - 1) == '/';) {
        path = so_string_slice(path, 0, so_len(path) - 1);
    }
    // Find the last element
    {
        so_int i = bytealg_LastIndexByteString(path, '/');
        if (i >= 0) {
            path = so_string_slice(path, i + 1, path.len);
        }
    }
    // If empty now, it had only slashes.
    if (so_string_eq(path, so_str(""))) {
        return so_str("/");
    }
    return path;
}

// IsAbs reports whether the path is absolute.
bool path_IsAbs(so_String path) {
    return so_len(path) > 0 && so_at(so_byte, path, 0) == '/';
}

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
so_String path_Dir(mem_Allocator a, so_String path) {
    so_R_str_str _res1 = path_Split(path);
    so_String dir = _res1.val;
    return path_Clean(a, dir);
}

static so_byte lazybuf_index(void* self, so_int i) {
    lazybuf* b = self;
    if (b->buf.ptr != NULL) {
        return so_at(so_byte, b->buf, i);
    }
    return so_at(so_byte, b->s, i);
}

static void lazybuf_append(void* self, so_byte c) {
    lazybuf* b = self;
    if (b->buf.ptr == NULL) {
        if (b->w < so_len(b->s) && so_at(so_byte, b->s, b->w) == c) {
            b->w++;
            return;
        }
        so_int n = so_len(b->s);
        b->buf = mem_AllocSlice(so_byte, (b->a), (n), (n));
        so_copy_string(b->buf, so_string_slice(b->s, 0, b->w));
    }
    so_at(so_byte, b->buf, b->w) = c;
    b->w++;
}

static so_String lazybuf_string(void* self) {
    lazybuf* b = self;
    if (b->buf.ptr == NULL) {
        return stringslite_Clone(b->a, so_string_slice(b->s, 0, b->w));
    }
    return so_bytes_string(so_slice(so_byte, b->buf, 0, b->w));
}

static void lazybuf_free(void* self) {
    lazybuf* b = self;
    if (b->buf.ptr == NULL) {
        return;
    }
    mem_FreeSlice(so_byte, (b->a), (b->buf));
    b->buf = (so_Slice){0};
}
