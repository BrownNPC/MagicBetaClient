#pragma once
#include "so/builtin/builtin.h"
#include "so/bytes/bytes.h"
#include "so/errors/errors.h"
#include "so/io/io.h"
#include "so/mem/mem.h"
#include "so/unicode/utf8/utf8.h"

// -- Types --

typedef struct bufio_Reader bufio_Reader;
typedef struct bufio_ReadLineResult bufio_ReadLineResult;
typedef struct bufio_SplitResult bufio_SplitResult;
typedef struct bufio_Scanner bufio_Scanner;
typedef struct bufio_Writer bufio_Writer;
typedef struct bufio_ReadWriter bufio_ReadWriter;

// Reader implements buffering for an io.Reader object.
//
// The caller is responsible for freeing the reader's resources
// with [Reader.Free] when done using it.
typedef struct bufio_Reader {
    mem_Allocator a;
    so_Slice buf;
    io_Reader rd;
    so_int r;
    so_int w;
    so_Error err;
    so_int lastByte;
    so_int lastRuneSize;
} bufio_Reader;

// ReadLineResult holds the result of a [Reader.ReadLine] call.
typedef struct bufio_ReadLineResult {
    so_Slice Line;
    bool IsPrefix;
    so_Error Err;
} bufio_ReadLineResult;

// SplitResult holds the return values from a [SplitFunc].
typedef struct bufio_SplitResult {
    so_int Advance;
    so_Slice Token;
    bool HasToken;
    so_Error Err;
} bufio_SplitResult;

// SplitFunc is the signature of the split function used to tokenize the
// input. The arguments are an initial substring of the remaining unprocessed
// data and a flag, atEOF, that reports whether the [Reader] has no more data
// to give. The return value is a [SplitResult] containing the number of bytes
// to advance the input, the next token to return to the user (if any),
// and an error (if any).
//
// Scanning stops if the function returns an error, in which case some of
// the input may be discarded. If that error is [ErrFinalToken], scanning
// stops with no error. A token delivered with [ErrFinalToken] where
// HasToken is true will be the last token, and a result with HasToken
// false and [ErrFinalToken] immediately stops the scanning.
//
// Otherwise, the [Scanner] advances the input. If HasToken is true,
// the [Scanner] returns the token to the user. If HasToken is false, the
// Scanner reads more data and continues scanning; if there is no more
// data -- if atEOF was true -- the [Scanner] returns. If the data does not
// yet hold a complete token, for instance if it has no newline while
// scanning lines, a [SplitFunc] can return an empty [SplitResult] to signal the
// [Scanner] to read more data into the slice and try again with a
// longer slice starting at the same point in the input.
//
// The function is never called with an empty data slice unless atEOF
// is true. If atEOF is true, however, data may be non-empty and,
// as always, holds unprocessed text.
typedef bufio_SplitResult (*bufio_SplitFunc)(so_Slice, bool);

// Scanner provides a convenient interface for reading data such as
// a file of newline-delimited lines of text. Successive calls to
// the [Scanner.Scan] method will step through the 'tokens' of a file, skipping
// the bytes between the tokens. The specification of a token is
// defined by a split function of type [SplitFunc]; the default split
// function breaks the input into lines with line termination stripped. [Scanner.Split]
// functions are defined in this package for scanning a file into
// lines, bytes, UTF-8-encoded runes, and space-delimited words. The
// client may instead provide a custom split function.
//
// Scanning stops unrecoverably at EOF, the first I/O error, or a token too
// large to fit in the [Scanner.Buffer]. When a scan stops, the reader may have
// advanced arbitrarily far past the last token. Programs that need more
// control over error handling or large tokens, or must run sequential scans
// on a reader, should use [bufio.Reader] instead.
typedef struct bufio_Scanner {
    mem_Allocator a;
    io_Reader r;
    bufio_SplitFunc split;
    so_int maxTokenSize;
    so_Slice token;
    so_Slice buf;
    so_int start;
    so_int end;
    so_Error err;
    so_int empties;
    bool scanCalled;
    bool done;
    bool ownsBuf;
} bufio_Scanner;

// Writer implements buffering for an [io.Writer] object.
// If an error occurs writing to a [Writer], no more data will be
// accepted and all subsequent writes, and [Writer.Flush], will return the error.
// After all data has been written, the client should call the
// [Writer.Flush] method to guarantee all data has been forwarded to
// the underlying [io.Writer].
//
// The caller is responsible for freeing the writer's resources
// with [Writer.Free] when done using it.
typedef struct bufio_Writer {
    mem_Allocator a;
    so_Error err;
    so_Slice buf;
    so_int n;
    io_Writer wr;
} bufio_Writer;

