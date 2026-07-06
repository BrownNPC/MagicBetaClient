#include "net.h"

// -- Forward declarations --
static void net_BufferedReader_reset(void* self, so_Slice buf, io_Reader r);
static void net_BufferedReader_fill(void* self);
static so_Error net_BufferedReader_readErr(void* self);
static so_R_i64_err net_BufferedReader_writeBuf(void* self, io_Writer w);

// -- Variables and constants --
static const int64_t minReadBufferSize = 1;
static const int64_t maxConsecutiveEmptyReads = 1;
so_Error net_ErrInvalidUnreadByte = errors_New("bufio: invalid use of UnreadByte");
so_Error net_ErrInvalidUnreadRune = errors_New("bufio: invalid use of UnreadRune");
so_Error net_ErrBufferFull = errors_New("bufio: buffer full");
so_Error net_ErrNegativeCount = errors_New("bufio: negative count");
static so_Error errNegativeRead = errors_New("bufio: reader returned negative count from Read");
static so_Error errNegativeWrite = errors_New("bufio: writer returned negative count from Write");
so_Error net_ErrConnectionClosed = errors_New("Connection closed.");

// -- bufferedReader.go --

// NewBufferedReaderSize returns a new [BufferedReader] whose buffer has at least the specified
// size. If the argument io.Reader is already a [BufferedReader] with large enough
// size, it returns the underlying [BufferedReader].
net_BufferedReader net_NewBufferedReaderSize(mem_Allocator a, io_Reader rd, so_int size) {
    // Is it already a Reader?
    {
        bool ok = (rd.Read == net_BufferedReader_Read);
        if (ok) {
            net_BufferedReader* b = (net_BufferedReader*)rd.self;
            if (so_len(b->buf) >= size) {
                return *b;
            }
        }
    }
    so_int sz = so_max(size, minReadBufferSize);
    so_Slice buf = mem_AllocSlice(so_byte, (a), (sz), (sz));
    net_BufferedReader r = (net_BufferedReader){.a = a};
    net_BufferedReader_reset(&r, buf, rd);
    return r;
}

// NewBufferedReader returns a new [BufferedReader] whose buffer has the default size.
net_BufferedReader net_NewBufferedReader(mem_Allocator a, io_Reader rd) {
    return net_NewBufferedReaderSize(a, rd, net_DefaultBufSize);
}

// Size returns the size of the underlying buffer in bytes.
so_int net_BufferedReader_Size(void* self) {
    net_BufferedReader* b = self;
    return so_len(b->buf);
}

// Reset discards any buffered data, resets all state, and switches
// the buffered reader to read from r.
// Calling Reset on the zero value of [BufferedReader] initializes the internal buffer
// to the default size.
// Calling b.Reset(b) (that is, resetting a [BufferedReader] to itself) does nothing.
void net_BufferedReader_Reset(void* self, io_Reader r) {
    net_BufferedReader* b = self;
    // Avoid no-op reset to self.
    bool ok = (r.Read == net_BufferedReader_Read);
    if (ok && b == (net_BufferedReader*)r.self) {
        return;
    }
    if (b->buf.ptr == NULL) {
        b->buf = mem_AllocSlice(so_byte, (b->a), (net_DefaultBufSize), (net_DefaultBufSize));
    }
    net_BufferedReader_reset(b, b->buf, r);
}

// Free releases the internal reader's buffer.
// The reader must not be used after calling Free.
void net_BufferedReader_Free(void* self) {
    net_BufferedReader* b = self;
    mem_FreeSlice(so_byte, (b->a), (b->buf));
    b->buf = (so_Slice){0};
}

static void net_BufferedReader_reset(void* self, so_Slice buf, io_Reader r) {
    net_BufferedReader* b = self;
    b->buf = buf;
    b->rd = r;
    b->lastByte = -1;
    b->lastRuneSize = -1;
}

