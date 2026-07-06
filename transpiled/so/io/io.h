#pragma once
#include "so/builtin/builtin.h"
#include "so/errors/errors.h"
#include "so/mem/mem.h"
#include "so/slices/slices.h"

// -- Types --

typedef struct io_RuneSizeResult io_RuneSizeResult;
typedef struct io_DiscardWriter io_DiscardWriter;
typedef struct io_LimitedReader io_LimitedReader;
typedef struct io_NopCloser io_NopCloser;
typedef struct io_SectionReader io_SectionReader;
typedef struct io_ReaderAtOffset io_ReaderAtOffset;
typedef struct io_MultiReader io_MultiReader;
typedef struct io_MultiWriter io_MultiWriter;

// Reader is the interface that wraps the basic Read method.
//
// Read reads up to len(p) bytes into p. It returns the number of bytes
// read (0 <= n <= len(p)) and any error encountered. Even if Read
// returns n < len(p), it may use all of p as scratch space during the call.
// If some data is available but not len(p) bytes, Read conventionally
// returns what is available instead of waiting for more.
//
// When Read encounters an error or end-of-file condition after
// successfully reading n > 0 bytes, it returns the number of
// bytes read. It may return the (non-nil) error from the same call
// or return the error (and n == 0) from a subsequent call.
// An instance of this general case is that a Reader returning
// a non-zero number of bytes at the end of the input stream may
// return either err == EOF or err == nil. The next Read should
// return 0, EOF.
//
// Callers should always process the n > 0 bytes returned before
// considering the error err. Doing so correctly handles I/O errors
// that happen after reading some bytes and also both of the
// allowed EOF behaviors.
//
// If len(p) == 0, Read should always return n == 0. It may return a
// non-nil error if some error condition is known, such as EOF.
//
// Implementations of Read are discouraged from returning a
// zero byte count with a nil error, except when len(p) == 0.
// Callers should treat a return of 0 and nil as indicating that
// nothing happened; in particular it does not indicate EOF.
//
// Implementations must not retain p.
typedef struct io_Reader {
    void* self;
    so_R_int_err (*Read)(void* self, so_Slice p);
} io_Reader;

// Writer is the interface that wraps the basic Write method.
//
// Write writes len(p) bytes from p to the underlying data stream.
// It returns the number of bytes written from p (0 <= n <= len(p))
// and any error encountered that caused the write to stop early.
// Write must return a non-nil error if it returns n < len(p).
// Write must not modify the slice data, even temporarily.
//
// Implementations must not retain p.
typedef struct io_Writer {
    void* self;
    so_R_int_err (*Write)(void* self, so_Slice p);
} io_Writer;

// Closer is the interface that wraps the basic Close method.
//
// The behavior of Close after the first call is undefined.
// Specific implementations may document their own behavior.
typedef struct io_Closer {
    void* self;
    so_Error (*Close)(void* self);
} io_Closer;

// Seeker is the interface that wraps the basic Seek method.
//
// Seek sets the offset for the next Read or Write to offset,
// interpreted according to whence:
// [SeekStart] means relative to the start of the file,
// [SeekCurrent] means relative to the current offset, and
// [SeekEnd] means relative to the end
// (for example, offset = -2 specifies the penultimate byte of the file).
// Seek returns the new offset relative to the start of the
// file or an error, if any.
//
// Seeking to an offset before the start of the file is an error.
// Seeking to any positive offset may be allowed, but if the new offset exceeds
// the size of the underlying object the behavior of subsequent I/O operations
// is implementation-dependent.
typedef struct io_Seeker {
    void* self;
    so_R_i64_err (*Seek)(void* self, int64_t offset, so_int whence);
} io_Seeker;

// ReadWriter is the interface that groups the basic Read and Write methods.
typedef struct io_ReadWriter {
    void* self;
    so_R_int_err (*Read)(void* self, so_Slice p);
    so_R_int_err (*Write)(void* self, so_Slice p);
} io_ReadWriter;

