#include "bytes.h"

// -- Types --

typedef struct asciiSet asciiSet;

// asciiSet is a 32-byte value, where each bit represents the presence of a
// given ASCII character in the set. The 128-bits of the lower 16 bytes,
// starting with the least-significant bit of the lowest word to the
// most-significant bit of the highest word, map to the full range of all
// 128 ASCII characters. The 128-bits of the upper 16 bytes will be zeroed,
// ensuring that any non-ASCII character will be reported as not in the set.
// This allocates a total of 32 bytes even though the upper half
// is unused to avoid bounds checks in asciiSet.contains.
typedef struct asciiSet {
    uint32_t val[8];
    bool ok;
} asciiSet;

// -- Forward declarations --
static bool bytes_Buffer_empty(void* self);
static so_R_int_bool bytes_Buffer_tryGrowByReslice(void* self, so_int n);
static so_int bytes_Buffer_grow(void* self, so_int n);
static void bytes_Buffer_growBuf(void* self, so_int m, so_int n);
static so_R_slice_err bytes_Buffer_readSlice(void* self, so_byte delim);
static so_Slice genSplit(mem_Allocator a, so_Slice s, so_Slice sep, so_int sepSave, so_int n);
static so_Slice explode(mem_Allocator a, so_Slice s, so_int n);
static bool asciiSet_contains(void* self, so_byte c);
static so_Slice trimLeftASCII(so_Slice s, asciiSet* as);
static so_Slice trimLeftByte(so_Slice s, so_byte c);
static so_Slice trimLeftFunc(so_Slice s, bytes_RunePredicate f);
static so_Slice trimLeftUnicode(so_Slice s, so_String cutset);
static so_Slice trimRightASCII(so_Slice s, asciiSet* as);
static so_Slice trimRightByte(so_Slice s, so_byte c);
static so_Slice trimRightFunc(so_Slice s, bytes_RunePredicate f);
static so_Slice trimRightUnicode(so_Slice s, so_String cutset);
static bool containsRune(so_String s, so_rune r);
static so_int indexFunc(so_Slice s, bytes_RunePredicate f, bool truth);
static so_int lastIndexFunc(so_Slice s, bytes_RunePredicate f, bool truth);
static asciiSet makeASCIISet(so_String chars);

// -- Variables and constants --

// ErrNegativeGrow means that a Buffer.Grow call was given a negative count.
so_Error bytes_ErrNegativeGrow = errors_New("bytes: negative grow");

// smallBufferSize is an initial allocation minimal capacity.
static const int64_t smallBufferSize = 64;

// maxInt is the maximum value of an int.
static const so_int maxInt = (so_int)((uint64_t)(~(so_uint)(0)) >> 1);
static uint8_t asciiSpace[256] = {[U'\t'] = 1, [U'\n'] = 1, [U'\v'] = 1, [U'\f'] = 1, [U'\r'] = 1, [U' '] = 1};

// -- buffer.go --

// Bytes returns a slice of length b.Len() holding the unread portion of the buffer.
// The slice is valid for use only until the next buffer modification (that is,
// only until the next call to a method like [Buffer.Read], [Buffer.Write], [Buffer.Reset].
// The slice aliases the buffer content at least until the next buffer modification,
// so immediate changes to the slice will affect the result of future reads.
so_Slice bytes_Buffer_Bytes(void* self) {
    bytes_Buffer* b = self;
    return so_slice(so_byte, b->buf, b->off, b->buf.len);
}

// String returns the contents of the unread portion of the buffer
// as a string. If the [Buffer] is a nil pointer, it returns "<nil>".
// The string is valid for use only until the next buffer modification.
//
// To build strings more efficiently, see the [strings.Builder] type.
so_String bytes_Buffer_String(void* self) {
    bytes_Buffer* b = self;
    if (b == NULL) {
        // Special case, useful in debugging.
        return so_str("<nil>");
    }
    return so_bytes_string(so_slice(so_byte, b->buf, b->off, b->buf.len));
}

// Peek returns the next n bytes without advancing the buffer.
// If Peek returns fewer than n bytes, it also returns [io.EOF].
// The slice is only valid until the next call to a read or write method.
// The slice aliases the buffer content at least until the next buffer modification,
// so immediate changes to the slice will affect the result of future reads.
so_R_slice_err bytes_Buffer_Peek(void* self, so_int n) {
    bytes_Buffer* b = self;
    if (bytes_Buffer_Len(b) < n) {
        return (so_R_slice_err){.val = so_slice(so_byte, b->buf, b->off, b->buf.len), .err = io_EOF};
    }
    return (so_R_slice_err){.val = so_slice(so_byte, b->buf, b->off, b->off + n), .err = (so_Error){0}};
}

// empty reports whether the unread portion of the buffer is empty.
static bool bytes_Buffer_empty(void* self) {
    bytes_Buffer* b = self;
    return so_len(b->buf) <= b->off;
}

// Len returns the number of bytes of the unread portion of the buffer;
// b.Len() == len(b.Bytes()).
so_int bytes_Buffer_Len(void* self) {
    bytes_Buffer* b = self;
    return so_len(b->buf) - b->off;
}

// Cap returns the capacity of the buffer's underlying byte slice, that is, the
// total space allocated for the buffer's data.
so_int bytes_Buffer_Cap(void* self) {
    bytes_Buffer* b = self;
    return so_cap(b->buf);
}

// Available returns how many bytes are unused in the buffer.
so_int bytes_Buffer_Available(void* self) {
    bytes_Buffer* b = self;
    return so_cap(b->buf) - so_len(b->buf);
}

// Reset resets the buffer to be empty,
// but it retains the underlying storage for use by future writes.
void bytes_Buffer_Reset(void* self) {
    bytes_Buffer* b = self;
    b->buf = so_slice(so_byte, b->buf, 0, 0);
    b->off = 0;
}

// Free frees the internal buffer and resets the buffer.
// After Free, the buffer can be reused with new writes.
void bytes_Buffer_Free(void* self) {
    bytes_Buffer* b = self;
    mem_FreeSlice(so_byte, (b->a), (b->buf));
    b->buf = (so_Slice){0};
    b->off = 0;
}

// tryGrowByReslice is an inlineable version of grow for the fast-case where the
// internal buffer only needs to be resliced.
// It returns the index where bytes should be written and whether it succeeded.
static so_R_int_bool bytes_Buffer_tryGrowByReslice(void* self, so_int n) {
    bytes_Buffer* b = self;
    {
        so_int l = so_len(b->buf);
        if (n <= so_cap(b->buf) - l) {
            b->buf = so_slice(so_byte, b->buf, 0, l + n);
            return (so_R_int_bool){.val = l, .val2 = true};
        }
    }
    return (so_R_int_bool){.val = 0, .val2 = false};
}