// ReadWriter stores pointers to a [Reader] and a [Writer].
// It implements [io.ReadWriter].
typedef struct bufio_ReadWriter {
    bufio_Reader* r;
    bufio_Writer* w;
} bufio_ReadWriter;

// -- Variables and constants --

// DefaultBufSize is the default buffer size used by [NewReader] and [NewWriter].
static const int64_t bufio_DefaultBufSize = 4096;
extern so_Error bufio_ErrInvalidUnreadByte;
extern so_Error bufio_ErrInvalidUnreadRune;
extern so_Error bufio_ErrBufferFull;
extern so_Error bufio_ErrNegativeCount;

// Errors returned by Scanner.
extern so_Error bufio_ErrTooLong;
extern so_Error bufio_ErrNegativeAdvance;
extern so_Error bufio_ErrAdvanceTooFar;
extern so_Error bufio_ErrBadReadCount;
static const int64_t bufio_MaxScanTokenSize = 64 * 1024;

// ErrFinalToken is a special sentinel error value. It is intended to be
// returned by a Split function to indicate that the scanning should stop
// with no error. If the token being delivered with this error has HasToken
// true, the token is the last token.
//
// The value is useful to stop processing early or when it is necessary to
// deliver a final empty token. One could achieve the same behavior
// with a custom error value but providing one here is tidier.
extern so_Error bufio_ErrFinalToken;

// -- Functions and methods --

// NewReaderSize returns a new [Reader] whose buffer has at least the specified
// size. If the argument io.Reader is already a [Reader] with large enough
// size, it returns the underlying [Reader].
bufio_Reader bufio_NewReaderSize(mem_Allocator a, io_Reader rd, so_int size);

// NewReader returns a new [Reader] whose buffer has the default size.
bufio_Reader bufio_NewReader(mem_Allocator a, io_Reader rd);

// Size returns the size of the underlying buffer in bytes.
so_int bufio_Reader_Size(void* self);

// Reset discards any buffered data, resets all state, and switches
// the buffered reader to read from r.
// Calling Reset on the zero value of [Reader] initializes the internal buffer
// to the default size.
// Calling b.Reset(b) (that is, resetting a [Reader] to itself) does nothing.
void bufio_Reader_Reset(void* self, io_Reader r);

// Free releases the internal reader's buffer.
// The reader must not be used after calling Free.
void bufio_Reader_Free(void* self);

// Peek returns the next n bytes without advancing the reader. The bytes stop
// being valid at the next read call. If necessary, Peek will read more bytes
// into the buffer in order to make n bytes available. If Peek returns fewer
// than n bytes, it also returns an error explaining why the read is short.
// The error is [ErrBufferFull] if n is larger than b's buffer size.
//
// Calling Peek prevents a [Reader.UnreadByte] or [Reader.UnreadRune] call from succeeding
// until the next read operation.
so_R_slice_err bufio_Reader_Peek(void* self, so_int n);

// Discard skips the next n bytes, returning the number of bytes discarded.
//
// If Discard skips fewer than n bytes, it also returns an error.
// If 0 <= n <= b.Buffered(), Discard is guaranteed to succeed without
// reading from the underlying io.Reader.
so_R_int_err bufio_Reader_Discard(void* self, so_int n);

// Read reads data into p.
// It returns the number of bytes read into p.
// The bytes are taken from at most one Read on the underlying [Reader],
// hence n may be less than len(p).
// To read exactly len(p) bytes, use io.ReadFull(b, p).
// If the underlying [Reader] can return a non-zero count with io.EOF,
// then this Read method can do so as well; see the [io.Reader] docs.
so_R_int_err bufio_Reader_Read(void* self, so_Slice p);

// ReadByte reads and returns a single byte.
// If no byte is available, returns an error.
so_R_byte_err bufio_Reader_ReadByte(void* self);

// UnreadByte unreads the last byte. Only the most recently read byte can be unread.
//
// UnreadByte returns an error if the most recent method called on the
// [Reader] was not a read operation. Notably, [Reader.Peek], [Reader.Discard], and [Reader.WriteTo] are not
// considered read operations.
so_Error bufio_Reader_UnreadByte(void* self);

// ReadRune reads a single UTF-8 encoded Unicode character and returns the
// rune and its size in bytes. If the encoded rune is invalid, it consumes one byte
// and returns unicode.ReplacementChar (U+FFFD) with a size of 1.
io_RuneSizeResult bufio_Reader_ReadRune(void* self);

// UnreadRune unreads the last rune. If the most recent method called on
// the [Reader] was not a [Reader.ReadRune], [Reader.UnreadRune] returns an error. (In this
// regard it is stricter than [Reader.UnreadByte], which will unread the last byte
// from any read operation.)
so_Error bufio_Reader_UnreadRune(void* self);