// ReadCloser is the interface that groups the basic Read and Close methods.
typedef struct io_ReadCloser {
    void* self;
    so_Error (*Close)(void* self);
    so_R_int_err (*Read)(void* self, so_Slice p);
} io_ReadCloser;

// WriteCloser is the interface that groups the basic Write and Close methods.
typedef struct io_WriteCloser {
    void* self;
    so_Error (*Close)(void* self);
    so_R_int_err (*Write)(void* self, so_Slice p);
} io_WriteCloser;

// ReadWriteCloser is the interface that groups the basic Read, Write and Close methods.
typedef struct io_ReadWriteCloser {
    void* self;
    so_Error (*Close)(void* self);
    so_R_int_err (*Read)(void* self, so_Slice p);
    so_R_int_err (*Write)(void* self, so_Slice p);
} io_ReadWriteCloser;

// ReadSeeker is the interface that groups the basic Read and Seek methods.
typedef struct io_ReadSeeker {
    void* self;
    so_R_int_err (*Read)(void* self, so_Slice p);
    so_R_i64_err (*Seek)(void* self, int64_t offset, so_int whence);
} io_ReadSeeker;

// ReadSeekCloser is the interface that groups the basic Read, Seek and Close methods.
typedef struct io_ReadSeekCloser {
    void* self;
    so_Error (*Close)(void* self);
    so_R_int_err (*Read)(void* self, so_Slice p);
    so_R_i64_err (*Seek)(void* self, int64_t offset, so_int whence);
} io_ReadSeekCloser;

// WriteSeeker is the interface that groups the basic Write and Seek methods.
typedef struct io_WriteSeeker {
    void* self;
    so_R_i64_err (*Seek)(void* self, int64_t offset, so_int whence);
    so_R_int_err (*Write)(void* self, so_Slice p);
} io_WriteSeeker;

// ReadWriteSeeker is the interface that groups the basic Read, Write and Seek methods.
typedef struct io_ReadWriteSeeker {
    void* self;
    so_R_int_err (*Read)(void* self, so_Slice p);
    so_R_i64_err (*Seek)(void* self, int64_t offset, so_int whence);
    so_R_int_err (*Write)(void* self, so_Slice p);
} io_ReadWriteSeeker;

// ReaderFrom is the interface that wraps the ReadFrom method.
//
// ReadFrom reads data from r until EOF or error.
// The return value n is the number of bytes read.
// Any error except EOF encountered during the read is also returned.
typedef struct io_ReaderFrom {
    void* self;
    so_R_i64_err (*ReadFrom)(void* self, io_Reader r);
} io_ReaderFrom;

// WriterTo is the interface that wraps the WriteTo method.
//
// WriteTo writes data to w until there's no more data to write or
// when an error occurs. The return value n is the number of bytes
// written. Any error encountered during the write is also returned.
typedef struct io_WriterTo {
    void* self;
    so_R_i64_err (*WriteTo)(void* self, io_Writer w);
} io_WriterTo;

// ReaderAt is the interface that wraps the basic ReadAt method.
//
// ReadAt reads len(p) bytes into p starting at offset off in the
// underlying input source. It returns the number of bytes
// read (0 <= n <= len(p)) and any error encountered.
//
// When ReadAt returns n < len(p), it returns a non-nil error
// explaining why more bytes were not returned. In this respect,
// ReadAt is stricter than Read.
//
// Even if ReadAt returns n < len(p), it may use all of p as scratch
// space during the call. If some data is available but not len(p) bytes,
// ReadAt blocks until either all the data is available or an error occurs.
// In this respect ReadAt is different from Read.
//
// If the n = len(p) bytes returned by ReadAt are at the end of the
// input source, ReadAt may return either err == EOF or err == nil.
//
// If ReadAt is reading from an input source with a seek offset,
// ReadAt should not affect nor be affected by the underlying
// seek offset.
//
// Clients of ReadAt can execute parallel ReadAt calls on the
// same input source.
//
// Implementations must not retain p.
typedef struct io_ReaderAt {
    void* self;
    so_R_int_err (*ReadAt)(void* self, so_Slice p, int64_t off);
} io_ReaderAt;