// fill reads a new chunk into the buffer.
static void net_BufferedReader_fill(void* self) {
    net_BufferedReader* b = self;
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

static so_Error net_BufferedReader_readErr(void* self) {
    net_BufferedReader* b = self;
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
// Calling Peek prevents a [BufferedReader.UnreadByte] or [BufferedReader.UnreadRune] call from succeeding
// until the next read operation.
so_R_slice_err net_BufferedReader_Peek(void* self, so_int n) {
    net_BufferedReader* b = self;
    if (n < 0) {
        return (so_R_slice_err){.val = (so_Slice){0}, .err = net_ErrNegativeCount};
    }
    b->lastByte = -1;
    b->lastRuneSize = -1;
    for (; b->w - b->r < n && b->w - b->r < so_len(b->buf) && b->err.self == NULL;) {
        // b.w-b.r < len(b.buf) => buffer is not full
        net_BufferedReader_fill(b);
    }
    if (n > so_len(b->buf)) {
        return (so_R_slice_err){.val = so_slice(so_byte, b->buf, b->r, b->w), .err = net_ErrBufferFull};
    }
    // 0 <= n <= len(b.buf)
    so_Error err = {0};
    {
        so_int avail = b->w - b->r;
        if (avail < n) {
            // not enough data in buffer
            n = avail;
            err = net_BufferedReader_readErr(b);
            if (err.self == NULL) {
                err = net_ErrBufferFull;
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
so_R_int_err net_BufferedReader_Discard(void* self, so_int n) {
    net_BufferedReader* b = self;
    if (n < 0) {
        return (so_R_int_err){.val = 0, .err = net_ErrNegativeCount};
    }
    if (n == 0) {
        return (so_R_int_err){.val = 0, .err = (so_Error){0}};
    }
    b->lastByte = -1;
    b->lastRuneSize = -1;
    so_int remain = n;
    for (;;) {
        so_int skip = net_BufferedReader_Buffered(b);
        if (skip == 0) {
            net_BufferedReader_fill(b);
            skip = net_BufferedReader_Buffered(b);
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
            return (so_R_int_err){.val = n - remain, .err = net_BufferedReader_readErr(b)};
        }
    }
}

// Read reads data into p.
// It returns the number of bytes read into p.
// The bytes are taken from at most one Read on the underlying [BufferedReader],
// hence n may be less than len(p).
// To read exactly len(p) bytes, use io.ReadFull(b, p).
// If the underlying [BufferedReader] can return a non-zero count with io.EOF,
// then this Read method can do so as well; see the [io.Reader] docs.
so_R_int_err net_BufferedReader_Read(void* self, so_Slice p) {
    net_BufferedReader* b = self;
    so_int n = so_len(p);
    if (n == 0) {
        if (net_BufferedReader_Buffered(b) > 0) {
            return (so_R_int_err){.val = 0, .err = (so_Error){0}};
        }
        return (so_R_int_err){.val = 0, .err = net_BufferedReader_readErr(b)};
    }
    if (b->r == b->w) {
        if (b->err.self != NULL) {
            return (so_R_int_err){.val = 0, .err = net_BufferedReader_readErr(b)};
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
            return (so_R_int_err){.val = n, .err = net_BufferedReader_readErr(b)};
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
            return (so_R_int_err){.val = 0, .err = net_BufferedReader_readErr(b)};
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
so_R_byte_err net_BufferedReader_ReadByte(void* self) {
    net_BufferedReader* b = self;
    b->lastRuneSize = -1;
    for (; b->r == b->w;) {
        if (b->err.self != NULL) {
            return (so_R_byte_err){.val = 0, .err = net_BufferedReader_readErr(b)};
        }
        // buffer is empty
        net_BufferedReader_fill(b);
    }
    so_byte c = so_at(so_byte, b->buf, b->r);
    b->r++;
    b->lastByte = (so_int)(c);
    return (so_R_byte_err){.val = c, .err = (so_Error){0}};
}

// UnreadByte unreads the last byte. Only the most recently read byte can be unread.
//
// UnreadByte returns an error if the most recent method called on the
// [BufferedReader] was not a read operation. Notably, [BufferedReader.Peek], [BufferedReader.Discard], and [BufferedReader.WriteTo] are not
// considered read operations.
so_Error net_BufferedReader_UnreadByte(void* self) {
    net_BufferedReader* b = self;
    if (b->lastByte < 0 || (b->r == 0 && b->w > 0)) {
        return net_ErrInvalidUnreadByte;
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
io_RuneSizeResult net_BufferedReader_ReadRune(void* self) {
    net_BufferedReader* b = self;
    for (; b->r + utf8_UTFMax > b->w && !utf8_FullRune(so_slice(so_byte, b->buf, b->r, b->w)) && b->err.self == NULL && b->w - b->r < so_len(b->buf);) {
        // b.w-b.r < len(buf) => buffer is not full
        net_BufferedReader_fill(b);
    }
    b->lastRuneSize = -1;
    if (b->r == b->w) {
        return (io_RuneSizeResult){.Err = net_BufferedReader_readErr(b)};
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
// the [BufferedReader] was not a [BufferedReader.ReadRune], [BufferedReader.UnreadRune] returns an error. (In this
// regard it is stricter than [BufferedReader.UnreadByte], which will unread the last byte
// from any read operation.)
so_Error net_BufferedReader_UnreadRune(void* self) {
    net_BufferedReader* b = self;
    if (b->lastRuneSize < 0 || b->r < b->lastRuneSize) {
        return net_ErrInvalidUnreadRune;
    }
    b->r -= b->lastRuneSize;
    b->lastByte = -1;
    b->lastRuneSize = -1;
    return (so_Error){0};
}

// Buffered returns the number of bytes that can be read from the current buffer.
so_int net_BufferedReader_Buffered(void* self) {
    net_BufferedReader* b = self;
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
// [BufferedReader.ReadBytes] or ReadString instead.
// ReadSlice returns err != nil if and only if line does not end in delim.
so_R_slice_err net_BufferedReader_ReadSlice(void* self, so_byte delim) {
    net_BufferedReader* b = self;
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
            err = net_BufferedReader_readErr(b);
            break;
        }
        // Buffer full?
        if (net_BufferedReader_Buffered(b) >= so_len(b->buf)) {
            b->r = b->w;
            line = b->buf;
            err = net_ErrBufferFull;
            break;
        }
        // do not rescan area we scanned before
        s = b->w - b->r;
        // buffer is not full
        net_BufferedReader_fill(b);
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
net_ReadLineResult net_BufferedReader_ReadLine(void* self) {
    net_BufferedReader* b = self;
    so_R_slice_err _res1 = net_BufferedReader_ReadSlice(b, '\n');
    so_Slice line = _res1.val;
    so_Error err = _res1.err;
    if (err.self == net_ErrBufferFull.self) {
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
        return (net_ReadLineResult){.Line = line, .IsPrefix = true};
    }
    if (so_len(line) == 0) {
        if (err.self != NULL) {
            return (net_ReadLineResult){.Err = err};
        }
        return (net_ReadLineResult){};
    }
    if (so_at(so_byte, line, so_len(line) - 1) == '\n') {
        so_int drop = 1;
        if (so_len(line) > 1 && so_at(so_byte, line, so_len(line) - 2) == '\r') {
            drop = 2;
        }
        line = so_slice(so_byte, line, 0, so_len(line) - drop);
    }
    return (net_ReadLineResult){.Line = line};
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
so_R_slice_err net_BufferedReader_ReadBytes(void* self, so_byte delim) {
    net_BufferedReader* b = self;
    so_R_slice_err _res1 = net_BufferedReader_ReadSlice(b, delim);
    so_Slice frag = _res1.val;
    so_Error err = _res1.err;
    if (err.self != net_ErrBufferFull.self) {
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
        so_R_slice_err _res2 = net_BufferedReader_ReadSlice(b, delim);
        frag = _res2.val;
        err = _res2.err;
        if (err.self != net_ErrBufferFull.self) {
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
so_R_str_err net_BufferedReader_ReadString(void* self, so_byte delim) {
    net_BufferedReader* b = self;
    so_R_slice_err _res1 = net_BufferedReader_ReadBytes(b, delim);
    so_Slice data = _res1.val;
    so_Error err = _res1.err;
    return (so_R_str_err){.val = so_bytes_string(data), .err = err};
}

// WriteTo implements io.WriterTo.
// This may make multiple calls to the [BufferedReader.Read] method of the underlying [BufferedReader].
so_R_i64_err net_BufferedReader_WriteTo(void* self, io_Writer w) {
    net_BufferedReader* b = self;
    b->lastByte = -1;
    b->lastRuneSize = -1;
    int64_t n = (int64_t)(0);
    if (b->r < b->w) {
        so_R_i64_err _res1 = net_BufferedReader_writeBuf(b, w);
        int64_t m = _res1.val;
        so_Error err = _res1.err;
        n += m;
        if (err.self != NULL) {
            return (so_R_i64_err){.val = n, .err = err};
        }
    }
    if (b->w - b->r < so_len(b->buf)) {
        // buffer not full
        net_BufferedReader_fill(b);
    }
    for (; b->r < b->w;) {
        // b.r < b.w => buffer is not empty
        so_R_i64_err _res2 = net_BufferedReader_writeBuf(b, w);
        int64_t m = _res2.val;
        so_Error err = _res2.err;
        n += m;
        if (err.self != NULL) {
            return (so_R_i64_err){.val = n, .err = err};
        }
        // buffer is empty
        net_BufferedReader_fill(b);
    }
    if (b->err.self == io_EOF.self) {
        b->err = (so_Error){0};
    }
    return (so_R_i64_err){.val = n, .err = net_BufferedReader_readErr(b)};
}

// writeBuf writes the [BufferedReader]'s buffer to the writer.
static so_R_i64_err net_BufferedReader_writeBuf(void* self, io_Writer w) {
    net_BufferedReader* b = self;
    so_R_int_err _res1 = w.Write(w.self, so_slice(so_byte, b->buf, b->r, b->w));
    so_int n = _res1.val;
    so_Error err = _res1.err;
    if (n < 0) {
        so_panic(so_error_cstr(errNegativeWrite));
    }
    b->r += n;
    return (so_R_i64_err){.val = (int64_t)(n), .err = err};
}

// -- conn.go --

// Read can return (0,nil) if there is nothing to be read.
// Read never blocks.
so_R_int_err net_Conn_Read(void* self, so_Slice b) {
    net_Conn* conn = self;
    if (conn->closed) {
        // already errored.
        return (so_R_int_err){.val = 0, .err = conn->err};
    }
    if (so_len(b) == 0) {
        return (so_R_int_err){.val = 0, .err = (so_Error){0}};
    }
    so_R_int_err _res1 = curl_ReadFromSocket(conn->sock, unsafe_SliceData(b), so_len(b));
    so_int n = _res1.val;
    so_Error err = _res1.err;
    if (err.self != NULL) {
        net_Conn_Close(conn);
        conn->err = err;
        return (so_R_int_err){.val = n, .err = err};
    }
    if (n != 0) {
        return (so_R_int_err){.val = n, .err = (so_Error){0}};
    }
    return (so_R_int_err){.val = 0, .err = (so_Error){0}};
}

// Write blocks until all bytes from the buffer have been written.
so_R_int_err net_Conn_Write(void* self, so_Slice b) {
    net_Conn* conn = self;
    if (conn->closed) {
        // already errored.
        return (so_R_int_err){.val = 0, .err = conn->err};
    }
    so_R_int_err _res1 = curl_WriteToSocket(conn->sock, &so_at(so_byte, b, 0), so_len(b));
    so_int n = _res1.val;
    so_Error err = _res1.err;
    if (err.self != NULL) {
        net_Conn_Close(conn);
    }
    return (so_R_int_err){.val = n, .err = err};
}

// Free Closes the connection and frees memory.
void net_Conn_Close(void* self) {
    net_Conn* conn = self;
    if (conn->closed) {
        return;
    }
    conn->closed = true;
    curl_CloseSocket(conn->sock);
}

// Dial dials the connection with a default timeout.
net_ConnResult net_Dial(so_String host) {
    host = so_string_add(so_str("http://"), host);
    net_Conn conn = (net_Conn){};
    so_R_ptr_err _res1 = curl_CreateSocket(host);
    conn.sock = _res1.val;
    conn.err = _res1.err;
    return (net_ConnResult){.val = conn, .err = conn.err};
}

// -- steppedReader.go --

// Will read sizeof(T) bytes.
// Step is a no-op if the reading has completed.
so_R_bool_err net_SteppedReader_Step(void* self, net_BufferedReader* rd) {
    net_SteppedReader* r = self;
    if (r->n < 1) {
        so_R_byte_err _res1 = net_BufferedReader_ReadByte(rd);
        so_byte b = _res1.val;
        so_Error err = _res1.err;
        if (err.self != NULL) {
            if (err.self == io_ErrNoProgress.self) {
                return (so_R_bool_err){.val = false, .err = (so_Error){0}};
            }
            return (so_R_bool_err){.val = false, .err = err};
        }
        r->Buf[0] = b;
        r->n += 1;
    }
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

// // makes the steppedReader reusable
void net_SteppedReader_Reset(void* self) {
    net_SteppedReader* r = self;
    r->n = 0;
}

// return size of the value being read.
so_int net_SteppedReader_Len(void* self) {
    net_SteppedReader* r = self;
    return 1;
}

// Will read sizeof(T) bytes.
// Step is a no-op if the reading has completed.
so_R_bool_err net_SteppedReader16_Step(void* self, net_BufferedReader* rd) {
    net_SteppedReader16* r = self;
    for (; r->n < 2;) {
        // no-op if fully read sizeof(T)
        so_R_int_err _res1 = net_BufferedReader_Read(rd, so_array_slice(so_byte, r->Buf, r->n, 2, 2));
        so_int n = _res1.val;
        so_Error err = _res1.err;
        r->n += n;
        if (err.self != NULL) {
            if (err.self == io_ErrNoProgress.self) {
                return (so_R_bool_err){.val = false, .err = (so_Error){0}};
            }
            return (so_R_bool_err){.val = false, .err = err};
        }
        if (n == 0) {
            return (so_R_bool_err){.val = false, .err = (so_Error){0}};
        }
    }
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

// // makes the steppedReader reusable
void net_SteppedReader16_Reset(void* self) {
    net_SteppedReader16* r = self;
    r->n = 0;
}

// return size of the value being read.
so_int net_SteppedReader16_Len(void* self) {
    net_SteppedReader16* r = self;
    return 2;
}

// Will read sizeof(T) bytes.
// Step is a no-op if the reading has completed.
so_R_bool_err net_SteppedReader32_Step(void* self, net_BufferedReader* rd) {
    net_SteppedReader32* r = self;
    for (; r->n < 4;) {
        // no-op if fully read sizeof(T)
        so_R_int_err _res1 = net_BufferedReader_Read(rd, so_array_slice(so_byte, r->Buf, r->n, 4, 4));
        so_int n = _res1.val;
        so_Error err = _res1.err;
        r->n += n;
        if (err.self != NULL) {
            if (err.self == io_ErrNoProgress.self) {
                return (so_R_bool_err){.val = false, .err = (so_Error){0}};
            }
            return (so_R_bool_err){.val = false, .err = err};
        }
        if (n == 0) {
            return (so_R_bool_err){.val = false, .err = (so_Error){0}};
        }
    }
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

// // makes the steppedReader reusable
void net_SteppedReader32_Reset(void* self) {
    net_SteppedReader32* r = self;
    r->n = 0;
}

// return size of the value being read.
so_int net_SteppedReader32_Len(void* self) {
    net_SteppedReader32* r = self;
    return 4;
}

so_int net_SteppedReader64_N(net_SteppedReader64 s) {
    return s.n;
}

// Will read sizeof(T) bytes.
// Step is a no-op if the reading has completed.
so_R_bool_err net_SteppedReader64_Step(void* self, net_BufferedReader* rd) {
    net_SteppedReader64* r = self;
    for (; r->n < 8;) {
        // no-op if fully read sizeof(T)
        so_R_int_err _res1 = net_BufferedReader_Read(rd, so_array_slice(so_byte, r->Buf, r->n, 8, 8));
        so_int n = _res1.val;
        so_Error err = _res1.err;
        r->n += n;
        if (err.self != NULL) {
            if (err.self == io_ErrNoProgress.self) {
                return (so_R_bool_err){.val = false, .err = (so_Error){0}};
            }
            return (so_R_bool_err){.val = false, .err = err};
        }
        if (n == 0) {
            return (so_R_bool_err){.val = false, .err = (so_Error){0}};
        }
    }
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

// // makes the steppedReader reusable
void net_SteppedReader64_Reset(void* self) {
    net_SteppedReader64* r = self;
    r->n = 0;
}

so_int net_SteppedReader64_Len(void* self) {
    net_SteppedReader64* r = self;
    return 8;
}
