#include "bufio.h"

// -- Forward declarations --
static void bufio_Reader_reset(void* self, so_Slice buf, io_Reader r);
static void bufio_Reader_fill(void* self);
static so_Error bufio_Reader_readErr(void* self);
static so_R_i64_err bufio_Reader_writeBuf(void* self, io_Writer w);
static bool bufio_Scanner_advance(void* self, so_int n);
static void bufio_Scanner_setErr(void* self, so_Error err);
static so_Slice dropCR(so_Slice data);
static bool isSpace(so_rune r);

// -- Variables and constants --
so_Error bufio_ErrInvalidUnreadByte = errors_New("bufio: invalid use of UnreadByte");
so_Error bufio_ErrInvalidUnreadRune = errors_New("bufio: invalid use of UnreadRune");
so_Error bufio_ErrBufferFull = errors_New("bufio: buffer full");
so_Error bufio_ErrNegativeCount = errors_New("bufio: negative count");
static const int64_t minReadBufferSize = 16;
static const int64_t maxConsecutiveEmptyReads = 100;
static so_Error errNegativeRead = errors_New("bufio: reader returned negative count from Read");
static so_Error errNegativeWrite = errors_New("bufio: writer returned negative count from Write");
static const so_int maxInt = (so_int)((uint64_t)(~(so_uint)(0)) >> 1);

// Errors returned by Scanner.
so_Error bufio_ErrTooLong = errors_New("bufio.Scanner: token too long");
so_Error bufio_ErrNegativeAdvance = errors_New("bufio.Scanner: SplitFunc returns negative advance count");
so_Error bufio_ErrAdvanceTooFar = errors_New("bufio.Scanner: SplitFunc returns advance count beyond input");
so_Error bufio_ErrBadReadCount = errors_New("bufio.Scanner: Read returned impossible count");
static const int64_t startBufSize = 4096;

// ErrFinalToken is a special sentinel error value. It is intended to be
// returned by a Split function to indicate that the scanning should stop
// with no error. If the token being delivered with this error has HasToken
// true, the token is the last token.
//
// The value is useful to stop processing early or when it is necessary to
// deliver a final empty token. One could achieve the same behavior
// with a custom error value but providing one here is tidier.
so_Error bufio_ErrFinalToken = errors_New("final token");

// errorRune is the UTF-8 encoding of U+FFFD (replacement character).
static so_byte errorRune[3] = {0xef, 0xbf, 0xbd};

// -- bufio.go --

// -- reader.go --

// NewReaderSize returns a new [Reader] whose buffer has at least the specified
// size. If the argument io.Reader is already a [Reader] with large enough
// size, it returns the underlying [Reader].
bufio_Reader bufio_NewReaderSize(mem_Allocator a, io_Reader rd, so_int size) {
    // Is it already a Reader?
    {
        bool ok = (rd.Read == bufio_Reader_Read);
        if (ok) {
            bufio_Reader* b = (bufio_Reader*)rd.self;
            if (so_len(b->buf) >= size) {
                return *b;
            }
        }
    }
    so_int sz = so_max(size, minReadBufferSize);
    so_Slice buf = mem_AllocSlice(so_byte, (a), (sz), (sz));
    bufio_Reader r = (bufio_Reader){.a = a};
    bufio_Reader_reset(&r, buf, rd);
    return r;
}

// NewReader returns a new [Reader] whose buffer has the default size.
bufio_Reader bufio_NewReader(mem_Allocator a, io_Reader rd) {
    return bufio_NewReaderSize(a, rd, bufio_DefaultBufSize);
}

// Size returns the size of the underlying buffer in bytes.
so_int bufio_Reader_Size(void* self) {
    bufio_Reader* b = self;
    return so_len(b->buf);
}

// Reset discards any buffered data, resets all state, and switches
// the buffered reader to read from r.
// Calling Reset on the zero value of [Reader] initializes the internal buffer
// to the default size.
// Calling b.Reset(b) (that is, resetting a [Reader] to itself) does nothing.
void bufio_Reader_Reset(void* self, io_Reader r) {
    bufio_Reader* b = self;
    // Avoid no-op reset to self.
    bool ok = (r.Read == bufio_Reader_Read);
    if (ok && b == (bufio_Reader*)r.self) {
        return;
    }
    if (b->buf.ptr == NULL) {
        b->buf = mem_AllocSlice(so_byte, (b->a), (bufio_DefaultBufSize), (bufio_DefaultBufSize));
    }
    bufio_Reader_reset(b, b->buf, r);
}

// Free releases the internal reader's buffer.
// The reader must not be used after calling Free.
void bufio_Reader_Free(void* self) {
    bufio_Reader* b = self;
    mem_FreeSlice(so_byte, (b->a), (b->buf));
    b->buf = (so_Slice){0};
}

static void bufio_Reader_reset(void* self, so_Slice buf, io_Reader r) {
    bufio_Reader* b = self;
    b->buf = buf;
    b->rd = r;
    b->lastByte = -1;
    b->lastRuneSize = -1;
}

// fill reads a new chunk into the buffer.
static void bufio_Reader_fill(void* self) {
    bufio_Reader* b = self;
    // Slide existing data to beginning.
    if (b->r > 0) {
        so_copy(so_byte, b->buf, so_slice(so_byte, b->buf, b->r, b->w));
        b->w -= b->r;
        b->r = 0;
    }
    if (b->w >= so_len(b->buf)) {
        so_panic("bufio: tried to fill full buffer");
    }
    // Read new data: try a limited number of times.
    for (so_int i = maxConsecutiveEmptyReads; i > 0; i--) {
        so_R_int_err _res1 = b->rd.Read(b->rd.self, so_slice(so_byte, b->buf, b->w, b->buf.len));
        so_int n = _res1.val;
        so_Error err = _res1.err;
        if (n < 0) {
            so_panic(so_error_cstr(errNegativeRead));
        }
        b->w += n;
        if (err.self != NULL) {
            b->err = err;
            return;
        }
        if (n > 0) {
            return;
        }
    }
    b->err = io_ErrNoProgress;
}