// WriterAt is the interface that wraps the basic WriteAt method.
//
// WriteAt writes len(p) bytes from p to the underlying data stream
// at offset off. It returns the number of bytes written from p (0 <= n <= len(p))
// and any error encountered that caused the write to stop early.
// WriteAt must return a non-nil error if it returns n < len(p).
//
// If WriteAt is writing to a destination with a seek offset,
// WriteAt should not affect nor be affected by the underlying
// seek offset.
//
// Clients of WriteAt can execute parallel WriteAt calls on the same
// destination if the ranges do not overlap.
//
// Implementations must not retain p.
typedef struct io_WriterAt {
    void* self;
    so_R_int_err (*WriteAt)(void* self, so_Slice p, int64_t off);
} io_WriterAt;

// ByteReader is the interface that wraps the ReadByte method.
//
// ReadByte reads and returns the next byte from the input or
// any error encountered. If ReadByte returns an error, no input
// byte was consumed, and the returned byte value is undefined.
//
// ReadByte provides an efficient interface for byte-at-time
// processing. A [Reader] that does not implement ByteReader
// can be wrapped using bufio.NewReader to add this method.
typedef struct io_ByteReader {
    void* self;
    so_R_byte_err (*ReadByte)(void* self);
} io_ByteReader;

// ByteScanner is the interface that adds the UnreadByte method to the
// basic ReadByte method.
//
// UnreadByte causes the next call to ReadByte to return the last byte read.
// If the last operation was not a successful call to ReadByte, UnreadByte may
// return an error, unread the last byte read (or the byte prior to the
// last-unread byte), or (in implementations that support the [Seeker] interface)
// seek to one byte before the current offset.
typedef struct io_ByteScanner {
    void* self;
    so_R_byte_err (*ReadByte)(void* self);
    so_Error (*UnreadByte)(void* self);
} io_ByteScanner;

// ByteWriter is the interface that wraps the WriteByte method.
typedef struct io_ByteWriter {
    void* self;
    so_Error (*WriteByte)(void* self, so_byte c);
} io_ByteWriter;

// RuneSizeResult is the result of a [RuneReader.ReadRune] operation:
// the rune read, its size in bytes, and any error encountered.
typedef struct io_RuneSizeResult {
    so_rune Rune;
    so_int Size;
    so_Error Err;
} io_RuneSizeResult;

// RuneReader is the interface that wraps the ReadRune method.
//
// ReadRune reads a single encoded Unicode character
// and returns the rune and its size in bytes. If no character is
// available, err will be set.
typedef struct io_RuneReader {
    void* self;
    io_RuneSizeResult (*ReadRune)(void* self);
} io_RuneReader;

// RuneScanner is the interface that adds the UnreadRune method to the
// basic ReadRune method.
//
// UnreadRune causes the next call to ReadRune to return the last rune read.
// If the last operation was not a successful call to ReadRune, UnreadRune may
// return an error, unread the last rune read (or the rune prior to the
// last-unread rune), or (in implementations that support the [Seeker] interface)
// seek to the start of the rune before the current offset.
typedef struct io_RuneScanner {
    void* self;
    io_RuneSizeResult (*ReadRune)(void* self);
    so_Error (*UnreadRune)(void* self);
} io_RuneScanner;

// StringWriter is the interface that wraps the WriteString method.
typedef struct io_StringWriter {
    void* self;
    so_R_int_err (*WriteString)(void* self, so_String s);
} io_StringWriter;

