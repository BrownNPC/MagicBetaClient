#pragma once
#include "so/builtin/builtin.h"
#include "net/curl/curl.h"
#include "so/bytes/bytes.h"
#include "so/errors/errors.h"
#include "so/io/io.h"
#include "so/mem/mem.h"
#include "so/unicode/utf8/utf8.h"

// -- Types --

typedef struct net_BufferedReader net_BufferedReader;
typedef struct net_ReadLineResult net_ReadLineResult;
typedef struct net_Conn net_Conn;
typedef struct net_SteppedReader net_SteppedReader;
typedef struct net_SteppedReader16 net_SteppedReader16;
typedef struct net_SteppedReader32 net_SteppedReader32;
typedef struct net_SteppedReader64 net_SteppedReader64;

// BufferedReader implements buffering for an io.BufferedReader object.
//
// The caller is responsible for freeing the reader's resources
// with [BufferedReader.Free] when done using it.
typedef struct net_BufferedReader {
    mem_Allocator a;
    so_Slice buf;
    io_Reader rd;
    so_int r;
    so_int w;
    so_Error err;
    so_int lastByte;
    so_int lastRuneSize;
} net_BufferedReader;

// ReadLineResult holds the result of a [BufferedReader.ReadLine] call.
typedef struct net_ReadLineResult {
    so_Slice Line;
    bool IsPrefix;
    so_Error Err;
} net_ReadLineResult;

// Conn is a TCP client connection.
typedef struct net_Conn {
    bool closed;
    so_Error err;
    CURL* sock;
} net_Conn;

// A reader that reads in steps
// zero value is a valid reader.
typedef struct net_SteppedReader {
    so_byte Buf[1];
    so_int n;
} net_SteppedReader;

// A reader that reads in steps
// zero value will cause Step to always be a no-op.
typedef struct net_SteppedReader16 {
    so_byte Buf[2];
    so_int n;
} net_SteppedReader16;

// A reader that reads in steps
// zero value will cause Step to always be a no-op.
typedef struct net_SteppedReader32 {
    so_byte Buf[4];
    so_int n;
} net_SteppedReader32;

// A reader that reads in steps
// zero value will cause Step to always be a no-op.
typedef struct net_SteppedReader64 {
    so_byte Buf[8];
    so_int n;
} net_SteppedReader64;

// -- Result types --

typedef struct net_ConnResult {
    net_Conn val;
    so_Error err;
} net_ConnResult;

// -- Variables and constants --
static const int64_t net_DefaultBufSize = 4096 * 10;
extern so_Error net_ErrInvalidUnreadByte;
extern so_Error net_ErrInvalidUnreadRune;
extern so_Error net_ErrBufferFull;
extern so_Error net_ErrNegativeCount;
extern so_Error net_ErrConnectionClosed;

// -- Functions and methods --

// NewBufferedReaderSize returns a new [BufferedReader] whose buffer has at least the specified
// size. If the argument io.Reader is already a [BufferedReader] with large enough
// size, it returns the underlying [BufferedReader].
net_BufferedReader net_NewBufferedReaderSize(mem_Allocator a, io_Reader rd, so_int size);

// NewBufferedReader returns a new [BufferedReader] whose buffer has the default size.
net_BufferedReader net_NewBufferedReader(mem_Allocator a, io_Reader rd);

// Size returns the size of the underlying buffer in bytes.
so_int net_BufferedReader_Size(void* self);

// Reset discards any buffered data, resets all state, and switches
// the buffered reader to read from r.
// Calling Reset on the zero value of [BufferedReader] initializes the internal buffer
// to the default size.
// Calling b.Reset(b) (that is, resetting a [BufferedReader] to itself) does nothing.
void net_BufferedReader_Reset(void* self, io_Reader r);

// Free releases the internal reader's buffer.
// The reader must not be used after calling Free.
void net_BufferedReader_Free(void* self);

// Peek returns the next n bytes without advancing the reader. The bytes stop
// being valid at the next read call. If necessary, Peek will read more bytes
// into the buffer in order to make n bytes available. If Peek returns fewer
// than n bytes, it also returns an error explaining why the read is short.
// The error is [ErrBufferFull] if n is larger than b's buffer size.
//
// Calling Peek prevents a [BufferedReader.UnreadByte] or [BufferedReader.UnreadRune] call from succeeding
// until the next read operation.
so_R_slice_err net_BufferedReader_Peek(void* self, so_int n);

// Discard skips the next n bytes, returning the number of bytes discarded.
//
// If Discard skips fewer than n bytes, it also returns an error.
// If 0 <= n <= b.Buffered(), Discard is guaranteed to succeed without
// reading from the underlying io.Reader.
so_R_int_err net_BufferedReader_Discard(void* self, so_int n);

// Read reads data into p.
// It returns the number of bytes read into p.
// The bytes are taken from at most one Read on the underlying [BufferedReader],
// hence n may be less than len(p).
// To read exactly len(p) bytes, use io.ReadFull(b, p).
// If the underlying [BufferedReader] can return a non-zero count with io.EOF,
// then this Read method can do so as well; see the [io.Reader] docs.
so_R_int_err net_BufferedReader_Read(void* self, so_Slice p);

// ReadByte reads and returns a single byte.
// If no byte is available, returns an error.
so_R_byte_err net_BufferedReader_ReadByte(void* self);