// grow grows the buffer to guarantee space for n more bytes.
// It returns the index where bytes should be written.
// Panics if the buffer can't grow.
static so_int bytes_Buffer_grow(void* self, so_int n) {
    bytes_Buffer* b = self;
    so_int m = bytes_Buffer_Len(b);
    // If buffer is empty, reset to recover space.
    if (m == 0 && b->off != 0) {
        bytes_Buffer_Reset(b);
    }
    // Try to grow by means of a reslice.
    {
        so_R_int_bool _res1 = bytes_Buffer_tryGrowByReslice(b, n);
        so_int i = _res1.val;
        bool ok = _res1.val2;
        if (ok) {
            return i;
        }
    }
    if (b->buf.ptr == NULL && n <= smallBufferSize) {
        b->buf = mem_AllocSlice(so_byte, (b->a), (n), (smallBufferSize));
        return 0;
    }
    so_int c = so_cap(b->buf);
    if (n <= c / 2 - m) {
        // We can slide things down instead of allocating a new
        // slice. We only need m+n <= c to slide, but
        // we instead let capacity get twice as large so we
        // don't spend all our time copying.
        so_copy(so_byte, b->buf, so_slice(so_byte, b->buf, b->off, b->buf.len));
    } else if (c > maxInt - c - n) {
        so_panic("bytes: buffer overflow");
    } else {
        // Allocate a new buffer, copy live data, free the old one.
        bytes_Buffer_growBuf(b, m, n);
    }
    // Restore b.off and len(b.buf).
    b->off = 0;
    b->buf = so_slice(so_byte, b->buf, 0, m + n);
    return m;
}

// growBuf allocates a new buffer with enough room for m+n bytes (where m is
// the live data length), copies the live portion (buf[off:]), and frees the
// old buffer.
static void bytes_Buffer_growBuf(void* self, so_int m, so_int n) {
    bytes_Buffer* b = self;
    // ensure enough space for n elements
    so_int c = m + n;
    // but double if it's less than double
    c = so_max(c, 2 * so_cap(b->buf));
    so_Slice buf = mem_AllocSlice(so_byte, (b->a), (c), (c));
    so_copy(so_byte, buf, so_slice(so_byte, b->buf, b->off, b->buf.len));
    mem_FreeSlice(so_byte, (b->a), (b->buf));
    b->buf = so_slice(so_byte, buf, 0, m);
}

// Grow grows the buffer's capacity, if necessary, to guarantee space for
// another n bytes. After Grow(n), at least n bytes can be written to the
// buffer without another allocation.
// Panics if n is negative or if the buffer cannot grow.
void bytes_Buffer_Grow(void* self, so_int n) {
    bytes_Buffer* b = self;
    if (n < 0) {
        so_panic("bytes: negative grow");
    }
    so_int m = bytes_Buffer_grow(b, n);
    b->buf = so_slice(so_byte, b->buf, 0, m);
}

// Write appends the contents of p to the buffer, growing the buffer as
// needed. The return value n is the length of p; err is always nil.
// Panics if the buffer becomes too large.
so_R_int_err bytes_Buffer_Write(void* self, so_Slice p) {
    bytes_Buffer* b = self;
    so_R_int_bool _res1 = bytes_Buffer_tryGrowByReslice(b, so_len(p));
    so_int m = _res1.val;
    bool ok = _res1.val2;
    if (!ok) {
        m = bytes_Buffer_grow(b, so_len(p));
    }
    return (so_R_int_err){.val = so_copy(so_byte, so_slice(so_byte, b->buf, m, b->buf.len), p), .err = (so_Error){0}};
}

// WriteString appends the contents of s to the buffer, growing the buffer as
// needed. The return value n is the length of s; err is always nil.
// Panics if the buffer becomes too large.
so_R_int_err bytes_Buffer_WriteString(void* self, so_String s) {
    bytes_Buffer* b = self;
    so_R_int_bool _res1 = bytes_Buffer_tryGrowByReslice(b, so_len(s));
    so_int m = _res1.val;
    bool ok = _res1.val2;
    if (!ok) {
        m = bytes_Buffer_grow(b, so_len(s));
    }
    return (so_R_int_err){.val = so_copy_string(so_slice(so_byte, b->buf, m, b->buf.len), s), .err = (so_Error){0}};
}

// ReadFrom reads data from r until EOF and appends it to the buffer, growing
// the buffer as needed. The return value n is the number of bytes read. Any
// error except io.EOF encountered during the read is also returned.
// Panics if the buffer becomes too large.
so_R_i64_err bytes_Buffer_ReadFrom(void* self, io_Reader r) {
    bytes_Buffer* b = self;
    int64_t n = 0;
    for (;;) {
        so_int i = bytes_Buffer_grow(b, bytes_MinRead);
        b->buf = so_slice(so_byte, b->buf, 0, i);
        so_R_int_err _res1 = r.Read(r.self, so_slice(so_byte, b->buf, i, so_cap(b->buf)));
        so_int m = _res1.val;
        so_Error err = _res1.err;
        if (m < 0) {
            return (so_R_i64_err){.val = 0, .err = io_ErrNegativeRead};
        }
        b->buf = so_slice(so_byte, b->buf, 0, i + m);
        n += (int64_t)(m);
        if (err.self == io_EOF.self) {
            // err is EOF, so return nil explicitly
            return (so_R_i64_err){.val = n, .err = (so_Error){0}};
        }
        if (err.self != NULL) {
            return (so_R_i64_err){.val = n, .err = err};
        }
    }
}

// WriteTo writes data to w until the buffer is drained or an error occurs.
// The return value n is the number of bytes written; it always fits into an
// int, but it is int64 to match the [io.WriterTo] interface. Any error
// encountered during the write is also returned.
so_R_i64_err bytes_Buffer_WriteTo(void* self, io_Writer w) {
    bytes_Buffer* b = self;
    int64_t n = 0;
    {
        so_int nBytes = bytes_Buffer_Len(b);
        if (nBytes > 0) {
            so_R_int_err _res1 = w.Write(w.self, so_slice(so_byte, b->buf, b->off, b->buf.len));
            so_int m = _res1.val;
            so_Error err = _res1.err;
            if (m > nBytes) {
                return (so_R_i64_err){.val = n, .err = io_ErrInvalidWrite};
            }
            b->off += m;
            n = (int64_t)(m);
            if (err.self != NULL) {
                return (so_R_i64_err){.val = n, .err = err};
            }
            // all bytes should have been written, by definition of
            // Write method in io.Writer
            if (m != nBytes) {
                return (so_R_i64_err){.val = n, .err = io_ErrShortWrite};
            }
        }
    }
    // Buffer is now empty; reset.
    bytes_Buffer_Reset(b);
    return (so_R_i64_err){.val = n, .err = (so_Error){0}};
}

// WriteByte appends the byte c to the buffer, growing the buffer as needed.
// The returned error is always nil, but is included to match [bufio.Writer]'s
// WriteByte. Panics if the buffer becomes too large.
so_Error bytes_Buffer_WriteByte(void* self, so_byte c) {
    bytes_Buffer* b = self;
    so_R_int_bool _res1 = bytes_Buffer_tryGrowByReslice(b, 1);
    so_int m = _res1.val;
    bool ok = _res1.val2;
    if (!ok) {
        m = bytes_Buffer_grow(b, 1);
    }
    so_at(so_byte, b->buf, m) = c;
    return (so_Error){0};
}