// A DiscardWriter provides Write methods
// that succeed without doing anything.
typedef struct io_DiscardWriter {
} io_DiscardWriter;

// A LimitedReader reads from R but limits the amount of
// data returned to just N bytes. Each call to Read
// updates N to reflect the new amount remaining.
// Read returns EOF when N <= 0 or when the underlying R returns EOF.
typedef struct io_LimitedReader {
    io_Reader R;
    int64_t N;
} io_LimitedReader;

// A NopCloser is a [ReadCloser] with a no-op Close method wrapping
// the provided [Reader] r.
typedef struct io_NopCloser {
    io_Reader r;
} io_NopCloser;

// SectionReader implements Read, Seek, and ReadAt on a section
// of an underlying [ReaderAt].
typedef struct io_SectionReader {
    io_ReaderAt r;
    int64_t base;
    int64_t off;
    int64_t limit;
    int64_t n;
} io_SectionReader;

// ReaderAtOffset represents the underlying [ReaderAt] and offsets for a section.
typedef struct io_ReaderAtOffset {
    io_ReaderAt R;
    int64_t Off;
    int64_t N;
} io_ReaderAtOffset;

typedef struct io_MultiReader {
    so_Slice readers;
} io_MultiReader;

typedef struct io_MultiWriter {
    so_Slice writers;
} io_MultiWriter;

// -- Variables and constants --

// Discard is a [Writer] on which all Write calls
// succeed without doing anything.
extern io_Writer io_Discard;

// Seek whence values.
static const int64_t io_SeekStart = 0;
static const int64_t io_SeekCurrent = 1;
static const int64_t io_SeekEnd = 2;

// EOF is the error returned by Read when no more input is available.
// (Read must return EOF itself, not an error wrapping EOF,
// because callers will test for EOF using ==.)
// Functions should return EOF only to signal a graceful end of input.
// If the EOF occurs unexpectedly in a structured data stream,
// the appropriate error is either [ErrUnexpectedEOF] or some other error
// giving more detail.
extern so_Error io_EOF;

// ErrInvalidWrite means that a write returned an impossible count.
extern so_Error io_ErrInvalidWrite;

// ErrNegativeRead means that a read returned a negative count.
extern so_Error io_ErrNegativeRead;

// ErrNoProgress is returned by some clients of a [Reader] when
// many calls to Read have failed to return any data or error,
// usually the sign of a broken [Reader] implementation.
extern so_Error io_ErrNoProgress;

// ErrOffset is returned by seek functions when the offset argument is invalid.
extern so_Error io_ErrOffset;

// ErrShortBuffer means that a read required a longer buffer than was provided.
extern so_Error io_ErrShortBuffer;

// ErrShortWrite means that a write accepted fewer bytes than requested
// but failed to return an explicit error.
extern so_Error io_ErrShortWrite;

// ErrUnexpectedEOF means that EOF was encountered in the
// middle of reading a fixed-size block or data structure.
extern so_Error io_ErrUnexpectedEOF;

// ErrUnread is returned by unread operations when they can't perform for some reason.
extern so_Error io_ErrUnread;

// ErrWhence is returned by seek functions when the whence argument is invalid.
extern so_Error io_ErrWhence;

// ErrClosedPipe is the error used for read or write operations on a closed pipe.
extern so_Error io_ErrClosedPipe;

// -- Functions and methods --
so_R_int_err io_DiscardWriter_Write(void* self, so_Slice p);
so_R_int_err io_DiscardWriter_WriteString(void* self, so_String s);

// LimitReader returns a LimitedReader that reads from r
// but stops with EOF after n bytes.
io_LimitedReader io_LimitReader(io_Reader r, int64_t n);
so_R_int_err io_LimitedReader_Read(void* self, so_Slice p);

// NewNopCloser returns a [NopCloser] wrapping r.
io_NopCloser io_NewNopCloser(io_Reader r);
so_R_int_err io_NopCloser_Read(void* self, so_Slice p);
so_Error io_NopCloser_Close(void* self);

