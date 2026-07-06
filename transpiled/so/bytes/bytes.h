#pragma once
#include "so/builtin/builtin.h"
#include "so/bytealg/bytealg.h"
#include "so/errors/errors.h"
#include "so/io/io.h"
#include "so/math/bits/bits.h"
#include "so/mem/mem.h"
#include "so/slices/slices.h"
#include "so/unicode/unicode.h"
#include "so/unicode/utf8/utf8.h"

// -- Types --

typedef struct bytes_Buffer bytes_Buffer;
typedef struct bytes_CutResult bytes_CutResult;
typedef struct bytes_Reader bytes_Reader;

// A Buffer is a variable-sized buffer of bytes with [Buffer.Read] and [Buffer.Write] methods.
// The zero value for Buffer is an empty buffer ready to use (with default allocator).
// A Buffer grows as needed when writing data, using the provided allocator.
// The caller is responsible for freeing the buffer's resources
// with [Buffer.Free] when done using it.
typedef struct bytes_Buffer {
    mem_Allocator a;
    so_Slice buf;
    so_int off;
} bytes_Buffer;

// CutResult is the result of a Cut operation.
typedef struct bytes_CutResult {
    so_Slice Before;
    so_Slice After;
    bool Found;
} bytes_CutResult;

// RuneFunc maps a rune to another rune. If mapping returns
// a negative value, the rune is dropped from the result.
typedef so_rune (*bytes_RuneFunc)(so_rune);

// A Reader implements the [io.Reader], [io.ReaderAt], [io.WriterTo], [io.Seeker],
// [io.ByteScanner], and [io.RuneScanner] interfaces by reading from a byte slice.
// Unlike a [Buffer], a Reader is read-only and supports seeking.
// The zero value for Reader operates like a Reader of an empty slice.
typedef struct bytes_Reader {
    so_Slice s;
    int64_t i;
    so_int prevRune;
} bytes_Reader;

// RunePredicate reports whether the rune satisfies a condition.
typedef bool (*bytes_RunePredicate)(so_rune);

// -- Variables and constants --

// ErrNegativeGrow means that a Buffer.Grow call was given a negative count.
extern so_Error bytes_ErrNegativeGrow;

// MinRead is the minimum slice size passed to a [Buffer.Read] call by
// [Buffer.ReadFrom]. As long as the [Buffer] has at least MinRead bytes beyond
// what is required to hold the contents of r, [Buffer.ReadFrom] will not grow the
// underlying buffer.
static const int64_t bytes_MinRead = 512;

// -- Functions and methods --

// Bytes returns a slice of length b.Len() holding the unread portion of the buffer.
// The slice is valid for use only until the next buffer modification (that is,
// only until the next call to a method like [Buffer.Read], [Buffer.Write], [Buffer.Reset].
// The slice aliases the buffer content at least until the next buffer modification,
// so immediate changes to the slice will affect the result of future reads.
so_Slice bytes_Buffer_Bytes(void* self);

// String returns the contents of the unread portion of the buffer
// as a string. If the [Buffer] is a nil pointer, it returns "<nil>".
// The string is valid for use only until the next buffer modification.
//
// To build strings more efficiently, see the [strings.Builder] type.
so_String bytes_Buffer_String(void* self);

// Peek returns the next n bytes without advancing the buffer.
// If Peek returns fewer than n bytes, it also returns [io.EOF].
// The slice is only valid until the next call to a read or write method.
// The slice aliases the buffer content at least until the next buffer modification,
// so immediate changes to the slice will affect the result of future reads.
so_R_slice_err bytes_Buffer_Peek(void* self, so_int n);

// Len returns the number of bytes of the unread portion of the buffer;
// b.Len() == len(b.Bytes()).
so_int bytes_Buffer_Len(void* self);

// Cap returns the capacity of the buffer's underlying byte slice, that is, the
// total space allocated for the buffer's data.
so_int bytes_Buffer_Cap(void* self);

// Available returns how many bytes are unused in the buffer.
so_int bytes_Buffer_Available(void* self);

// Reset resets the buffer to be empty,
// but it retains the underlying storage for use by future writes.
void bytes_Buffer_Reset(void* self);

// Free frees the internal buffer and resets the buffer.
// After Free, the buffer can be reused with new writes.
void bytes_Buffer_Free(void* self);

// Grow grows the buffer's capacity, if necessary, to guarantee space for
// another n bytes. After Grow(n), at least n bytes can be written to the
// buffer without another allocation.
// Panics if n is negative or if the buffer cannot grow.
void bytes_Buffer_Grow(void* self, so_int n);

// Write appends the contents of p to the buffer, growing the buffer as
// needed. The return value n is the length of p; err is always nil.
// Panics if the buffer becomes too large.
so_R_int_err bytes_Buffer_Write(void* self, so_Slice p);

// WriteString appends the contents of s to the buffer, growing the buffer as
// needed. The return value n is the length of s; err is always nil.
// Panics if the buffer becomes too large.
so_R_int_err bytes_Buffer_WriteString(void* self, so_String s);