static so_Error bufio_Reader_readErr(void* self) {
    bufio_Reader* b = self;
    so_Error err = b->err;
    b->err = (so_Error){0};
    return err;
}

// Peek returns the next n bytes without advancing the reader. The bytes stop
// being valid at the next read call. If necessary, Peek will read more bytes
// into the buffer in order to make n bytes available. If Peek returns fewer
// than n bytes, it also returns an error explaining why the read is short.
// The error is [ErrBufferFull] if n is larger than b's buffer size.
//
// Calling Peek prevents a [Reader.UnreadByte] or [Reader.UnreadRune] call from succeeding
// until the next read operation.
so_R_slice_err bufio_Reader_Peek(void* self, so_int n) {
    bufio_Reader* b = self;
    if (n < 0) {
        return (so_R_slice_err){.val = (so_Slice){0}, .err = bufio_ErrNegativeCount};
    }
    b->lastByte = -1;
    b->lastRuneSize = -1;
    for (; b->w - b->r < n && b->w - b->r < so_len(b->buf) && b->err.self == NULL;) {
        // b.w-b.r < len(b.buf) => buffer is not full
        bufio_Reader_fill(b);
    }
    if (n > so_len(b->buf)) {
        return (so_R_slice_err){.val = so_slice(so_byte, b->buf, b->r, b->w), .err = bufio_ErrBufferFull};
    }
    // 0 <= n <= len(b.buf)
    so_Error err = {0};
    {
        so_int avail = b->w - b->r;
        if (avail < n) {
            // not enough data in buffer
            n = avail;
            err = bufio_Reader_readErr(b);
            if (err.self == NULL) {
                err = bufio_ErrBufferFull;
            }
        }
    }
    return (so_R_slice_err){.val = so_slice(so_byte, b->buf, b->r, b->r + n), .err = err};
}

// Discard skips the next n bytes, returning the number of bytes discarded.
//
// If Discard skips fewer than n bytes, it also returns an error.
// If 0 <= n <= b.Buffered(), Discard is guaranteed to succeed without
// reading from the underlying io.Reader.
so_R_int_err bufio_Reader_Discard(void* self, so_int n) {
    bufio_Reader* b = self;
    if (n < 0) {
        return (so_R_int_err){.val = 0, .err = bufio_ErrNegativeCount};
    }
    if (n == 0) {
        return (so_R_int_err){.val = 0, .err = (so_Error){0}};
    }
    b->lastByte = -1;
    b->lastRuneSize = -1;
    so_int remain = n;
    for (;;) {
        so_int skip = bufio_Reader_Buffered(b);
        if (skip == 0) {
            bufio_Reader_fill(b);
            skip = bufio_Reader_Buffered(b);
        }
        if (skip > remain) {
            skip = remain;
        }
        b->r += skip;
        remain -= skip;
        if (remain == 0) {
            return (so_R_int_err){.val = n, .err = (so_Error){0}};
        }
        if (b->err.self != NULL) {
            return (so_R_int_err){.val = n - remain, .err = bufio_Reader_readErr(b)};
        }
    }
}

// Read reads data into p.
// It returns the number of bytes read into p.
// The bytes are taken from at most one Read on the underlying [Reader],
// hence n may be less than len(p).
// To read exactly len(p) bytes, use io.ReadFull(b, p).
// If the underlying [Reader] can return a non-zero count with io.EOF,
// then this Read method can do so as well; see the [io.Reader] docs.
so_R_int_err bufio_Reader_Read(void* self, so_Slice p) {
    bufio_Reader* b = self;
    so_int n = so_len(p);
    if (n == 0) {
        if (bufio_Reader_Buffered(b) > 0) {
            return (so_R_int_err){.val = 0, .err = (so_Error){0}};
        }
        return (so_R_int_err){.val = 0, .err = bufio_Reader_readErr(b)};
    }
    if (b->r == b->w) {
        if (b->err.self != NULL) {
            return (so_R_int_err){.val = 0, .err = bufio_Reader_readErr(b)};
        }
        if (so_len(p) >= so_len(b->buf)) {
            // Large read, empty buffer.
            // Read directly into p to avoid copy.
            so_R_int_err _res1 = b->rd.Read(b->rd.self, p);
            n = _res1.val;
            b->err = _res1.err;
            if (n < 0) {
                so_panic(so_error_cstr(errNegativeRead));
            }
            if (n > 0) {
                b->lastByte = (so_int)(so_at(so_byte, p, n - 1));
                b->lastRuneSize = -1;
            }
            return (so_R_int_err){.val = n, .err = bufio_Reader_readErr(b)};
        }
        // One read.
        // Do not use b.fill, which will loop.
        b->r = 0;
        b->w = 0;
        so_R_int_err _res2 = b->rd.Read(b->rd.self, b->buf);
        n = _res2.val;
        b->err = _res2.err;
        if (n < 0) {
            so_panic(so_error_cstr(errNegativeRead));
        }
        if (n == 0) {
            return (so_R_int_err){.val = 0, .err = bufio_Reader_readErr(b)};
        }
        b->w += n;
    }
    // copy as much as we can
    // Note: if the slice panics here, it is probably because
    // the underlying reader returned a bad count. See issue 49795.
    n = so_copy(so_byte, p, so_slice(so_byte, b->buf, b->r, b->w));
    b->r += n;
    b->lastByte = (so_int)(so_at(so_byte, b->buf, b->r - 1));
    b->lastRuneSize = -1;
    return (so_R_int_err){.val = n, .err = (so_Error){0}};
}