// NewSectionReader returns a [SectionReader] that reads from r
// starting at offset off and stops with EOF after n bytes.
io_SectionReader io_NewSectionReader(io_ReaderAt r, int64_t off, int64_t n);
so_R_int_err io_SectionReader_Read(void* self, so_Slice p);
so_R_i64_err io_SectionReader_Seek(void* self, int64_t offset, so_int whence);
so_R_int_err io_SectionReader_ReadAt(void* self, so_Slice p, int64_t off);

// Size returns the size of the section in bytes.
int64_t io_SectionReader_Size(void* self);

// Outer returns the underlying [ReaderAt] and offsets for the section.
//
// The returned values are the same that were passed to [NewSectionReader]
// when the [SectionReader] was created.
io_ReaderAtOffset io_SectionReader_Outer(void* self);

// Copy copies from src to dst until either EOF is reached
// on src or an error occurs. It returns the number of bytes
// copied and the first error encountered while copying, if any.
//
// A successful Copy returns err == nil, not err == EOF.
// Because Copy is defined to read from src until EOF, it does
// not treat an EOF from Read as an error to be reported.
//
// Copy allocates a buffer on the stack to hold data during the copy.
so_R_i64_err io_Copy(io_Writer dst, io_Reader src);

// CopyBuffer is identical to Copy except that it stages through the
// provided buffer rather than allocating a temporary one.
// If buf is nil or has zero length, CopyBuffer panics.
so_R_i64_err io_CopyBuffer(io_Writer dst, io_Reader src, so_Slice buf);

// CopyN copies n bytes (or until an error) from src to dst.
// It returns the number of bytes copied and the earliest
// error encountered while copying.
// On return, written == n if and only if err == nil.
//
// Allocates a buffer on the stack to hold data during the copy.
so_R_i64_err io_CopyN(io_Writer dst, io_Reader src, int64_t n);

// ReadAll reads from r until an error or EOF and returns the data it read.
// A successful call returns err == nil, not err == EOF. Because ReadAll is
// defined to read from src until EOF, it does not treat an EOF from Read
// as an error to be reported.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_R_slice_err io_ReadAll(mem_Allocator a, io_Reader r);

// ReadFull reads exactly len(buf) bytes from r into buf.
// It returns the number of bytes copied and an error if fewer bytes were read.
// The error is EOF only if no bytes were read.
// If an EOF happens after reading some but not all the bytes,
// ReadFull returns [ErrUnexpectedEOF].
// On return, n == len(buf) if and only if err == nil.
// If r returns an error having read at least len(buf) bytes, the error is dropped.
so_R_int_err io_ReadFull(io_Reader r, so_Slice buf);

// WriteString writes the contents of the string s to w, which accepts a slice of bytes.
// [Writer.Write] is called exactly once.
so_R_int_err io_WriteString(io_Writer w, so_String s);
so_R_int_err io_MultiReader_Read(void* self, so_Slice p);
so_R_i64_err io_MultiReader_WriteTo(void* self, io_Writer w);

// NewMultiReader returns a Reader that's the logical concatenation of
// the provided input readers. They're read sequentially. Once all
// inputs have returned EOF, Read will return EOF.  If any of the readers
// return a non-nil, non-EOF error, Read will return that error.
io_MultiReader io_NewMultiReader(so_Slice readers);
so_R_int_err io_MultiWriter_Write(void* self, so_Slice p);
so_R_int_err io_MultiWriter_WriteString(void* self, so_String s);

// NewMultiWriter creates a writer that duplicates its writes to all the
// provided writers, similar to the Unix tee(1) command.
//
// Each write is written to each listed writer, one at a time.
// If a listed writer returns an error, that overall write operation
// stops and returns the error; it does not continue down the list.
io_MultiWriter io_NewMultiWriter(so_Slice writers);