// ReadFrom reads data from r until EOF and appends it to the buffer, growing
// the buffer as needed. The return value n is the number of bytes read. Any
// error except io.EOF encountered during the read is also returned.
// Panics if the buffer becomes too large.
so_R_i64_err bytes_Buffer_ReadFrom(void* self, io_Reader r);

// WriteTo writes data to w until the buffer is drained or an error occurs.
// The return value n is the number of bytes written; it always fits into an
// int, but it is int64 to match the [io.WriterTo] interface. Any error
// encountered during the write is also returned.
so_R_i64_err bytes_Buffer_WriteTo(void* self, io_Writer w);

// WriteByte appends the byte c to the buffer, growing the buffer as needed.
// The returned error is always nil, but is included to match [bufio.Writer]'s
// WriteByte. Panics if the buffer becomes too large.
so_Error bytes_Buffer_WriteByte(void* self, so_byte c);

// WriteRune appends the UTF-8 encoding of Unicode code point r to the
// buffer, returning its length and an error, which is always nil but is
// included to match [bufio.Writer]'s WriteRune. The buffer is grown as needed;
// if it becomes too large, WriteRune will panic.
so_R_int_err bytes_Buffer_WriteRune(void* self, so_rune r);

// Read reads the next len(p) bytes from the buffer or until the buffer
// is drained. The return value n is the number of bytes read. If the
// buffer has no data to return, err is [io.EOF] (unless len(p) is zero);
// otherwise it is nil.
so_R_int_err bytes_Buffer_Read(void* self, so_Slice p);

// Next returns a slice containing the next n bytes from the buffer,
// advancing the buffer as if the bytes had been returned by [Buffer.Read].
// If there are fewer than n bytes in the buffer, Next returns the entire buffer.
// The slice is only valid until the next call to a read or write method.
so_Slice bytes_Buffer_Next(void* self, so_int n);

// ReadByte reads and returns the next byte from the buffer.
// If no byte is available, it returns error [io.EOF].
so_R_byte_err bytes_Buffer_ReadByte(void* self);

// ReadRune reads and returns the next UTF-8-encoded
// Unicode code point from the buffer.
// If no bytes are available, the error returned is io.EOF.
// If the bytes are an erroneous UTF-8 encoding, it
// consumes one byte and returns U+FFFD, 1.
io_RuneSizeResult bytes_Buffer_ReadRune(void* self);

// ReadBytes reads until the first occurrence of delim in the input,
// returning a slice containing the data up to and including the delimiter.
// If ReadBytes encounters an error before finding a delimiter,
// it returns the data read before the error and the error itself (often [io.EOF]).
// ReadBytes returns err != nil if and only if the returned data does not end in
// delim.
//
// The returned slice is allocated; the caller owns it.
so_R_slice_err bytes_Buffer_ReadBytes(void* self, so_byte delim);

// ReadString reads until the first occurrence of delim in the input,
// returning a string containing the data up to and including the delimiter.
// If ReadString encounters an error before finding a delimiter,
// it returns the data read before the error and the error itself (often [io.EOF]).
// ReadString returns err != nil if and only if the returned data does not end
// in delim.
//
// The returned string is allocated; the caller owns it.
so_R_str_err bytes_Buffer_ReadString(void* self, so_byte delim);

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
bytes_Buffer bytes_NewBuffer(mem_Allocator a, so_Slice buf);

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
bytes_Buffer bytes_NewBufferString(mem_Allocator a, so_String s);

// Clone returns a copy of b[:len(b)].
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_Clone(mem_Allocator a, so_Slice b);

// Contains reports whether subslice is within b.
bool bytes_Contains(so_Slice b, so_Slice subslice);

// Compare returns an integer comparing two byte slices lexicographically.
// The result will be 0 if a == b, -1 if a < b, and +1 if a > b.
// A nil argument is equivalent to an empty slice.
so_int bytes_Compare(so_Slice a, so_Slice b);

// Count counts the number of non-overlapping instances of sep in s.
// If sep is an empty slice, Count returns 1 + the number of UTF-8-encoded code points in s.
so_int bytes_Count(so_Slice s, so_Slice sep);

// Cut slices s around the first instance of sep,
// returning the text before and after sep.
// The found result reports whether sep appears in s.
// If sep does not appear in s, cut returns s, nil, false.
//
// Cut returns slices of the original slice s, not copies.
bytes_CutResult bytes_Cut(so_Slice s, so_Slice sep);

// Equal reports whether a and b
// are the same length and contain the same bytes.
// A nil argument is equivalent to an empty slice.
bool bytes_Equal(so_Slice a, so_Slice b);

// HasPrefix reports whether the byte slice s begins with prefix.
bool bytes_HasPrefix(so_Slice s, so_Slice prefix);

// HasSuffix reports whether the byte slice s ends with suffix.
bool bytes_HasSuffix(so_Slice s, so_Slice suffix);

// Index returns the index of the first instance of sep in s, or -1 if sep is not present in s.
so_int bytes_Index(so_Slice s, so_Slice sep);

// IndexByte returns the index of the first instance of c in b, or -1 if c is not present in b.
so_int bytes_IndexByte(so_Slice b, so_byte c);