// Buffered returns the number of bytes that can be read from the current buffer.
so_int bufio_Reader_Buffered(void* self);

// ReadSlice reads until the first occurrence of delim in the input,
// returning a slice pointing at the bytes in the buffer.
// The bytes stop being valid at the next read.
// If ReadSlice encounters an error before finding a delimiter,
// it returns all the data in the buffer and the error itself (often io.EOF).
// ReadSlice fails with error [ErrBufferFull] if the buffer fills without a delim.
// Because the data returned from ReadSlice will be overwritten
// by the next I/O operation, most clients should use
// [Reader.ReadBytes] or ReadString instead.
// ReadSlice returns err != nil if and only if line does not end in delim.
so_R_slice_err bufio_Reader_ReadSlice(void* self, so_byte delim);

// ReadLine is a low-level line-reading primitive. Most callers should use
// [Reader.ReadBytes]('\n') or [Reader.ReadString]('\n') instead or use a [Scanner].
//
// ReadLine tries to return a single line, not including the end-of-line bytes.
// If the line was too long for the buffer then isPrefix is set and the
// beginning of the line is returned. The rest of the line will be returned
// from future calls. isPrefix will be false when returning the last fragment
// of the line. The returned buffer is only valid until the next call to
// ReadLine. ReadLine either returns a non-nil line or it returns an error,
// never both.
//
// The text returned from ReadLine does not include the line end ("\r\n" or "\n").
// No indication or error is given if the input ends without a final line end.
// Calling [Reader.UnreadByte] after ReadLine will always unread the last byte read
// (possibly a character belonging to the line end) even if that byte is not
// part of the line returned by ReadLine.
bufio_ReadLineResult bufio_Reader_ReadLine(void* self);

// ReadBytes reads until the first occurrence of delim in the input,
// returning a slice containing the data up to and including the delimiter.
// If ReadBytes encounters an error before finding a delimiter,
// it returns the data read before the error and the error itself (often io.EOF).
// ReadBytes returns err != nil if and only if the returned data does not end in
// delim.
// For simple uses, a Scanner may be more convenient.
//
// The returned slice is allocated; the caller owns it.
so_R_slice_err bufio_Reader_ReadBytes(void* self, so_byte delim);

// ReadString reads until the first occurrence of delim in the input,
// returning a string containing the data up to and including the delimiter.
// If ReadString encounters an error before finding a delimiter,
// it returns the data read before the error and the error itself (often io.EOF).
// ReadString returns err != nil if and only if the returned data does not end in
// delim.
// For simple uses, a Scanner may be more convenient.
//
// The returned string is allocated; the caller owns it.
so_R_str_err bufio_Reader_ReadString(void* self, so_byte delim);

// WriteTo implements io.WriterTo.
// This may make multiple calls to the [Reader.Read] method of the underlying [Reader].
so_R_i64_err bufio_Reader_WriteTo(void* self, io_Writer w);

// NewScanner returns a new [Scanner] to read from r.
// The split function defaults to [ScanLines].
//
// The caller is responsible for freeing the scanner resources
// with [Scanner.Free] when done using it.
bufio_Scanner bufio_NewScanner(mem_Allocator a, io_Reader r);

// Free releases the internal scanner buffer.
// It is safe to call Free on a scanner that used a user-provided buffer
// via [Scanner.Buffer]; in that case Free is a no-op.
void bufio_Scanner_Free(void* self);

// Err returns the first non-EOF error that was encountered by the [Scanner].
so_Error bufio_Scanner_Err(void* self);

// Bytes returns the most recent token generated by a call to [Scanner.Scan].
// The underlying array may point to data that will be overwritten
// by a subsequent call to Scan. It does no allocation.
so_Slice bufio_Scanner_Bytes(void* self);

// Text returns the most recent token generated by a call to [Scanner.Scan]
// as a string. The returned string is a zero-copy view into the buffer and
// is invalidated by the next call to [Scanner.Scan].
so_String bufio_Scanner_Text(void* self);

// Scan advances the [Scanner] to the next token, which will then be
// available through the [Scanner.Bytes] or [Scanner.Text] method. It returns false when
// there are no more tokens, either by reaching the end of the input or an error.
// After Scan returns false, the [Scanner.Err] method will return any error that
// occurred during scanning, except that if it was [io.EOF], [Scanner.Err]
// will return nil.
// Scan panics if the split function returns too many empty
// tokens without advancing the input. This is a common error mode for
// scanners.
bool bufio_Scanner_Scan(void* self);