// WriteRune appends the UTF-8 encoding of Unicode code point r to the
// buffer, returning its length and an error, which is always nil but is
// included to match [bufio.Writer]'s WriteRune. The buffer is grown as needed;
// if it becomes too large, WriteRune will panic.
so_R_int_err bytes_Buffer_WriteRune(void* self, so_rune r) {
    bytes_Buffer* b = self;
    // Compare as uint32 to correctly handle negative runes.
    if ((uint32_t)(r) < utf8_RuneSelf) {
        bytes_Buffer_WriteByte(b, (so_byte)(r));
        return (so_R_int_err){.val = 1, .err = (so_Error){0}};
    }
    so_R_int_bool _res1 = bytes_Buffer_tryGrowByReslice(b, utf8_UTFMax);
    so_int m = _res1.val;
    bool ok = _res1.val2;
    if (!ok) {
        m = bytes_Buffer_grow(b, utf8_UTFMax);
    }
    b->buf = utf8_AppendRune(so_slice(so_byte, b->buf, 0, m), r);
    return (so_R_int_err){.val = so_len(b->buf) - m, .err = (so_Error){0}};
}

// Read reads the next len(p) bytes from the buffer or until the buffer
// is drained. The return value n is the number of bytes read. If the
// buffer has no data to return, err is [io.EOF] (unless len(p) is zero);
// otherwise it is nil.
so_R_int_err bytes_Buffer_Read(void* self, so_Slice p) {
    bytes_Buffer* b = self;
    if (bytes_Buffer_empty(b)) {
        // Buffer is empty, reset to recover space.
        bytes_Buffer_Reset(b);
        if (so_len(p) == 0) {
            return (so_R_int_err){.val = 0, .err = (so_Error){0}};
        }
        return (so_R_int_err){.val = 0, .err = io_EOF};
    }
    so_int n = so_copy(so_byte, p, so_slice(so_byte, b->buf, b->off, b->buf.len));
    b->off += n;
    return (so_R_int_err){.val = n, .err = (so_Error){0}};
}

// Next returns a slice containing the next n bytes from the buffer,
// advancing the buffer as if the bytes had been returned by [Buffer.Read].
// If there are fewer than n bytes in the buffer, Next returns the entire buffer.
// The slice is only valid until the next call to a read or write method.
so_Slice bytes_Buffer_Next(void* self, so_int n) {
    bytes_Buffer* b = self;
    so_int m = bytes_Buffer_Len(b);
    if (n > m) {
        n = m;
    }
    so_Slice data = so_slice(so_byte, b->buf, b->off, b->off + n);
    b->off += n;
    return data;
}

// ReadByte reads and returns the next byte from the buffer.
// If no byte is available, it returns error [io.EOF].
so_R_byte_err bytes_Buffer_ReadByte(void* self) {
    bytes_Buffer* b = self;
    if (bytes_Buffer_empty(b)) {
        // Buffer is empty, reset to recover space.
        bytes_Buffer_Reset(b);
        return (so_R_byte_err){.val = 0, .err = io_EOF};
    }
    so_byte c = so_at(so_byte, b->buf, b->off);
    b->off++;
    return (so_R_byte_err){.val = c, .err = (so_Error){0}};
}

// ReadRune reads and returns the next UTF-8-encoded
// Unicode code point from the buffer.
// If no bytes are available, the error returned is io.EOF.
// If the bytes are an erroneous UTF-8 encoding, it
// consumes one byte and returns U+FFFD, 1.
io_RuneSizeResult bytes_Buffer_ReadRune(void* self) {
    bytes_Buffer* b = self;
    if (bytes_Buffer_empty(b)) {
        // Buffer is empty, reset to recover space.
        bytes_Buffer_Reset(b);
        return (io_RuneSizeResult){.Rune = 0, .Size = 0, .Err = io_EOF};
    }
    so_byte c = so_at(so_byte, b->buf, b->off);
    if (c < utf8_RuneSelf) {
        b->off++;
        return (io_RuneSizeResult){.Rune = (so_rune)(c), .Size = 1, .Err = (so_Error){0}};
    }
    so_R_rune_int _res1 = utf8_DecodeRune(so_slice(so_byte, b->buf, b->off, b->buf.len));
    so_rune r = _res1.val;
    so_int n = _res1.val2;
    b->off += n;
    return (io_RuneSizeResult){.Rune = r, .Size = n, .Err = (so_Error){0}};
}

// ReadBytes reads until the first occurrence of delim in the input,
// returning a slice containing the data up to and including the delimiter.
// If ReadBytes encounters an error before finding a delimiter,
// it returns the data read before the error and the error itself (often [io.EOF]).
// ReadBytes returns err != nil if and only if the returned data does not end in
// delim.
//
// The returned slice is allocated; the caller owns it.
so_R_slice_err bytes_Buffer_ReadBytes(void* self, so_byte delim) {
    bytes_Buffer* b = self;
    so_R_slice_err _res1 = bytes_Buffer_readSlice(b, delim);
    so_Slice slice = _res1.val;
    so_Error err = _res1.err;
    // return a copy of slice. The buffer's backing array may
    // be overwritten by later calls.
    so_Slice line = slices_Clone(so_byte, (b->a), (slice));
    return (so_R_slice_err){.val = line, .err = err};
}

// readSlice is like ReadBytes but returns a reference to internal buffer data.
static so_R_slice_err bytes_Buffer_readSlice(void* self, so_byte delim) {
    bytes_Buffer* b = self;
    so_Error err = {0};
    so_int i = bytes_IndexByte(so_slice(so_byte, b->buf, b->off, b->buf.len), delim);
    so_int end = b->off + i + 1;
    if (i < 0) {
        end = so_len(b->buf);
        err = io_EOF;
    }
    so_Slice line = {0};
    line = so_slice(so_byte, b->buf, b->off, end);
    b->off = end;
    return (so_R_slice_err){.val = line, .err = err};
}

// ReadString reads until the first occurrence of delim in the input,
// returning a string containing the data up to and including the delimiter.
// If ReadString encounters an error before finding a delimiter,
// it returns the data read before the error and the error itself (often [io.EOF]).
// ReadString returns err != nil if and only if the returned data does not end
// in delim.
//
// The returned string is allocated; the caller owns it.
so_R_str_err bytes_Buffer_ReadString(void* self, so_byte delim) {
    bytes_Buffer* b = self;
    so_R_slice_err _res1 = bytes_Buffer_readSlice(b, delim);
    so_Slice slice = _res1.val;
    so_Error err = _res1.err;
    return (so_R_str_err){.val = bytes_String(b->a, slice), .err = err};
}

// NewBuffer creates and initializes a new [Buffer] using buf as its
// initial contents. The new Buffer takes ownership of buf, and the
// caller should not use buf after this call. NewBuffer is intended to
// prepare a Buffer to read existing data. It can also be used to set
// the initial size of the internal buffer for writing. To do that,
// buf should have the desired capacity but a length of zero.
//
// If buf was allocated with an allocator, the same allocator must be
// passed to NewBuffer so that [Buffer.Free] can release it correctly.
// Do not call [Buffer.Free] if buf was not heap-allocated.
//
// Do not provide a stack-allocated buf if you intend to write to the buffer,
// as the buffer may need to grow and reallocate, which would cause free()
// on a stack pointer. Only use heap-allocated slices in this case.
//
// If the allocator is nil, uses the system allocator.
bytes_Buffer bytes_NewBuffer(mem_Allocator a, so_Slice buf) {
    return (bytes_Buffer){.a = a, .buf = buf};
}