// ReadByte reads and returns a single byte.
// If no byte is available, returns an error.
so_R_byte_err bufio_Reader_ReadByte(void* self) {
    bufio_Reader* b = self;
    b->lastRuneSize = -1;
    for (; b->r == b->w;) {
        if (b->err.self != NULL) {
            return (so_R_byte_err){.val = 0, .err = bufio_Reader_readErr(b)};
        }
        // buffer is empty
        bufio_Reader_fill(b);
    }
    so_byte c = so_at(so_byte, b->buf, b->r);
    b->r++;
    b->lastByte = (so_int)(c);
    return (so_R_byte_err){.val = c, .err = (so_Error){0}};
}

// UnreadByte unreads the last byte. Only the most recently read byte can be unread.
//
// UnreadByte returns an error if the most recent method called on the
// [Reader] was not a read operation. Notably, [Reader.Peek], [Reader.Discard], and [Reader.WriteTo] are not
// considered read operations.
so_Error bufio_Reader_UnreadByte(void* self) {
    bufio_Reader* b = self;
    if (b->lastByte < 0 || (b->r == 0 && b->w > 0)) {
        return bufio_ErrInvalidUnreadByte;
    }
    // b.r > 0 || b.w == 0
    if (b->r > 0) {
        b->r--;
    } else {
        // b.r == 0 && b.w == 0
        b->w = 1;
    }
    so_at(so_byte, b->buf, b->r) = (so_byte)(b->lastByte);
    b->lastByte = -1;
    b->lastRuneSize = -1;
    return (so_Error){0};
}

// ReadRune reads a single UTF-8 encoded Unicode character and returns the
// rune and its size in bytes. If the encoded rune is invalid, it consumes one byte
// and returns unicode.ReplacementChar (U+FFFD) with a size of 1.
io_RuneSizeResult bufio_Reader_ReadRune(void* self) {
    bufio_Reader* b = self;
    for (; b->r + utf8_UTFMax > b->w && !utf8_FullRune(so_slice(so_byte, b->buf, b->r, b->w)) && b->err.self == NULL && b->w - b->r < so_len(b->buf);) {
        // b.w-b.r < len(buf) => buffer is not full
        bufio_Reader_fill(b);
    }
    b->lastRuneSize = -1;
    if (b->r == b->w) {
        return (io_RuneSizeResult){.Err = bufio_Reader_readErr(b)};
    }
    so_R_rune_int _res1 = utf8_DecodeRune(so_slice(so_byte, b->buf, b->r, b->w));
    so_rune r = _res1.val;
    so_int size = _res1.val2;
    b->r += size;
    b->lastByte = (so_int)(so_at(so_byte, b->buf, b->r - 1));
    b->lastRuneSize = size;
    return (io_RuneSizeResult){.Rune = r, .Size = size};
}

// UnreadRune unreads the last rune. If the most recent method called on
// the [Reader] was not a [Reader.ReadRune], [Reader.UnreadRune] returns an error. (In this
// regard it is stricter than [Reader.UnreadByte], which will unread the last byte
// from any read operation.)
so_Error bufio_Reader_UnreadRune(void* self) {
    bufio_Reader* b = self;
    if (b->lastRuneSize < 0 || b->r < b->lastRuneSize) {
        return bufio_ErrInvalidUnreadRune;
    }
    b->r -= b->lastRuneSize;
    b->lastByte = -1;
    b->lastRuneSize = -1;
    return (so_Error){0};
}

// Buffered returns the number of bytes that can be read from the current buffer.
so_int bufio_Reader_Buffered(void* self) {
    bufio_Reader* b = self;
    return b->w - b->r;
}

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
so_R_slice_err bufio_Reader_ReadSlice(void* self, so_byte delim) {
    bufio_Reader* b = self;
    so_Slice line = {0};
    so_Error err = {0};
    // search start index
    so_int s = 0;
    for (;;) {
        // Search buffer.
        {
            so_int i = bytes_IndexByte(so_slice(so_byte, b->buf, b->r + s, b->w), delim);
            if (i >= 0) {
                i += s;
                line = so_slice(so_byte, b->buf, b->r, b->r + i + 1);
                b->r += i + 1;
                break;
            }
        }
        // Pending error?
        if (b->err.self != NULL) {
            line = so_slice(so_byte, b->buf, b->r, b->w);
            b->r = b->w;
            err = bufio_Reader_readErr(b);
            break;
        }
        // Buffer full?
        if (bufio_Reader_Buffered(b) >= so_len(b->buf)) {
            b->r = b->w;
            line = b->buf;
            err = bufio_ErrBufferFull;
            break;
        }
        // do not rescan area we scanned before
        s = b->w - b->r;
        // buffer is not full
        bufio_Reader_fill(b);
    }
    // Handle last byte, if any.
    {
        so_int i = so_len(line) - 1;
        if (i >= 0) {
            b->lastByte = (so_int)(so_at(so_byte, line, i));
            b->lastRuneSize = -1;
        }
    }
    return (so_R_slice_err){.val = line, .err = err};
}

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
bufio_ReadLineResult bufio_Reader_ReadLine(void* self) {
    bufio_Reader* b = self;
    so_R_slice_err _res1 = bufio_Reader_ReadSlice(b, '\n');
    so_Slice line = _res1.val;
    so_Error err = _res1.err;
    if (err.self == bufio_ErrBufferFull.self) {
        // Handle the case where "\r\n" straddles the buffer.
        if (so_len(line) > 0 && so_at(so_byte, line, so_len(line) - 1) == '\r') {
            // Put the '\r' back on buf and drop it from line.
            // Let the next call to ReadLine check for "\r\n".
            if (b->r == 0) {
                // should be unreachable
                so_panic("bufio: tried to rewind past start of buffer");
            }
            b->r--;
            line = so_slice(so_byte, line, 0, so_len(line) - 1);
        }
        return (bufio_ReadLineResult){.Line = line, .IsPrefix = true};
    }
    if (so_len(line) == 0) {
        if (err.self != NULL) {
            return (bufio_ReadLineResult){.Err = err};
        }
        return (bufio_ReadLineResult){};
    }
    if (so_at(so_byte, line, so_len(line) - 1) == '\n') {
        so_int drop = 1;
        if (so_len(line) > 1 && so_at(so_byte, line, so_len(line) - 2) == '\r') {
            drop = 2;
        }
        line = so_slice(so_byte, line, 0, so_len(line) - drop);
    }
    return (bufio_ReadLineResult){.Line = line};
}