// Join concatenates the elements of s to create a new byte slice. The separator
// sep is placed between elements in the resulting slice.
// Panics if the result is too large to allocate.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_Join(mem_Allocator a, so_Slice s, so_Slice sep);

// Repeat returns a new byte slice consisting of count copies of b.
// It panics if count is negative or if the result of (len(b) * count) overflows.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_Repeat(mem_Allocator a, so_Slice b, so_int count);

// Replace returns a copy of the slice s with the first n
// non-overlapping instances of old replaced by new.
// If old is empty, it matches at the beginning of the slice
// and after each UTF-8 sequence, yielding up to k+1 replacements
// for a k-rune slice.
// If n < 0, there is no limit on the number of replacements.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_Replace(mem_Allocator a, so_Slice s, so_Slice old, so_Slice new, so_int n);

// Runes interprets s as a sequence of UTF-8-encoded code points.
// It returns a slice of runes (Unicode code points) equivalent to s.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_Runes(mem_Allocator a, so_Slice s);

// String creates a string from a byte slice.
// If the allocator is nil, uses the system allocator.
// The returned string is allocated; the caller owns it.
so_String bytes_String(mem_Allocator a, so_Slice s);

// ToLower returns a copy of the byte slice s with all Unicode letters mapped to
// their lower case.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_ToLower(mem_Allocator a, so_Slice s);

// ToUpper returns a copy of the byte slice s with all Unicode letters mapped to
// their upper case.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_ToUpper(mem_Allocator a, so_Slice s);

// Map returns a copy of the byte slice s with all its characters modified
// according to the mapping function. If mapping returns a negative value, the character is
// dropped from the byte slice with no replacement. The characters in s and the
// output are interpreted as UTF-8-encoded code points.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_Slice bytes_Map(mem_Allocator a, bytes_RuneFunc mapping, so_Slice s);

// Len returns the number of bytes of the unread portion of the
// slice.
so_int bytes_Reader_Len(void* self);

// Size returns the original length of the underlying byte slice.
// Size is the number of bytes available for reading via [Reader.ReadAt].
// The result is unaffected by any method calls except [Reader.Reset].
int64_t bytes_Reader_Size(void* self);

// Read implements the [io.Reader] interface.
so_R_int_err bytes_Reader_Read(void* self, so_Slice b);

// ReadAt implements the [io.ReaderAt] interface.
so_R_int_err bytes_Reader_ReadAt(void* self, so_Slice b, int64_t off);

// ReadByte implements the [io.ByteReader] interface.
so_R_byte_err bytes_Reader_ReadByte(void* self);

// UnreadByte complements [Reader.ReadByte] in implementing the [io.ByteScanner] interface.
so_Error bytes_Reader_UnreadByte(void* self);

// ReadRune implements the [io.RuneReader] interface.
io_RuneSizeResult bytes_Reader_ReadRune(void* self);

// UnreadRune complements [Reader.ReadRune] in implementing the [io.RuneScanner] interface.
so_Error bytes_Reader_UnreadRune(void* self);

// Seek implements the [io.Seeker] interface.
so_R_i64_err bytes_Reader_Seek(void* self, int64_t offset, so_int whence);

// WriteTo implements the [io.WriterTo] interface.
so_R_i64_err bytes_Reader_WriteTo(void* self, io_Writer w);

// Reset resets the [Reader] to be reading from b.
void bytes_Reader_Reset(void* self, so_Slice b);

// NewReader returns a new [Reader] reading from b.
bytes_Reader bytes_NewReader(so_Slice b);

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
so_Slice bytes_Split(mem_Allocator a, so_Slice s, so_Slice sep);

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
so_Slice bytes_SplitN(mem_Allocator a, so_Slice s, so_Slice sep, so_int n);

// Trim returns a subslice of s by slicing off all leading and
// trailing UTF-8-encoded code points contained in cutset.
so_Slice bytes_Trim(so_Slice s, so_String cutset);

// TrimFunc returns a subslice of s by slicing off all leading and trailing
// UTF-8-encoded code points c that satisfy f(c).
so_Slice bytes_TrimFunc(so_Slice s, bytes_RunePredicate f);

// TrimLeft returns a subslice of s by slicing off all leading
// UTF-8-encoded code points contained in cutset.
so_Slice bytes_TrimLeft(so_Slice s, so_String cutset);

// TrimPrefix returns s without the provided leading prefix string.
// If s doesn't start with prefix, s is returned unchanged.
so_Slice bytes_TrimPrefix(so_Slice s, so_Slice prefix);

// TrimRight returns a subslice of s by slicing off all trailing
// UTF-8-encoded code points that are contained in cutset.
so_Slice bytes_TrimRight(so_Slice s, so_String cutset);

// TrimSpace returns a subslice of s by slicing off all leading and
// trailing white space, as defined by Unicode.
so_Slice bytes_TrimSpace(so_Slice s);

// TrimSuffix returns s without the provided trailing suffix string.
// If s doesn't end with suffix, s is returned unchanged.
so_Slice bytes_TrimSuffix(so_Slice s, so_Slice suffix);