// NewBufferString creates and initializes a new [Buffer] using string s as its
// initial contents. It is intended to prepare a buffer to read an existing string.
//
// If s was allocated with an allocator, the same allocator must be
// passed to NewBuffer so that [Buffer.Free] can release it correctly.
// Do not call [Buffer.Free] if s was not heap-allocated.
//
// Do not provide a stack-allocated s if you intend to write to the buffer,
// as the buffer may need to grow and reallocate, which would cause free()
// on a stack pointer. Only use heap-allocated strings in this case.
//
// If the allocator is nil, uses the system allocator.
bytes_Buffer bytes_NewBufferString(mem_Allocator a, so_String s) {
    return (bytes_Buffer){.a = a, .buf = so_string_bytes(s)};
}

// -- bytes.go --

// Clone returns a copy of b[:len(b)].
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_Clone(mem_Allocator a, so_Slice b) {
    return slices_Clone(so_byte, (a), (b));
}

// Contains reports whether subslice is within b.
bool bytes_Contains(so_Slice b, so_Slice subslice) {
    return bytes_Index(b, subslice) != -1;
}

// Compare returns an integer comparing two byte slices lexicographically.
// The result will be 0 if a == b, -1 if a < b, and +1 if a > b.
// A nil argument is equivalent to an empty slice.
so_int bytes_Compare(so_Slice a, so_Slice b) {
    return bytealg_Compare(a, b);
}

// Count counts the number of non-overlapping instances of sep in s.
// If sep is an empty slice, Count returns 1 + the number of UTF-8-encoded code points in s.
so_int bytes_Count(so_Slice s, so_Slice sep) {
    // special case
    if (so_len(sep) == 0) {
        return utf8_RuneCount(s) + 1;
    }
    if (so_len(sep) == 1) {
        return bytealg_Count(s, so_at(so_byte, sep, 0));
    }
    so_int n = 0;
    for (;;) {
        so_int i = bytes_Index(s, sep);
        if (i == -1) {
            return n;
        }
        n++;
        s = so_slice(so_byte, s, i + so_len(sep), s.len);
    }
}

// Cut slices s around the first instance of sep,
// returning the text before and after sep.
// The found result reports whether sep appears in s.
// If sep does not appear in s, cut returns s, nil, false.
//
// Cut returns slices of the original slice s, not copies.
bytes_CutResult bytes_Cut(so_Slice s, so_Slice sep) {
    bytes_CutResult res = {0};
    {
        so_int i = bytes_Index(s, sep);
        if (i >= 0) {
            res.Before = so_slice(so_byte, s, 0, i);
            res.After = so_slice(so_byte, s, i + so_len(sep), s.len);
            res.Found = true;
            return res;
        }
    }
    res.Before = s;
    return res;
}

// Equal reports whether a and b
// are the same length and contain the same bytes.
// A nil argument is equivalent to an empty slice.
bool bytes_Equal(so_Slice a, so_Slice b) {
    return so_string_eq(so_bytes_string(a), so_bytes_string(b));
}

// HasPrefix reports whether the byte slice s begins with prefix.
bool bytes_HasPrefix(so_Slice s, so_Slice prefix) {
    return so_len(s) >= so_len(prefix) && bytes_Equal(so_slice(so_byte, s, 0, so_len(prefix)), prefix);
}

// HasSuffix reports whether the byte slice s ends with suffix.
bool bytes_HasSuffix(so_Slice s, so_Slice suffix) {
    return so_len(s) >= so_len(suffix) && bytes_Equal(so_slice(so_byte, s, so_len(s) - so_len(suffix), s.len), suffix);
}

// Index returns the index of the first instance of sep in s, or -1 if sep is not present in s.
so_int bytes_Index(so_Slice s, so_Slice sep) {
    so_int n = so_len(sep);
    if (n == 0) {
        return 0;
    } else if (n == 1) {
        return bytes_IndexByte(s, so_at(so_byte, sep, 0));
    } else if (n == so_len(s)) {
        if (bytes_Equal(sep, s)) {
            return 0;
        }
        return -1;
    } else if (n > so_len(s)) {
        return -1;
    }
    so_byte c0 = so_at(so_byte, sep, 0);
    so_byte c1 = so_at(so_byte, sep, 1);
    so_int i = 0;
    so_int fails = 0;
    so_int t = so_len(s) - n + 1;
    for (; i < t;) {
        if (so_at(so_byte, s, i) != c0) {
            so_int o = bytes_IndexByte(so_slice(so_byte, s, i + 1, t), c0);
            if (o < 0) {
                break;
            }
            i += o + 1;
        }
        if (so_at(so_byte, s, i + 1) == c1 && bytes_Equal(so_slice(so_byte, s, i, i + n), sep)) {
            return i;
        }
        i++;
        fails++;
        if (fails >= (4 + (i >> 4)) && i < t) {
            // Give up on IndexByte, it isn't skipping ahead
            // far enough to be better than Rabin-Karp.
            // Experiments (using IndexPeriodic) suggest
            // the cutover is about 16 byte skips.
            // TODO: if large prefixes of sep are matching
            // we should cutover at even larger average skips,
            // because Equal becomes that much more expensive.
            // This code does not take that effect into account.
            so_int j = bytealg_IndexRabinKarp(so_slice(so_byte, s, i, s.len), sep);
            if (j < 0) {
                return -1;
            }
            return i + j;
        }
    }
    return -1;
}

// IndexByte returns the index of the first instance of c in b, or -1 if c is not present in b.
so_int bytes_IndexByte(so_Slice b, so_byte c) {
    return bytealg_IndexByte(b, c);
}

// Join concatenates the elements of s to create a new byte slice. The separator
// sep is placed between elements in the resulting slice.
// Panics if the result is too large to allocate.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_Join(mem_Allocator a, so_Slice s, so_Slice sep) {
    if (so_len(s) == 0) {
        return (so_Slice){0};
    }
    if (so_len(s) == 1) {
        // Just return a copy.
        return slices_Clone(so_byte, (a), (so_at(so_Slice, s, 0)));
    }
    so_int n = 0;
    if (so_len(sep) > 0) {
        if (so_len(sep) >= maxInt / (so_len(s) - 1)) {
            so_panic("bytes: join separator too large");
        }
        n += so_len(sep) * (so_len(s) - 1);
    }
    for (so_int _ = 0; _ < so_len(s); _++) {
        so_Slice v = so_at(so_Slice, s, _);
        if (so_len(v) > maxInt - n) {
            so_panic("bytes: join overflow");
        }
        n += so_len(v);
    }
    so_Slice b = mem_AllocSlice(so_byte, (a), (n), (n));
    so_int bp = so_copy(so_byte, b, so_at(so_Slice, s, 0));
    for (so_int _ = 0; _ < so_len(so_slice(so_Slice, s, 1, s.len)); _++) {
        so_Slice v = so_at(so_Slice, so_slice(so_Slice, s, 1, s.len), _);
        bp += so_copy(so_byte, so_slice(so_byte, b, bp, b.len), sep);
        bp += so_copy(so_byte, so_slice(so_byte, b, bp, b.len), v);
    }
    return b;
}