// ReadBytes reads until the first occurrence of delim in the input,
// returning a slice containing the data up to and including the delimiter.
// If ReadBytes encounters an error before finding a delimiter,
// it returns the data read before the error and the error itself (often io.EOF).
// ReadBytes returns err != nil if and only if the returned data does not end in
// delim.
// For simple uses, a Scanner may be more convenient.
//
// The returned slice is allocated; the caller owns it.
so_R_slice_err bufio_Reader_ReadBytes(void* self, so_byte delim) {
    bufio_Reader* b = self;
    so_R_slice_err _res1 = bufio_Reader_ReadSlice(b, delim);
    so_Slice frag = _res1.val;
    so_Error err = _res1.err;
    if (err.self != bufio_ErrBufferFull.self) {
        // Fast path: delimiter found or non-full-buffer error.
        // Clone since frag points into internal buffer.
        so_Slice line = mem_AllocSlice(so_byte, (b->a), (so_len(frag)), (so_len(frag)));
        so_copy(so_byte, line, frag);
        return (so_R_slice_err){.val = line, .err = err};
    }
    // Slow path: accumulate fragments in a buffer.
    bytes_Buffer buf = bytes_NewBuffer(b->a, (so_Slice){0});
    for (;;) {
        bytes_Buffer_Write(&buf, frag);
        so_R_slice_err _res2 = bufio_Reader_ReadSlice(b, delim);
        frag = _res2.val;
        err = _res2.err;
        if (err.self != bufio_ErrBufferFull.self) {
            break;
        }
    }
    bytes_Buffer_Write(&buf, frag);
    so_Slice result = bytes_Clone(b->a, bytes_Buffer_Bytes(&buf));
    bytes_Buffer_Free(&buf);
    return (so_R_slice_err){.val = result, .err = err};
}

// ReadString reads until the first occurrence of delim in the input,
// returning a string containing the data up to and including the delimiter.
// If ReadString encounters an error before finding a delimiter,
// it returns the data read before the error and the error itself (often io.EOF).
// ReadString returns err != nil if and only if the returned data does not end in
// delim.
// For simple uses, a Scanner may be more convenient.
//
// The returned string is allocated; the caller owns it.
so_R_str_err bufio_Reader_ReadString(void* self, so_byte delim) {
    bufio_Reader* b = self;
    so_R_slice_err _res1 = bufio_Reader_ReadBytes(b, delim);
    so_Slice data = _res1.val;
    so_Error err = _res1.err;
    return (so_R_str_err){.val = so_bytes_string(data), .err = err};
}

// WriteTo implements io.WriterTo.
// This may make multiple calls to the [Reader.Read] method of the underlying [Reader].
so_R_i64_err bufio_Reader_WriteTo(void* self, io_Writer w) {
    bufio_Reader* b = self;
    b->lastByte = -1;
    b->lastRuneSize = -1;
    int64_t n = (int64_t)(0);
    if (b->r < b->w) {
        so_R_i64_err _res1 = bufio_Reader_writeBuf(b, w);
        int64_t m = _res1.val;
        so_Error err = _res1.err;
        n += m;
        if (err.self != NULL) {
            return (so_R_i64_err){.val = n, .err = err};
        }
    }
    if (b->w - b->r < so_len(b->buf)) {
        // buffer not full
        bufio_Reader_fill(b);
    }
    for (; b->r < b->w;) {
        // b.r < b.w => buffer is not empty
        so_R_i64_err _res2 = bufio_Reader_writeBuf(b, w);
        int64_t m = _res2.val;
        so_Error err = _res2.err;
        n += m;
        if (err.self != NULL) {
            return (so_R_i64_err){.val = n, .err = err};
        }
        // buffer is empty
        bufio_Reader_fill(b);
    }
    if (b->err.self == io_EOF.self) {
        b->err = (so_Error){0};
    }
    return (so_R_i64_err){.val = n, .err = bufio_Reader_readErr(b)};
}

// writeBuf writes the [Reader]'s buffer to the writer.
static so_R_i64_err bufio_Reader_writeBuf(void* self, io_Writer w) {
    bufio_Reader* b = self;
    so_R_int_err _res1 = w.Write(w.self, so_slice(so_byte, b->buf, b->r, b->w));
    so_int n = _res1.val;
    so_Error err = _res1.err;
    if (n < 0) {
        so_panic(so_error_cstr(errNegativeWrite));
    }
    b->r += n;
    return (so_R_i64_err){.val = (int64_t)(n), .err = err};
}

// -- scan.go --

// NewScanner returns a new [Scanner] to read from r.
// The split function defaults to [ScanLines].
//
// The caller is responsible for freeing the scanner resources
// with [Scanner.Free] when done using it.
bufio_Scanner bufio_NewScanner(mem_Allocator a, io_Reader r) {
    return (bufio_Scanner){.a = a, .r = r, .split = bufio_ScanLines, .maxTokenSize = bufio_MaxScanTokenSize};
}

// Free releases the internal scanner buffer.
// It is safe to call Free on a scanner that used a user-provided buffer
// via [Scanner.Buffer]; in that case Free is a no-op.
void bufio_Scanner_Free(void* self) {
    bufio_Scanner* s = self;
    if (!s->ownsBuf) {
        return;
    }
    mem_FreeSlice(so_byte, (s->a), (s->buf));
    s->buf = (so_Slice){0};
    s->ownsBuf = false;
}