// Buffer controls memory allocation by the Scanner.
// It sets the initial buffer to use when scanning
// and the maximum size of buffer that may be allocated during scanning.
// The contents of the buffer are ignored.
//
// The maximum token size must be less than the larger of max and cap(buf).
// If max <= cap(buf), [Scanner.Scan] will use this buffer only and do no allocation.
//
// By default, [Scanner.Scan] uses an internal buffer and sets the
// maximum token size to [MaxScanTokenSize].
//
// Buffer panics if it is called after scanning has started.
void bufio_Scanner_Buffer(void* self, so_Slice buf, so_int max);

// Split sets the split function for the [Scanner].
// The default split function is [ScanLines].
//
// Split panics if it is called after scanning has started.
void bufio_Scanner_Split(void* self, bufio_SplitFunc split);

// Split functions
// ScanBytes is a split function for a [Scanner] that returns each byte as a token.
bufio_SplitResult bufio_ScanBytes(so_Slice data, bool atEOF);

// ScanRunes is a split function for a [Scanner] that returns each
// UTF-8-encoded rune as a token. The sequence of runes returned is
// equivalent to that from a range loop over the input as a string, which
// means that erroneous UTF-8 encodings translate to U+FFFD = "\xef\xbf\xbd".
// Because of the Scan interface, this makes it impossible for the client to
// distinguish correctly encoded replacement runes from encoding errors.
bufio_SplitResult bufio_ScanRunes(so_Slice data, bool atEOF);

// ScanLines is a split function for a [Scanner] that returns each line of
// text, stripped of any trailing end-of-line marker. The returned line may
// be empty. The end-of-line marker is one optional carriage return followed
// by one mandatory newline. In regular expression notation, it is `\r?\n`.
// The last non-empty line of input will be returned even if it has no
// newline.
bufio_SplitResult bufio_ScanLines(so_Slice data, bool atEOF);

// ScanWords is a split function for a [Scanner] that returns each
// space-separated word of text, with surrounding spaces deleted. It will
// never return an empty string. The definition of space is set by
// unicode.IsSpace.
bufio_SplitResult bufio_ScanWords(so_Slice data, bool atEOF);

// NewWriterSize returns a new [Writer] whose buffer has at least the specified
// size. If the argument io.Writer is already a [Writer] with large enough
// size, it returns the underlying [Writer].
bufio_Writer bufio_NewWriterSize(mem_Allocator a, io_Writer w, so_int size);

// NewWriter returns a new [Writer] whose buffer has the default size.
// If the argument io.Writer is already a [Writer] with large enough buffer size,
// it returns the underlying [Writer].
bufio_Writer bufio_NewWriter(mem_Allocator a, io_Writer w);

// Size returns the size of the underlying buffer in bytes.
so_int bufio_Writer_Size(void* self);

// Reset discards any unflushed buffered data, clears any error, and
// resets b to write its output to w.
// Calling Reset on the zero value of [Writer] initializes the internal buffer
// to the default size.
// Calling w.Reset(w) (that is, resetting a [Writer] to itself) does nothing.
void bufio_Writer_Reset(void* self, io_Writer w);

// Free releases the internal buffer memory.
// The writer must not be used after calling Free.
void bufio_Writer_Free(void* self);

// Flush writes any buffered data to the underlying [io.Writer].
so_Error bufio_Writer_Flush(void* self);

// Available returns how many bytes are unused in the buffer.
so_int bufio_Writer_Available(void* self);

// AvailableBuffer returns an empty buffer with b.Available() capacity.
// This buffer is intended to be appended to and
// passed to an immediately succeeding [Writer.Write] call.
// The buffer is only valid until the next write operation on b.
so_Slice bufio_Writer_AvailableBuffer(void* self);

// Buffered returns the number of bytes that have been written into the current buffer.
so_int bufio_Writer_Buffered(void* self);

// Write writes the contents of p into the buffer.
// It returns the number of bytes written.
// If nn < len(p), it also returns an error explaining
// why the write is short.
so_R_int_err bufio_Writer_Write(void* self, so_Slice p);

// WriteByte writes a single byte.
so_Error bufio_Writer_WriteByte(void* self, so_byte c);

// WriteRune writes a single Unicode code point, returning
// the number of bytes written and any error.
so_R_int_err bufio_Writer_WriteRune(void* self, so_rune r);

// WriteString writes a string.
// It returns the number of bytes written.
// If the count is less than len(s), it also returns an error explaining
// why the write is short.
so_R_int_err bufio_Writer_WriteString(void* self, so_String s);

// ReadFrom implements [io.ReaderFrom].
so_R_i64_err bufio_Writer_ReadFrom(void* self, io_Reader r);

// NewReadWriter allocates a new [ReadWriter] that dispatches to r and w.
bufio_ReadWriter bufio_NewReadWriter(bufio_Reader* r, bufio_Writer* w);
so_R_int_err bufio_ReadWriter_Read(void* self, so_Slice p);
so_R_int_err bufio_ReadWriter_Write(void* self, so_Slice p);