// Repeat returns a new byte slice consisting of count copies of b.
// It panics if count is negative or if the result of (len(b) * count) overflows.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_Repeat(mem_Allocator a, so_Slice b, so_int count) {
    if (count == 0) {
        return (so_Slice){0};
    }
    // Since we cannot return an error on overflow,
    // we should panic if the repeat will generate an overflow.
    // See golang.org/issue/16237.
    if (count < 0) {
        so_panic("bytes: negative Repeat count");
    }
    so_R_uint_uint _res1 = bits_Mul((so_uint)(so_len(b)), (so_uint)(count));
    so_uint hi = _res1.val;
    so_uint lo = _res1.val2;
    if (hi > 0 || lo > (so_uint)(maxInt)) {
        so_panic("bytes: Repeat output length overflow");
    }
    // lo = len(b) * count
    so_int n = (so_int)(lo);
    if (so_len(b) == 0) {
        return (so_Slice){0};
    }
    // Past a certain chunk size it is counterproductive to use
    // larger chunks as the source of the write, as when the source
    // is too large we are basically just thrashing the CPU D-cache.
    // So if the result length is larger than an empirically-found
    // limit (8KB), we stop growing the source string once the limit
    // is reached and keep reusing the same source string - that
    // should therefore be always resident in the L1 cache - until we
    // have completed the construction of the result.
    // This yields significant speedups (up to +100%) in cases where
    // the result length is large (roughly, over L2 cache size).
    const int64_t chunkLimit = 8 * 1024;
    so_int chunkMax = n;
    if (chunkMax > chunkLimit) {
        chunkMax = chunkLimit / so_len(b) * so_len(b);
        if (chunkMax == 0) {
            chunkMax = so_len(b);
        }
    }
    so_Slice nb = mem_AllocSlice(so_byte, (a), (n), (n));
    so_int bp = so_copy(so_byte, nb, b);
    for (; bp < n;) {
        so_int chunk = so_min(bp, chunkMax);
        bp += so_copy(so_byte, so_slice(so_byte, nb, bp, nb.len), so_slice(so_byte, nb, 0, chunk));
    }
    return nb;
}

// Replace returns a copy of the slice s with the first n
// non-overlapping instances of old replaced by new.
// If old is empty, it matches at the beginning of the slice
// and after each UTF-8 sequence, yielding up to k+1 replacements
// for a k-rune slice.
// If n < 0, there is no limit on the number of replacements.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_Replace(mem_Allocator a, so_Slice s, so_Slice old, so_Slice new, so_int n) {
    so_int m = 0;
    if (n != 0) {
        // Compute number of replacements.
        m = bytes_Count(s, old);
    }
    if (m == 0) {
        // Just return a copy.
        return slices_Clone(so_byte, (a), (s));
    }
    if (n < 0 || m < n) {
        n = m;
    }
    // Apply replacements to buffer.
    so_int tlen = so_len(s) + n * (so_len(new) - so_len(old));
    so_Slice t = mem_AllocSlice(so_byte, (a), (tlen), (tlen));
    so_int w = 0;
    so_int start = 0;
    if (so_len(old) > 0) {
        for (so_int _i = 0; _i < n; _i++) {
            so_int j = start + bytes_Index(so_slice(so_byte, s, start, s.len), old);
            w += so_copy(so_byte, so_slice(so_byte, t, w, t.len), so_slice(so_byte, s, start, j));
            w += so_copy(so_byte, so_slice(so_byte, t, w, t.len), new);
            start = j + so_len(old);
        }
    } else {
        // len(old) == 0
        w += so_copy(so_byte, so_slice(so_byte, t, w, t.len), new);
        for (so_int _i = 0; _i < n - 1; _i++) {
            so_R_rune_int _res1 = utf8_DecodeRune(so_slice(so_byte, s, start, s.len));
            so_int wid = _res1.val2;
            so_int j = start + wid;
            w += so_copy(so_byte, so_slice(so_byte, t, w, t.len), so_slice(so_byte, s, start, j));
            w += so_copy(so_byte, so_slice(so_byte, t, w, t.len), new);
            start = j;
        }
    }
    w += so_copy(so_byte, so_slice(so_byte, t, w, t.len), so_slice(so_byte, s, start, s.len));
    return so_slice(so_byte, t, 0, w);
}

// Runes interprets s as a sequence of UTF-8-encoded code points.
// It returns a slice of runes (Unicode code points) equivalent to s.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_Runes(mem_Allocator a, so_Slice s) {
    so_int tlen = utf8_RuneCount(s);
    so_Slice t = mem_AllocSlice(so_rune, (a), (tlen), (tlen));
    so_int i = 0;
    for (; so_len(s) > 0;) {
        so_R_rune_int _res1 = utf8_DecodeRune(s);
        so_rune r = _res1.val;
        so_int l = _res1.val2;
        so_at(so_rune, t, i) = r;
        i++;
        s = so_slice(so_byte, s, l, s.len);
    }
    return t;
}

// String creates a string from a byte slice.
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String bytes_String(mem_Allocator a, so_Slice s) {
    so_Slice clone = slices_Clone(so_byte, (a), (s));
    return so_bytes_string(clone);
}

// -- map.go --

// ToLower returns a copy of the byte slice s with all Unicode letters mapped to
// their lower case.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_ToLower(mem_Allocator a, so_Slice s) {
    bool isASCII = true, hasUpper = false;
    for (so_int i = 0; i < so_len(s); i++) {
        so_byte c = so_at(so_byte, s, i);
        if (c >= utf8_RuneSelf) {
            isASCII = false;
            break;
        }
        hasUpper = hasUpper || ('A' <= c && c <= 'Z');
    }
    if (isASCII) {
        // optimize for ASCII-only byte slices.
        if (!hasUpper) {
            return slices_Clone(so_byte, (a), (s));
        }
        so_Slice b = mem_AllocSlice(so_byte, (a), (so_len(s)), (so_len(s)));
        for (so_int i = 0; i < so_len(s); i++) {
            so_byte c = so_at(so_byte, s, i);
            if ('A' <= c && c <= 'Z') {
                c += U'a' - U'A';
            }
            so_at(so_byte, b, i) = c;
        }
        return b;
    }
    return bytes_Map(a, unicode_ToLower, s);
}

// ToUpper returns a copy of the byte slice s with all Unicode letters mapped to
// their upper case.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_ToUpper(mem_Allocator a, so_Slice s) {
    bool isASCII = true, hasLower = false;
    for (so_int i = 0; i < so_len(s); i++) {
        so_byte c = so_at(so_byte, s, i);
        if (c >= utf8_RuneSelf) {
            isASCII = false;
            break;
        }
        hasLower = hasLower || ('a' <= c && c <= 'z');
    }
    if (isASCII) {
        // optimize for ASCII-only byte slices.
        if (!hasLower) {
            // Just return a copy.
            return slices_Clone(so_byte, (a), (s));
        }
        so_Slice b = mem_AllocSlice(so_byte, (a), (so_len(s)), (so_len(s)));
        for (so_int i = 0; i < so_len(s); i++) {
            so_byte c = so_at(so_byte, s, i);
            if ('a' <= c && c <= 'z') {
                c -= U'a' - U'A';
            }
            so_at(so_byte, b, i) = c;
        }
        return b;
    }
    return bytes_Map(a, unicode_ToUpper, s);
}