// Err returns the first non-EOF error that was encountered by the [Scanner].
so_Error bufio_Scanner_Err(void* self) {
    bufio_Scanner* s = self;
    if (s->err.self == io_EOF.self) {
        return (so_Error){0};
    }
    return s->err;
}

// Bytes returns the most recent token generated by a call to [Scanner.Scan].
// The underlying array may point to data that will be overwritten
// by a subsequent call to Scan. It does no allocation.
so_Slice bufio_Scanner_Bytes(void* self) {
    bufio_Scanner* s = self;
    return s->token;
}

// Text returns the most recent token generated by a call to [Scanner.Scan]
// as a string. The returned string is a zero-copy view into the buffer and
// is invalidated by the next call to [Scanner.Scan].
so_String bufio_Scanner_Text(void* self) {
    bufio_Scanner* s = self;
    return so_bytes_string(s->token);
}

// Scan advances the [Scanner] to the next token, which will then be
// available through the [Scanner.Bytes] or [Scanner.Text] method. It returns false when
// there are no more tokens, either by reaching the end of the input or an error.
// After Scan returns false, the [Scanner.Err] method will return any error that
// occurred during scanning, except that if it was [io.EOF], [Scanner.Err]
// will return nil.
// Scan panics if the split function returns too many empty
// tokens without advancing the input. This is a common error mode for
// scanners.
bool bufio_Scanner_Scan(void* self) {
    bufio_Scanner* s = self;
    if (s->done) {
        return false;
    }
    s->scanCalled = true;
    // Loop until we have a token.
    for (;;) {
        // See if we can get a token with what we already have.
        // If we've run out of data but have an error, give the split function
        // a chance to recover any remaining, possibly empty token.
        if (s->end > s->start || s->err.self != NULL) {
            bufio_SplitResult res = s->split(so_slice(so_byte, s->buf, s->start, s->end), s->err.self != NULL);
            if (res.Err.self != NULL) {
                if (res.Err.self == bufio_ErrFinalToken.self) {
                    s->token = res.Token;
                    s->done = true;
                    return res.HasToken;
                }
                bufio_Scanner_setErr(s, res.Err);
                return false;
            }
            if (!bufio_Scanner_advance(s, res.Advance)) {
                return false;
            }
            s->token = res.Token;
            if (res.HasToken) {
                if (s->err.self == NULL || res.Advance > 0) {
                    s->empties = 0;
                } else {
                    // Returning tokens not advancing input at EOF.
                    s->empties++;
                    if (s->empties > maxConsecutiveEmptyReads) {
                        so_panic("bufio.Scanner: too many empty tokens without progressing");
                    }
                }
                return true;
            }
        }
        // We cannot generate a token with what we are holding.
        // If we've already hit EOF or an I/O error, we are done.
        if (s->err.self != NULL) {
            // Shut it down.
            s->start = 0;
            s->end = 0;
            return false;
        }
        // Must read more data.
        // First, shift data to beginning of buffer if there's lots of empty space
        // or space is needed.
        if (s->start > 0 && (s->end == so_len(s->buf) || s->start > so_len(s->buf) / 2)) {
            so_copy(so_byte, s->buf, so_slice(so_byte, s->buf, s->start, s->end));
            s->end -= s->start;
            s->start = 0;
        }
        // Is the buffer full? If so, resize.
        if (s->end == so_len(s->buf)) {
            // Guarantee no overflow in the multiplication below.
            if (so_len(s->buf) >= s->maxTokenSize || so_len(s->buf) > maxInt / 2) {
                bufio_Scanner_setErr(s, bufio_ErrTooLong);
                return false;
            }
            so_int newSize = so_len(s->buf) * 2;
            if (newSize == 0) {
                newSize = startBufSize;
            }
            newSize = so_min(newSize, s->maxTokenSize);
            so_Slice newBuf = mem_AllocSlice(so_byte, (s->a), (newSize), (newSize));
            if (s->end > s->start) {
                // Only copy if s.buf is not nil to avoid UB in C.
                so_copy(so_byte, newBuf, so_slice(so_byte, s->buf, s->start, s->end));
            }
            if (s->ownsBuf) {
                mem_FreeSlice(so_byte, (s->a), (s->buf));
            }
            s->buf = newBuf;
            s->ownsBuf = true;
            s->end -= s->start;
            s->start = 0;
        }
        // Finally we can read some input. Make sure we don't get stuck with
        // a misbehaving Reader. Officially we don't need to do this, but let's
        // be extra careful: Scanner is for safe, simple jobs.
        for (so_int loop = 0;;) {
            so_R_int_err _res1 = s->r.Read(s->r.self, so_slice(so_byte, s->buf, s->end, so_len(s->buf)));
            so_int n = _res1.val;
            so_Error err = _res1.err;
            if (n < 0 || so_len(s->buf) - s->end < n) {
                bufio_Scanner_setErr(s, bufio_ErrBadReadCount);
                break;
            }
            s->end += n;
            if (err.self != NULL) {
                bufio_Scanner_setErr(s, err);
                break;
            }
            if (n > 0) {
                s->empties = 0;
                break;
            }
            loop++;
            if (loop > maxConsecutiveEmptyReads) {
                bufio_Scanner_setErr(s, io_ErrNoProgress);
                break;
            }
        }
    }
}

// advance consumes n bytes of the buffer. It reports whether the advance was legal.
static bool bufio_Scanner_advance(void* self, so_int n) {
    bufio_Scanner* s = self;
    if (n < 0) {
        bufio_Scanner_setErr(s, bufio_ErrNegativeAdvance);
        return false;
    }
    if (n > s->end - s->start) {
        bufio_Scanner_setErr(s, bufio_ErrAdvanceTooFar);
        return false;
    }
    s->start += n;
    return true;
}