// UnreadByte unreads the last byte. Only the most recently read byte can be unread.
//
// UnreadByte returns an error if the most recent method called on the
// [BufferedReader] was not a read operation. Notably, [BufferedReader.Peek], [BufferedReader.Discard], and [BufferedReader.WriteTo] are not
// considered read operations.
so_Error net_BufferedReader_UnreadByte(void* self);

// ReadRune reads a single UTF-8 encoded Unicode character and returns the
// rune and its size in bytes. If the encoded rune is invalid, it consumes one byte
// and returns unicode.ReplacementChar (U+FFFD) with a size of 1.
io_RuneSizeResult net_BufferedReader_ReadRune(void* self);

// UnreadRune unreads the last rune. If the most recent method called on
// the [BufferedReader] was not a [BufferedReader.ReadRune], [BufferedReader.UnreadRune] returns an error. (In this
// regard it is stricter than [BufferedReader.UnreadByte], which will unread the last byte
// from any read operation.)
so_Error net_BufferedReader_UnreadRune(void* self);

// Buffered returns the number of bytes that can be read from the current buffer.
so_int net_BufferedReader_Buffered(void* self);

// ReadSlice reads until the first occurrence of delim in the input,
// returning a slice pointing at the bytes in the buffer.
// The bytes stop being valid at the next read.
// If ReadSlice encounters an error before finding a delimiter,
// it returns all the data in the buffer and the error itself (often io.EOF).
// ReadSlice fails with error [ErrBufferFull] if the buffer fills without a delim.
// Because the data returned from ReadSlice will be overwritten
// by the next I/O operation, most clients should use
// [BufferedReader.ReadBytes] or ReadString instead.
// ReadSlice returns err != nil if and only if line does not end in delim.
so_R_slice_err net_BufferedReader_ReadSlice(void* self, so_byte delim);

// ReadLine is a low-level line-reading primitive. Most callers should use
// [BufferedReader.ReadBytes]('\n') or [BufferedReader.ReadString]('\n') instead or use a [Scanner].
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
// Calling [BufferedReader.UnreadByte] after ReadLine will always unread the last byte read
// (possibly a character belonging to the line end) even if that byte is not
// part of the line returned by ReadLine.
net_ReadLineResult net_BufferedReader_ReadLine(void* self);

// ReadBytes reads until the first occurrence of delim in the input,
// returning a slice containing the data up to and including the delimiter.
// If ReadBytes encounters an error before finding a delimiter,
// it returns the data read before the error and the error itself (often io.EOF).
// ReadBytes returns err != nil if and only if the returned data does not end in
// delim.
// For simple uses, a Scanner may be more convenient.
//
// The returned slice is allocated; the caller owns it.
so_R_slice_err net_BufferedReader_ReadBytes(void* self, so_byte delim);

// ReadString reads until the first occurrence of delim in the input,
// returning a string containing the data up to and including the delimiter.
// If ReadString encounters an error before finding a delimiter,
// it returns the data read before the error and the error itself (often io.EOF).
// ReadString returns err != nil if and only if the returned data does not end in
// delim.
// For simple uses, a Scanner may be more convenient.
//
// The returned string is allocated; the caller owns it.
so_R_str_err net_BufferedReader_ReadString(void* self, so_byte delim);

// WriteTo implements io.WriterTo.
// This may make multiple calls to the [BufferedReader.Read] method of the underlying [BufferedReader].
so_R_i64_err net_BufferedReader_WriteTo(void* self, io_Writer w);

// Read can return (0,nil) if there is nothing to be read.
// Read never blocks.
so_R_int_err net_Conn_Read(void* self, so_Slice b);

// Write blocks until all bytes from the buffer have been written.
so_R_int_err net_Conn_Write(void* self, so_Slice b);

// Free Closes the connection and frees memory.
void net_Conn_Close(void* self);

// Dial dials the connection with a default timeout.
net_ConnResult net_Dial(so_String host);

// Will read sizeof(T) bytes.
// Step is a no-op if the reading has completed.
so_R_bool_err net_SteppedReader_Step(void* self, net_BufferedReader* rd);

// // makes the steppedReader reusable
void net_SteppedReader_Reset(void* self);

// return size of the value being read.
so_int net_SteppedReader_Len(void* self);

// Will read sizeof(T) bytes.
// Step is a no-op if the reading has completed.
so_R_bool_err net_SteppedReader16_Step(void* self, net_BufferedReader* rd);

// // makes the steppedReader reusable
void net_SteppedReader16_Reset(void* self);

// return size of the value being read.
so_int net_SteppedReader16_Len(void* self);

// Will read sizeof(T) bytes.
// Step is a no-op if the reading has completed.
so_R_bool_err net_SteppedReader32_Step(void* self, net_BufferedReader* rd);

// // makes the steppedReader reusable
void net_SteppedReader32_Reset(void* self);

// return size of the value being read.
so_int net_SteppedReader32_Len(void* self);
so_int net_SteppedReader64_N(net_SteppedReader64 s);

// Will read sizeof(T) bytes.
// Step is a no-op if the reading has completed.
so_R_bool_err net_SteppedReader64_Step(void* self, net_BufferedReader* rd);

// // makes the steppedReader reusable
void net_SteppedReader64_Reset(void* self);
so_int net_SteppedReader64_Len(void* self);