// Map returns a copy of the byte slice s with all its characters modified
// according to the mapping function. If mapping returns a negative value, the character is
// dropped from the byte slice with no replacement. The characters in s and the
// output are interpreted as UTF-8-encoded code points.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_Map(mem_Allocator a, bytes_RuneFunc mapping, so_Slice s) {
    so_Slice b = mem_AllocSlice(so_byte, (a), (0), (so_len(s)));
    for (so_int i = 0; i < so_len(s);) {
        so_R_rune_int _res1 = utf8_DecodeRune(so_slice(so_byte, s, i, s.len));
        so_rune r = _res1.val;
        so_int wid = _res1.val2;
        r = mapping(r);
        if (r >= 0) {
            so_byte buf[4] = {0};
            so_int n = utf8_EncodeRune(so_array_slice(so_byte, buf, 0, 4, 4), r);
            b = slices_Extend(so_byte, (a), (b), (so_array_slice(so_byte, buf, 0, n, 4)));
        }
        i += wid;
    }
    return b;
}

// -- reader.go --

// Len returns the number of bytes of the unread portion of the
// slice.
so_int bytes_Reader_Len(void* self) {
    bytes_Reader* r = self;
    if (r->i >= (int64_t)(so_len(r->s))) {
        return 0;
    }
    return (so_int)((int64_t)(so_len(r->s)) - r->i);
}

// Size returns the original length of the underlying byte slice.
// Size is the number of bytes available for reading via [Reader.ReadAt].
// The result is unaffected by any method calls except [Reader.Reset].
int64_t bytes_Reader_Size(void* self) {
    bytes_Reader* r = self;
    return (int64_t)(so_len(r->s));
}

// Read implements the [io.Reader] interface.
so_R_int_err bytes_Reader_Read(void* self, so_Slice b) {
    bytes_Reader* r = self;
    if (r->i >= (int64_t)(so_len(r->s))) {
        return (so_R_int_err){.val = 0, .err = io_EOF};
    }
    r->prevRune = -1;
    so_int n = so_copy(so_byte, b, so_slice(so_byte, r->s, r->i, r->s.len));
    r->i += (int64_t)(n);
    return (so_R_int_err){.val = n, .err = (so_Error){0}};
}

// ReadAt implements the [io.ReaderAt] interface.
so_R_int_err bytes_Reader_ReadAt(void* self, so_Slice b, int64_t off) {
    bytes_Reader* r = self;
    // cannot modify state - see io.ReaderAt
    so_Error err = {0};
    if (off < 0) {
        return (so_R_int_err){.val = 0, .err = io_ErrOffset};
    }
    if (off >= (int64_t)(so_len(r->s))) {
        return (so_R_int_err){.val = 0, .err = io_EOF};
    }
    so_int n = so_copy(so_byte, b, so_slice(so_byte, r->s, off, r->s.len));
    if (n < so_len(b)) {
        err = io_EOF;
    }
    return (so_R_int_err){.val = n, .err = err};
}

// ReadByte implements the [io.ByteReader] interface.
so_R_byte_err bytes_Reader_ReadByte(void* self) {
    bytes_Reader* r = self;
    r->prevRune = -1;
    if (r->i >= (int64_t)(so_len(r->s))) {
        return (so_R_byte_err){.val = 0, .err = io_EOF};
    }
    so_byte b = so_at(so_byte, r->s, r->i);
    r->i++;
    return (so_R_byte_err){.val = b, .err = (so_Error){0}};
}

// UnreadByte complements [Reader.ReadByte] in implementing the [io.ByteScanner] interface.
so_Error bytes_Reader_UnreadByte(void* self) {
    bytes_Reader* r = self;
    if (r->i <= 0) {
        return io_ErrUnread;
    }
    r->prevRune = -1;
    r->i--;
    return (so_Error){0};
}

// ReadRune implements the [io.RuneReader] interface.
io_RuneSizeResult bytes_Reader_ReadRune(void* self) {
    bytes_Reader* r = self;
    if (r->i >= (int64_t)(so_len(r->s))) {
        r->prevRune = -1;
        return (io_RuneSizeResult){.Rune = 0, .Size = 0, .Err = io_EOF};
    }
    r->prevRune = (so_int)(r->i);
    {
        so_byte c = so_at(so_byte, r->s, r->i);
        if (c < utf8_RuneSelf) {
            r->i++;
            return (io_RuneSizeResult){.Rune = (so_rune)(c), .Size = 1, .Err = (so_Error){0}};
        }
    }
    so_R_rune_int _res1 = utf8_DecodeRune(so_slice(so_byte, r->s, r->i, r->s.len));
    so_rune ch = _res1.val;
    so_int size = _res1.val2;
    r->i += (int64_t)(size);
    return (io_RuneSizeResult){.Rune = ch, .Size = size, .Err = (so_Error){0}};
}

// UnreadRune complements [Reader.ReadRune] in implementing the [io.RuneScanner] interface.
so_Error bytes_Reader_UnreadRune(void* self) {
    bytes_Reader* r = self;
    if (r->i <= 0) {
        return io_ErrUnread;
    }
    if (r->prevRune < 0) {
        return io_ErrUnread;
    }
    r->i = (int64_t)(r->prevRune);
    r->prevRune = -1;
    return (so_Error){0};
}

// Seek implements the [io.Seeker] interface.
so_R_i64_err bytes_Reader_Seek(void* self, int64_t offset, so_int whence) {
    bytes_Reader* r = self;
    r->prevRune = -1;
    int64_t abs = 0;
    if (whence == io_SeekStart) {
        abs = offset;
    } else if (whence == io_SeekCurrent) {
        abs = r->i + offset;
    } else if (whence == io_SeekEnd) {
        abs = (int64_t)(so_len(r->s)) + offset;
    } else {
        return (so_R_i64_err){.val = 0, .err = io_ErrWhence};
    }
    if (abs < 0) {
        return (so_R_i64_err){.val = 0, .err = io_ErrOffset};
    }
    r->i = abs;
    return (so_R_i64_err){.val = abs, .err = (so_Error){0}};
}

// WriteTo implements the [io.WriterTo] interface.
so_R_i64_err bytes_Reader_WriteTo(void* self, io_Writer w) {
    bytes_Reader* r = self;
    so_Error err = {0};
    r->prevRune = -1;
    if (r->i >= (int64_t)(so_len(r->s))) {
        return (so_R_i64_err){.val = 0, .err = (so_Error){0}};
    }
    so_Slice b = so_slice(so_byte, r->s, r->i, r->s.len);
    so_R_int_err _res1 = w.Write(w.self, b);
    so_int m = _res1.val;
    err = _res1.err;
    if (m > so_len(b)) {
        return (so_R_i64_err){.val = 0, .err = io_ErrInvalidWrite};
    }
    r->i += (int64_t)(m);
    int64_t n = (int64_t)(m);
    if (m != so_len(b) && err.self == NULL) {
        err = io_ErrShortWrite;
    }
    return (so_R_i64_err){.val = n, .err = err};
}