// setErr records the first error encountered.
static void bufio_Scanner_setErr(void* self, so_Error err) {
    bufio_Scanner* s = self;
    if (s->err.self == NULL || s->err.self == io_EOF.self) {
        s->err = err;
    }
}

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
void bufio_Scanner_Buffer(void* self, so_Slice buf, so_int max) {
    bufio_Scanner* s = self;
    if (s->scanCalled) {
        so_panic("bufio.Scanner: Buffer called after Scan");
    }
    if (s->ownsBuf) {
        mem_FreeSlice(so_byte, (s->a), (s->buf));
        s->ownsBuf = false;
    }
    s->buf = so_slice(so_byte, buf, 0, so_cap(buf));
    s->maxTokenSize = max;
}

// Split sets the split function for the [Scanner].
// The default split function is [ScanLines].
//
// Split panics if it is called after scanning has started.
void bufio_Scanner_Split(void* self, bufio_SplitFunc split) {
    bufio_Scanner* s = self;
    if (s->scanCalled) {
        so_panic("bufio.Scanner: Split called after Scan");
    }
    s->split = split;
}

// Split functions
// ScanBytes is a split function for a [Scanner] that returns each byte as a token.
bufio_SplitResult bufio_ScanBytes(so_Slice data, bool atEOF) {
    if (atEOF && so_len(data) == 0) {
        return (bufio_SplitResult){};
    }
    return (bufio_SplitResult){.Advance = 1, .Token = so_slice(so_byte, data, 0, 1), .HasToken = true};
}

// ScanRunes is a split function for a [Scanner] that returns each
// UTF-8-encoded rune as a token. The sequence of runes returned is
// equivalent to that from a range loop over the input as a string, which
// means that erroneous UTF-8 encodings translate to U+FFFD = "\xef\xbf\xbd".
// Because of the Scan interface, this makes it impossible for the client to
// distinguish correctly encoded replacement runes from encoding errors.
bufio_SplitResult bufio_ScanRunes(so_Slice data, bool atEOF) {
    if (atEOF && so_len(data) == 0) {
        return (bufio_SplitResult){};
    }
    // Fast path 1: ASCII.
    if (so_at(so_byte, data, 0) < utf8_RuneSelf) {
        return (bufio_SplitResult){.Advance = 1, .Token = so_slice(so_byte, data, 0, 1), .HasToken = true};
    }
    // Fast path 2: Correct UTF-8 decode without error.
    so_R_rune_int _res1 = utf8_DecodeRune(data);
    so_int width = _res1.val2;
    if (width > 1) {
        // It's a valid encoding. Width cannot be one for a correctly encoded
        // non-ASCII rune.
        return (bufio_SplitResult){.Advance = width, .Token = so_slice(so_byte, data, 0, width), .HasToken = true};
    }
    // We know it's an error: we have width==1 and implicitly r==utf8.RuneError.
    // Is the error because there wasn't a full rune to be decoded?
    // FullRune distinguishes correctly between erroneous and incomplete encodings.
    if (!atEOF && !utf8_FullRune(data)) {
        // Incomplete; get more bytes.
        return (bufio_SplitResult){};
    }
    // We have a real UTF-8 encoding error. Return a properly encoded error rune
    // but advance only one byte. This matches the behavior of a range loop over
    // an incorrectly encoded string.
    return (bufio_SplitResult){.Advance = 1, .Token = so_array_slice(so_byte, errorRune, 0, 3, 3), .HasToken = true};
}

// dropCR drops a terminal \r from the data.
static so_Slice dropCR(so_Slice data) {
    if (so_len(data) > 0 && so_at(so_byte, data, so_len(data) - 1) == '\r') {
        return so_slice(so_byte, data, 0, so_len(data) - 1);
    }
    return data;
}

// ScanLines is a split function for a [Scanner] that returns each line of
// text, stripped of any trailing end-of-line marker. The returned line may
// be empty. The end-of-line marker is one optional carriage return followed
// by one mandatory newline. In regular expression notation, it is `\r?\n`.
// The last non-empty line of input will be returned even if it has no
// newline.
bufio_SplitResult bufio_ScanLines(so_Slice data, bool atEOF) {
    if (atEOF && so_len(data) == 0) {
        return (bufio_SplitResult){};
    }
    {
        so_int i = bytes_IndexByte(data, '\n');
        if (i >= 0) {
            // We have a full newline-terminated line.
            return (bufio_SplitResult){.Advance = i + 1, .Token = dropCR(so_slice(so_byte, data, 0, i)), .HasToken = true};
        }
    }
    // If we're at EOF, we have a final, non-terminated line. Return it.
    if (atEOF) {
        return (bufio_SplitResult){.Advance = so_len(data), .Token = dropCR(data), .HasToken = true};
    }
    // Request more data.
    return (bufio_SplitResult){};
}

// isSpace reports whether the character is a Unicode white space character.
// We avoid dependency on the unicode package, but check validity of the implementation
// in the tests.
static bool isSpace(so_rune r) {
    if (r <= 0x00FF) {
        // Obvious ASCII ones: \t through \r plus space. Plus two Latin-1 oddballs.
        if (r == (U' ') || r == (U'\t') || r == (U'\n') || r == (U'\v') || r == (U'\f') || r == (U'\r')) {
            return true;
        } else if (r == (0x0085) || r == (0x00A0)) {
            return true;
        }
        return false;
    }
    // High-valued ones.
    if (0x2000 <= r && r <= 0x200a) {
        return true;
    }
    if (r == (0x1680) || r == (0x2028) || r == (0x2029) || r == (0x202f) || r == (0x205f) || r == (0x3000)) {
        return true;
    }
    return false;
}