// Reset resets the [Reader] to be reading from b.
void bytes_Reader_Reset(void* self, so_Slice b) {
    bytes_Reader* r = self;
    *r = (bytes_Reader){b, 0, -1};
}

// NewReader returns a new [Reader] reading from b.
bytes_Reader bytes_NewReader(so_Slice b) {
    return (bytes_Reader){b, 0, -1};
}

// -- split.go --

// Split slices s into all subslices separated by sep and returns a slice of
// the subslices between those separators.
// If sep is empty, Split splits after each UTF-8 sequence.
// It is equivalent to SplitN with a count of -1.
//
// To split around the first instance of a separator, see [Cut].
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
// The subslices in the returned slice are views into the original slice s.
so_Slice bytes_Split(mem_Allocator a, so_Slice s, so_Slice sep) {
    return genSplit(a, s, sep, 0, -1);
}

// SplitN slices s into subslices separated by sep and returns a slice of
// the subslices between those separators.
// If sep is empty, SplitN splits after each UTF-8 sequence.
// The count determines the number of subslices to return:
//   - n > 0: at most n subslices; the last subslice will be the unsplit remainder;
//   - n == 0: the result is nil (zero subslices);
//   - n < 0: all subslices.
//
// To split around the first instance of a separator, see [Cut].
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
// The subslices in the returned slice are views into the original slice s.
so_Slice bytes_SplitN(mem_Allocator a, so_Slice s, so_Slice sep, so_int n) {
    return genSplit(a, s, sep, 0, n);
}

// Generic split: splits after each instance of sep,
// including sepSave bytes of sep in the subslices.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
// The subslices in the returned slice are views into the original slice s.
static so_Slice genSplit(mem_Allocator a, so_Slice s, so_Slice sep, so_int sepSave, so_int n) {
    if (n == 0) {
        return (so_Slice){0};
    }
    if (so_len(sep) == 0) {
        return explode(a, s, n);
    }
    if (n < 0) {
        n = bytes_Count(s, sep) + 1;
    }
    if (n > so_len(s) + 1) {
        n = so_len(s) + 1;
    }
    so_Slice res = mem_AllocSlice(so_Slice, (a), (n), (n));
    n--;
    so_int i = 0;
    for (; i < n;) {
        so_int m = bytes_Index(s, sep);
        if (m < 0) {
            break;
        }
        so_at(so_Slice, res, i) = so_slice3(so_byte, s, 0, m + sepSave, m + sepSave);
        s = so_slice(so_byte, s, m + so_len(sep), s.len);
        i++;
    }
    so_at(so_Slice, res, i) = s;
    return so_slice(so_Slice, res, 0, i + 1);
}

// explode splits s into a slice of UTF-8 sequences, one per Unicode code point
// (still slices of bytes), up to a maximum of n byte slices. Invalid UTF-8
// sequences are chopped into individual bytes.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
// The subslices in the returned slice are views into the original slice s.
static so_Slice explode(mem_Allocator a, so_Slice s, so_int n) {
    if (n <= 0 || n > so_len(s)) {
        n = so_len(s);
    }
    so_Slice res = mem_AllocSlice(so_Slice, (a), (n), (n));
    so_int size = 0;
    so_int na = 0;
    for (; so_len(s) > 0;) {
        if (na + 1 >= n) {
            so_at(so_Slice, res, na) = s;
            na++;
            break;
        }
        so_R_rune_int _res1 = utf8_DecodeRune(s);
        size = _res1.val2;
        so_at(so_Slice, res, na) = so_slice3(so_byte, s, 0, size, size);
        s = so_slice(so_byte, s, size, s.len);
        na++;
    }
    return so_slice(so_Slice, res, 0, na);
}

// -- trim.go --

// contains reports whether c is inside the set.
static bool asciiSet_contains(void* self, so_byte c) {
    asciiSet* as = self;
    return (as->val[c / 32] & ((uint32_t)1 << (c % 32))) != 0;
}

// Trim returns a subslice of s by slicing off all leading and
// trailing UTF-8-encoded code points contained in cutset.
so_Slice bytes_Trim(so_Slice s, so_String cutset) {
    if (so_len(s) == 0) {
        return s;
    }
    if (so_string_eq(cutset, so_str(""))) {
        return s;
    }
    if (so_len(cutset) == 1 && so_at(so_byte, cutset, 0) < utf8_RuneSelf) {
        return trimLeftByte(trimRightByte(s, so_at(so_byte, cutset, 0)), so_at(so_byte, cutset, 0));
    }
    {
        asciiSet as = makeASCIISet(cutset);
        if (as.ok) {
            return trimLeftASCII(trimRightASCII(s, &as), &as);
        }
    }
    return trimLeftUnicode(trimRightUnicode(s, cutset), cutset);
}

// TrimFunc returns a subslice of s by slicing off all leading and trailing
// UTF-8-encoded code points c that satisfy f(c).
so_Slice bytes_TrimFunc(so_Slice s, bytes_RunePredicate f) {
    return trimRightFunc(trimLeftFunc(s, f), f);
}

// TrimLeft returns a subslice of s by slicing off all leading
// UTF-8-encoded code points contained in cutset.
so_Slice bytes_TrimLeft(so_Slice s, so_String cutset) {
    if (so_len(s) == 0) {
        return s;
    }
    if (so_string_eq(cutset, so_str(""))) {
        return s;
    }
    if (so_len(cutset) == 1 && so_at(so_byte, cutset, 0) < utf8_RuneSelf) {
        return trimLeftByte(s, so_at(so_byte, cutset, 0));
    }
    {
        asciiSet as = makeASCIISet(cutset);
        if (as.ok) {
            return trimLeftASCII(s, &as);
        }
    }
    return trimLeftUnicode(s, cutset);
}

// TrimPrefix returns s without the provided leading prefix string.
// If s doesn't start with prefix, s is returned unchanged.
so_Slice bytes_TrimPrefix(so_Slice s, so_Slice prefix) {
    if (bytes_HasPrefix(s, prefix)) {
        return so_slice(so_byte, s, so_len(prefix), s.len);
    }
    return s;
}

// TrimRight returns a subslice of s by slicing off all trailing
// UTF-8-encoded code points that are contained in cutset.
so_Slice bytes_TrimRight(so_Slice s, so_String cutset) {
    if (so_len(s) == 0 || so_string_eq(cutset, so_str(""))) {
        return s;
    }
    if (so_len(cutset) == 1 && so_at(so_byte, cutset, 0) < utf8_RuneSelf) {
        return trimRightByte(s, so_at(so_byte, cutset, 0));
    }
    {
        asciiSet as = makeASCIISet(cutset);
        if (as.ok) {
            return trimRightASCII(s, &as);
        }
    }
    return trimRightUnicode(s, cutset);
}