// ScanWords is a split function for a [Scanner] that returns each
// space-separated word of text, with surrounding spaces deleted. It will
// never return an empty string. The definition of space is set by
// unicode.IsSpace.
bufio_SplitResult bufio_ScanWords(so_Slice data, bool atEOF) {
    // Skip leading spaces.
    so_int start = 0;
    for (so_int width = 0; start < so_len(data); start += width) {
        so_rune r = 0;
        so_R_rune_int _res1 = utf8_DecodeRune(so_slice(so_byte, data, start, data.len));
        r = _res1.val;
        width = _res1.val2;
        if (!isSpace(r)) {
            break;
        }
    }
    // Scan until space, marking end of word.
    so_int width = 0;
    for (so_int i = start; i < so_len(data); i += width) {
        so_rune r = 0;
        so_R_rune_int _res2 = utf8_DecodeRune(so_slice(so_byte, data, i, data.len));
        r = _res2.val;
        width = _res2.val2;
        if (isSpace(r)) {
            return (bufio_SplitResult){.Advance = i + width, .Token = so_slice(so_byte, data, start, i), .HasToken = true};
        }
    }
    // If we're at EOF, we have a final, non-empty, non-terminated word. Return it.
    if (atEOF && so_len(data) > start) {
        return (bufio_SplitResult){.Advance = so_len(data), .Token = so_slice(so_byte, data, start, data.len), .HasToken = true};
    }
    // Request more data.
    return (bufio_SplitResult){.Advance = start};
}

// -- writer.go --

// NewWriterSize returns a new [Writer] whose buffer has at least the specified
// size. If the argument io.Writer is already a [Writer] with large enough
// size, it returns the underlying [Writer].
bufio_Writer bufio_NewWriterSize(mem_Allocator a, io_Writer w, so_int size) {
    // Is it already a Writer?
    bool ok = (w.Write == bufio_Writer_Write);
    if (ok) {
        bufio_Writer* b = (bufio_Writer*)w.self;
        if (so_len(b->buf) >= size) {
            return *b;
        }
    }
    if (size <= 0) {
        size = bufio_DefaultBufSize;
    }
    bufio_Writer wr = (bufio_Writer){.a = a};
    wr.buf = mem_AllocSlice(so_byte, (a), (size), (size));
    wr.wr = w;
    return wr;
}

// NewWriter returns a new [Writer] whose buffer has the default size.
// If the argument io.Writer is already a [Writer] with large enough buffer size,
// it returns the underlying [Writer].
bufio_Writer bufio_NewWriter(mem_Allocator a, io_Writer w) {
    return bufio_NewWriterSize(a, w, bufio_DefaultBufSize);
}

// Size returns the size of the underlying buffer in bytes.
so_int bufio_Writer_Size(void* self) {
    bufio_Writer* b = self;
    return so_len(b->buf);
}

// Reset discards any unflushed buffered data, clears any error, and
// resets b to write its output to w.
// Calling Reset on the zero value of [Writer] initializes the internal buffer
// to the default size.
// Calling w.Reset(w) (that is, resetting a [Writer] to itself) does nothing.
void bufio_Writer_Reset(void* self, io_Writer w) {
    bufio_Writer* b = self;
    // Avoid no-op reset to self.
    bool ok = (w.Write == bufio_Writer_Write);
    if (ok && b == (bufio_Writer*)w.self) {
        return;
    }
    if (b->buf.ptr == NULL) {
        b->buf = mem_AllocSlice(so_byte, (b->a), (bufio_DefaultBufSize), (bufio_DefaultBufSize));
    }
    b->err = (so_Error){0};
    b->n = 0;
    b->wr = w;
}

// Free releases the internal buffer memory.
// The writer must not be used after calling Free.
void bufio_Writer_Free(void* self) {
    bufio_Writer* b = self;
    mem_FreeSlice(so_byte, (b->a), (b->buf));
    b->buf = (so_Slice){0};
}

// Flush writes any buffered data to the underlying [io.Writer].
so_Error bufio_Writer_Flush(void* self) {
    bufio_Writer* b = self;
    if (b->err.self != NULL) {
        return b->err;
    }
    if (b->n == 0) {
        return (so_Error){0};
    }
    so_R_int_err _res1 = b->wr.Write(b->wr.self, so_slice(so_byte, b->buf, 0, b->n));
    so_int n = _res1.val;
    so_Error err = _res1.err;
    if (n < b->n && err.self == NULL) {
        err = io_ErrShortWrite;
    }
    if (err.self != NULL) {
        if (n > 0 && n < b->n) {
            so_copy(so_byte, so_slice(so_byte, b->buf, 0, b->n - n), so_slice(so_byte, b->buf, n, b->n));
        }
        b->n -= n;
        b->err = err;
        return err;
    }
    b->n = 0;
    return (so_Error){0};
}

// Available returns how many bytes are unused in the buffer.
so_int bufio_Writer_Available(void* self) {
    bufio_Writer* b = self;
    return so_len(b->buf) - b->n;
}

// AvailableBuffer returns an empty buffer with b.Available() capacity.
// This buffer is intended to be appended to and
// passed to an immediately succeeding [Writer.Write] call.
// The buffer is only valid until the next write operation on b.
so_Slice bufio_Writer_AvailableBuffer(void* self) {
    bufio_Writer* b = self;
    return so_slice(so_byte, so_slice(so_byte, b->buf, b->n, b->buf.len), 0, 0);
}

// Buffered returns the number of bytes that have been written into the current buffer.
so_int bufio_Writer_Buffered(void* self) {
    bufio_Writer* b = self;
    return b->n;
}