// TrimSpace returns a subslice of s by slicing off all leading and
// trailing white space, as defined by Unicode.
so_Slice bytes_TrimSpace(so_Slice s) {
    // Fast path for ASCII: look for the first ASCII non-space byte.
    for (so_int lo = 0; lo < so_len(s); lo++) {
        so_byte c = so_at(so_byte, s, lo);
        if (c >= utf8_RuneSelf) {
            // If we run into a non-ASCII byte, fall back to the
            // slower unicode-aware method on the remaining bytes.
            return bytes_TrimFunc(so_slice(so_byte, s, lo, s.len), unicode_IsSpace);
        }
        if (asciiSpace[c] != 0) {
            continue;
        }
        s = so_slice(so_byte, s, lo, s.len);
        // Now look for the first ASCII non-space byte from the end.
        for (so_int hi = so_len(s) - 1; hi >= 0; hi--) {
            so_byte c = so_at(so_byte, s, hi);
            if (c >= utf8_RuneSelf) {
                return bytes_TrimFunc(so_slice(so_byte, s, 0, hi + 1), unicode_IsSpace);
            }
            if (asciiSpace[c] == 0) {
                // At this point, s[:hi+1] starts and ends with ASCII
                // non-space bytes, so we're done. Non-ASCII cases have
                // already been handled above.
                return so_slice(so_byte, s, 0, hi + 1);
            }
        }
    }
    return (so_Slice){0};
}

// TrimSuffix returns s without the provided trailing suffix string.
// If s doesn't end with suffix, s is returned unchanged.
so_Slice bytes_TrimSuffix(so_Slice s, so_Slice suffix) {
    if (bytes_HasSuffix(s, suffix)) {
        return so_slice(so_byte, s, 0, so_len(s) - so_len(suffix));
    }
    return s;
}

static so_Slice trimLeftASCII(so_Slice s, asciiSet* as) {
    for (; so_len(s) > 0;) {
        if (!asciiSet_contains(as, so_at(so_byte, s, 0))) {
            break;
        }
        s = so_slice(so_byte, s, 1, s.len);
    }
    if (so_len(s) == 0) {
        return (so_Slice){0};
    }
    return s;
}

static so_Slice trimLeftByte(so_Slice s, so_byte c) {
    for (; so_len(s) > 0 && so_at(so_byte, s, 0) == c;) {
        s = so_slice(so_byte, s, 1, s.len);
    }
    if (so_len(s) == 0) {
        return (so_Slice){0};
    }
    return s;
}

// trimLeftFunc treats s as UTF-8-encoded bytes and returns a subslice of s by slicing off
// all leading UTF-8-encoded code points c that satisfy f(c).
static so_Slice trimLeftFunc(so_Slice s, bytes_RunePredicate f) {
    so_int i = indexFunc(s, f, false);
    if (i == -1) {
        return (so_Slice){0};
    }
    return so_slice(so_byte, s, i, s.len);
}

static so_Slice trimLeftUnicode(so_Slice s, so_String cutset) {
    for (; so_len(s) > 0;) {
        so_R_rune_int _res1 = utf8_DecodeRune(s);
        so_rune r = _res1.val;
        so_int n = _res1.val2;
        if (!containsRune(cutset, r)) {
            break;
        }
        s = so_slice(so_byte, s, n, s.len);
    }
    if (so_len(s) == 0) {
        return (so_Slice){0};
    }
    return s;
}

static so_Slice trimRightASCII(so_Slice s, asciiSet* as) {
    for (; so_len(s) > 0;) {
        if (!asciiSet_contains(as, so_at(so_byte, s, so_len(s) - 1))) {
            break;
        }
        s = so_slice(so_byte, s, 0, so_len(s) - 1);
    }
    return s;
}

static so_Slice trimRightByte(so_Slice s, so_byte c) {
    for (; so_len(s) > 0 && so_at(so_byte, s, so_len(s) - 1) == c;) {
        s = so_slice(so_byte, s, 0, so_len(s) - 1);
    }
    return s;
}

// trimRightFunc returns a subslice of s by slicing off all trailing
// UTF-8-encoded code points c that satisfy f(c).
static so_Slice trimRightFunc(so_Slice s, bytes_RunePredicate f) {
    so_int i = lastIndexFunc(s, f, false);
    if (i >= 0 && so_at(so_byte, s, i) >= utf8_RuneSelf) {
        so_R_rune_int _res1 = utf8_DecodeRune(so_slice(so_byte, s, i, s.len));
        so_int wid = _res1.val2;
        i += wid;
    } else {
        i++;
    }
    return so_slice(so_byte, s, 0, i);
}

static so_Slice trimRightUnicode(so_Slice s, so_String cutset) {
    for (; so_len(s) > 0;) {
        so_rune r = (so_rune)(so_at(so_byte, s, so_len(s) - 1));
        so_int n = 1;
        if (r >= utf8_RuneSelf) {
            so_R_rune_int _res1 = utf8_DecodeLastRune(s);
            r = _res1.val;
            n = _res1.val2;
        }
        if (!containsRune(cutset, r)) {
            break;
        }
        s = so_slice(so_byte, s, 0, so_len(s) - n);
    }
    return s;
}

// containsRune is a simplified version of strings.ContainsRune
// to avoid importing the strings package.
static bool containsRune(so_String s, so_rune r) {
    for (so_int _ = 0, __w = 0; _ < so_len(s); _ += __w) {
        __w = 0;
        so_rune c = so_utf8_decode(s, _, &__w);
        if (c == r) {
            return true;
        }
    }
    return false;
}

// indexFunc is the same as IndexFunc except that if
// truth==false, the sense of the predicate function is
// inverted.
static so_int indexFunc(so_Slice s, bytes_RunePredicate f, bool truth) {
    so_int start = 0;
    for (; start < so_len(s);) {
        so_R_rune_int _res1 = utf8_DecodeRune(so_slice(so_byte, s, start, s.len));
        so_rune r = _res1.val;
        so_int wid = _res1.val2;
        if (f(r) == truth) {
            return start;
        }
        start += wid;
    }
    return -1;
}

// lastIndexFunc is the same as LastIndexFunc except that if
// truth==false, the sense of the predicate function is
// inverted.
static so_int lastIndexFunc(so_Slice s, bytes_RunePredicate f, bool truth) {
    for (so_int i = so_len(s); i > 0;) {
        so_rune r = (so_rune)(so_at(so_byte, s, i - 1));
        so_int size = 1;
        if (r >= utf8_RuneSelf) {
            so_R_rune_int _res1 = utf8_DecodeLastRune(so_slice(so_byte, s, 0, i));
            r = _res1.val;
            size = _res1.val2;
        }
        i -= size;
        if (f(r) == truth) {
            return i;
        }
    }
    return -1;
}

// makeASCIISet creates a set of ASCII characters and reports whether all
// characters in chars are ASCII.
static asciiSet makeASCIISet(so_String chars) {
    asciiSet as = {0};
    for (so_int i = 0; i < so_len(chars); i++) {
        so_byte c = so_at(so_byte, chars, i);
        if (c >= utf8_RuneSelf) {
            return as;
        }
        as.val[c / 32] |= ((uint32_t)1 << (c % 32));
    }
    as.ok = true;
    return as;
}