// Write writes the contents of p into the buffer.
// It returns the number of bytes written.
// If nn < len(p), it also returns an error explaining
// why the write is short.
so_R_int_err bufio_Writer_Write(void* self, so_Slice p) {
    bufio_Writer* b = self;
    so_int nn = 0;
    for (; so_len(p) > bufio_Writer_Available(b) && b->err.self == NULL;) {
        so_int n = 0;
        if (bufio_Writer_Buffered(b) == 0) {
            // Large write, empty buffer.
            // Write directly from p to avoid copy.
            so_R_int_err _res1 = b->wr.Write(b->wr.self, p);
            n = _res1.val;
            b->err = _res1.err;
        } else {
            n = so_copy(so_byte, so_slice(so_byte, b->buf, b->n, b->buf.len), p);
            b->n += n;
            bufio_Writer_Flush(b);
        }
        nn += n;
        p = so_slice(so_byte, p, n, p.len);
    }
    if (b->err.self != NULL) {
        return (so_R_int_err){.val = nn, .err = b->err};
    }
    so_int n = so_copy(so_byte, so_slice(so_byte, b->buf, b->n, b->buf.len), p);
    b->n += n;
    nn += n;
    return (so_R_int_err){.val = nn, .err = (so_Error){0}};
}

// WriteByte writes a single byte.
so_Error bufio_Writer_WriteByte(void* self, so_byte c) {
    bufio_Writer* b = self;
    if (b->err.self != NULL) {
        return b->err;
    }
    if (bufio_Writer_Available(b) <= 0 && bufio_Writer_Flush(b).self != NULL) {
        return b->err;
    }
    so_at(so_byte, b->buf, b->n) = c;
    b->n++;
    return (so_Error){0};
}

// WriteRune writes a single Unicode code point, returning
// the number of bytes written and any error.
so_R_int_err bufio_Writer_WriteRune(void* self, so_rune r) {
    bufio_Writer* b = self;
    // Compare as uint32 to correctly handle negative runes.
    if ((uint32_t)(r) < utf8_RuneSelf) {
        so_Error err = bufio_Writer_WriteByte(b, (so_byte)(r));
        if (err.self != NULL) {
            return (so_R_int_err){.val = 0, .err = err};
        }
        return (so_R_int_err){.val = 1, .err = (so_Error){0}};
    }
    if (b->err.self != NULL) {
        return (so_R_int_err){.val = 0, .err = b->err};
    }
    so_int n = bufio_Writer_Available(b);
    if (n < utf8_UTFMax) {
        {
            bufio_Writer_Flush(b);
            if (b->err.self != NULL) {
                return (so_R_int_err){.val = 0, .err = b->err};
            }
        }
        n = bufio_Writer_Available(b);
        if (n < utf8_UTFMax) {
            // Can only happen if buffer is silly small.
            return bufio_Writer_WriteString(b, so_rune_string(r));
        }
    }
    so_int size = utf8_EncodeRune(so_slice(so_byte, b->buf, b->n, b->buf.len), r);
    b->n += size;
    return (so_R_int_err){.val = size, .err = (so_Error){0}};
}

// WriteString writes a string.
// It returns the number of bytes written.
// If the count is less than len(s), it also returns an error explaining
// why the write is short.
so_R_int_err bufio_Writer_WriteString(void* self, so_String s) {
    bufio_Writer* b = self;
    so_int nn = 0;
    for (; so_len(s) > bufio_Writer_Available(b) && b->err.self == NULL;) {
        so_int n = so_copy_string(so_slice(so_byte, b->buf, b->n, b->buf.len), s);
        b->n += n;
        bufio_Writer_Flush(b);
        nn += n;
        s = so_string_slice(s, n, s.len);
    }
    if (b->err.self != NULL) {
        return (so_R_int_err){.val = nn, .err = b->err};
    }
    so_int n = so_copy_string(so_slice(so_byte, b->buf, b->n, b->buf.len), s);
    b->n += n;
    nn += n;
    return (so_R_int_err){.val = nn, .err = (so_Error){0}};
}

// ReadFrom implements [io.ReaderFrom].
so_R_i64_err bufio_Writer_ReadFrom(void* self, io_Reader r) {
    bufio_Writer* b = self;
    if (b->err.self != NULL) {
        return (so_R_i64_err){.val = 0, .err = b->err};
    }
    int64_t n = (int64_t)(0);
    so_int m = 0;
    so_Error err = {0};
    for (;;) {
        if (bufio_Writer_Available(b) == 0) {
            {
                so_Error err1 = bufio_Writer_Flush(b);
                if (err1.self != NULL) {
                    return (so_R_i64_err){.val = n, .err = err1};
                }
            }
        }
        so_int nr = 0;
        for (; nr < maxConsecutiveEmptyReads;) {
            so_R_int_err _res1 = r.Read(r.self, so_slice(so_byte, b->buf, b->n, b->buf.len));
            m = _res1.val;
            err = _res1.err;
            if (m != 0 || err.self != NULL) {
                break;
            }
            nr++;
        }
        if (nr == maxConsecutiveEmptyReads) {
            return (so_R_i64_err){.val = n, .err = io_ErrNoProgress};
        }
        b->n += m;
        n += (int64_t)(m);
        if (err.self != NULL) {
            break;
        }
    }
    if (err.self == io_EOF.self) {
        // If we filled the buffer exactly, flush preemptively.
        if (bufio_Writer_Available(b) == 0) {
            err = bufio_Writer_Flush(b);
        } else {
            err = (so_Error){0};
        }
    }
    return (so_R_i64_err){.val = n, .err = err};
}

// NewReadWriter allocates a new [ReadWriter] that dispatches to r and w.
bufio_ReadWriter bufio_NewReadWriter(bufio_Reader* r, bufio_Writer* w) {
    return (bufio_ReadWriter){.r = r, .w = w};
}

so_R_int_err bufio_ReadWriter_Read(void* self, so_Slice p) {
    bufio_ReadWriter* rw = self;
    return bufio_Reader_Read(rw->r, p);
}

so_R_int_err bufio_ReadWriter_Write(void* self, so_Slice p) {
    bufio_ReadWriter* rw = self;
    return bufio_Writer_Write(rw->w, p);
}
