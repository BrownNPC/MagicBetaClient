package net

// copied the implementation here to set maxConsecutiveReads to 0.
import (
	"solod.dev/so/bytes"
	"solod.dev/so/errors"
	"solod.dev/so/io"
	"solod.dev/so/mem"
	"solod.dev/so/unicode/utf8"
)

// BufferedReader implements buffering for an io.BufferedReader object.
//
// The caller is responsible for freeing the reader's resources
// with [BufferedReader.Free] when done using it.
type BufferedReader struct {
	a            mem.Allocator
	buf          []byte
	rd           io.Reader // reader provided by the client
	r, w         int       // buf read and write positions
	err          error
	lastByte     int // last byte read for UnreadByte; -1 means invalid
	lastRuneSize int // size of last rune read for UnreadRune; -1 means invalid
}

const minReadBufferSize = 1
const maxConsecutiveEmptyReads = 1
const DefaultBufSize = 4096 * 10

var (
	ErrInvalidUnreadByte = errors.New("bufio: invalid use of UnreadByte")
	ErrInvalidUnreadRune = errors.New("bufio: invalid use of UnreadRune")
	ErrBufferFull        = errors.New("bufio: buffer full")
	ErrNegativeCount     = errors.New("bufio: negative count")
)

// NewBufferedReaderSize returns a new [BufferedReader] whose buffer has at least the specified
// size. If the argument io.Reader is already a [BufferedReader] with large enough
// size, it returns the underlying [BufferedReader].
func NewBufferedReaderSize(a mem.Allocator, rd io.Reader, size int) BufferedReader {
	// Is it already a Reader?
	if _, ok := rd.(*BufferedReader); ok {
		b := rd.(*BufferedReader)
		if len(b.buf) >= size {
			return *b
		}
	}
	sz := max(size, minReadBufferSize)
	buf := mem.AllocSlice[byte](a, sz, sz)
	r := BufferedReader{a: a}
	r.reset(buf, rd)
	return r
}

// NewBufferedReader returns a new [BufferedReader] whose buffer has the default size.
func NewBufferedReader(a mem.Allocator, rd io.Reader) BufferedReader {
	return NewBufferedReaderSize(a, rd, DefaultBufSize)
}

// Size returns the size of the underlying buffer in bytes.
func (b *BufferedReader) Size() int { return len(b.buf) }

// Reset discards any buffered data, resets all state, and switches
// the buffered reader to read from r.
// Calling Reset on the zero value of [BufferedReader] initializes the internal buffer
// to the default size.
// Calling b.Reset(b) (that is, resetting a [BufferedReader] to itself) does nothing.
func (b *BufferedReader) Reset(r io.Reader) {
	// Avoid no-op reset to self.
	_, ok := r.(*BufferedReader)
	if ok && b == r.(*BufferedReader) {
		return
	}
	if b.buf == nil {
		b.buf = mem.AllocSlice[byte](b.a, DefaultBufSize, DefaultBufSize)
	}
	b.reset(b.buf, r)
}

// Free releases the internal reader's buffer.
// The reader must not be used after calling Free.
func (b *BufferedReader) Free() {
	mem.FreeSlice(b.a, b.buf)
	b.buf = nil
}

func (b *BufferedReader) reset(buf []byte, r io.Reader) {
	b.buf = buf
	b.rd = r
	b.lastByte = -1
	b.lastRuneSize = -1
}

var errNegativeRead = errors.New("bufio: reader returned negative count from Read")

// fill reads a new chunk into the buffer.
func (b *BufferedReader) fill() {
	// Slide existing data to beginning.
	if b.r > 0 {
		copy(b.buf, b.buf[b.r:b.w])
		b.w -= b.r
		b.r = 0
	}

	if b.w >= len(b.buf) {
		panic("bufio: tried to fill full buffer")
	}

	// Read new data: try a limited number of times.
	for i := maxConsecutiveEmptyReads; i > 0; i-- {
		n, err := b.rd.Read(b.buf[b.w:])
		if n < 0 {
			panic(errNegativeRead)
		}
		b.w += n
		if err != nil {
			b.err = err
			return
		}
		if n > 0 {
			return
		}
	}
	b.err = io.ErrNoProgress
}

func (b *BufferedReader) readErr() error {
	err := b.err
	b.err = nil
	return err
}

// Peek returns the next n bytes without advancing the reader. The bytes stop
// being valid at the next read call. If necessary, Peek will read more bytes
// into the buffer in order to make n bytes available. If Peek returns fewer
// than n bytes, it also returns an error explaining why the read is short.
// The error is [ErrBufferFull] if n is larger than b's buffer size.
//
// Calling Peek prevents a [BufferedReader.UnreadByte] or [BufferedReader.UnreadRune] call from succeeding
// until the next read operation.
func (b *BufferedReader) Peek(n int) ([]byte, error) {
	if n < 0 {
		return []byte{}, ErrNegativeCount
	}

	b.lastByte = -1
	b.lastRuneSize = -1

	for b.w-b.r < n && b.w-b.r < len(b.buf) && b.err == nil {
		b.fill() // b.w-b.r < len(b.buf) => buffer is not full
	}

	if n > len(b.buf) {
		return b.buf[b.r:b.w], ErrBufferFull
	}

	// 0 <= n <= len(b.buf)
	var err error
	if avail := b.w - b.r; avail < n {
		// not enough data in buffer
		n = avail
		err = b.readErr()
		if err == nil {
			err = ErrBufferFull
		}
	}
	return b.buf[b.r : b.r+n], err
}

// Discard skips the next n bytes, returning the number of bytes discarded.
//
// If Discard skips fewer than n bytes, it also returns an error.
// If 0 <= n <= b.Buffered(), Discard is guaranteed to succeed without
// reading from the underlying io.Reader.
func (b *BufferedReader) Discard(n int) (int, error) {
	if n < 0 {
		return 0, ErrNegativeCount
	}
	if n == 0 {
		return 0, nil
	}

	b.lastByte = -1
	b.lastRuneSize = -1

	remain := n
	for {
		skip := b.Buffered()
		if skip == 0 {
			b.fill()
			skip = b.Buffered()
		}
		if skip > remain {
			skip = remain
		}
		b.r += skip
		remain -= skip
		if remain == 0 {
			return n, nil
		}
		if b.err != nil {
			return n - remain, b.readErr()
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
func (b *BufferedReader) Read(p []byte) (int, error) {
	n := len(p)
	if n == 0 {
		if b.Buffered() > 0 {
			return 0, nil
		}
		return 0, b.readErr()
	}
	if b.r == b.w {
		if b.err != nil {
			return 0, b.readErr()
		}
		if len(p) >= len(b.buf) {
			// Large read, empty buffer.
			// Read directly into p to avoid copy.
			n, b.err = b.rd.Read(p)
			if n < 0 {
				panic(errNegativeRead)
			}
			if n > 0 {
				b.lastByte = int(p[n-1])
				b.lastRuneSize = -1
			}
			return n, b.readErr()
		}
		// One read.
		// Do not use b.fill, which will loop.
		b.r = 0
		b.w = 0
		n, b.err = b.rd.Read(b.buf)
		if n < 0 {
			panic(errNegativeRead)
		}
		if n == 0 {
			return 0, b.readErr()
		}
		b.w += n
	}

	// copy as much as we can
	// Note: if the slice panics here, it is probably because
	// the underlying reader returned a bad count. See issue 49795.
	n = copy(p, b.buf[b.r:b.w])
	b.r += n
	b.lastByte = int(b.buf[b.r-1])
	b.lastRuneSize = -1
	return n, nil
}

// ReadByte reads and returns a single byte.
// If no byte is available, returns an error.
func (b *BufferedReader) ReadByte() (byte, error) {
	b.lastRuneSize = -1
	for b.r == b.w {
		if b.err != nil {
			return 0, b.readErr()
		}
		b.fill() // buffer is empty
	}
	c := b.buf[b.r]
	b.r++
	b.lastByte = int(c)
	return c, nil
}

// UnreadByte unreads the last byte. Only the most recently read byte can be unread.
//
// UnreadByte returns an error if the most recent method called on the
// [BufferedReader] was not a read operation. Notably, [BufferedReader.Peek], [BufferedReader.Discard], and [BufferedReader.WriteTo] are not
// considered read operations.
func (b *BufferedReader) UnreadByte() error {
	if b.lastByte < 0 || (b.r == 0 && b.w > 0) {
		return ErrInvalidUnreadByte
	}
	// b.r > 0 || b.w == 0
	if b.r > 0 {
		b.r--
	} else {
		// b.r == 0 && b.w == 0
		b.w = 1
	}
	b.buf[b.r] = byte(b.lastByte)
	b.lastByte = -1
	b.lastRuneSize = -1
	return nil
}

// ReadRune reads a single UTF-8 encoded Unicode character and returns the
// rune and its size in bytes. If the encoded rune is invalid, it consumes one byte
// and returns unicode.ReplacementChar (U+FFFD) with a size of 1.
func (b *BufferedReader) ReadRune() io.RuneSizeResult {
	for b.r+utf8.UTFMax > b.w && !utf8.FullRune(b.buf[b.r:b.w]) && b.err == nil && b.w-b.r < len(b.buf) {
		b.fill() // b.w-b.r < len(buf) => buffer is not full
	}
	b.lastRuneSize = -1
	if b.r == b.w {
		return io.RuneSizeResult{Err: b.readErr()}
	}
	r, size := utf8.DecodeRune(b.buf[b.r:b.w])
	b.r += size
	b.lastByte = int(b.buf[b.r-1])
	b.lastRuneSize = size
	return io.RuneSizeResult{Rune: r, Size: size}
}

// UnreadRune unreads the last rune. If the most recent method called on
// the [BufferedReader] was not a [BufferedReader.ReadRune], [BufferedReader.UnreadRune] returns an error. (In this
// regard it is stricter than [BufferedReader.UnreadByte], which will unread the last byte
// from any read operation.)
func (b *BufferedReader) UnreadRune() error {
	if b.lastRuneSize < 0 || b.r < b.lastRuneSize {
		return ErrInvalidUnreadRune
	}
	b.r -= b.lastRuneSize
	b.lastByte = -1
	b.lastRuneSize = -1
	return nil
}

// Buffered returns the number of bytes that can be read from the current buffer.
func (b *BufferedReader) Buffered() int { return b.w - b.r }

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
func (b *BufferedReader) ReadSlice(delim byte) ([]byte, error) {
	var line []byte
	var err error
	s := 0 // search start index
	for {
		// Search buffer.
		if i := bytes.IndexByte(b.buf[b.r+s:b.w], delim); i >= 0 {
			i += s
			line = b.buf[b.r : b.r+i+1]
			b.r += i + 1
			break
		}

		// Pending error?
		if b.err != nil {
			line = b.buf[b.r:b.w]
			b.r = b.w
			err = b.readErr()
			break
		}

		// Buffer full?
		if b.Buffered() >= len(b.buf) {
			b.r = b.w
			line = b.buf
			err = ErrBufferFull
			break
		}

		s = b.w - b.r // do not rescan area we scanned before

		b.fill() // buffer is not full
	}

	// Handle last byte, if any.
	if i := len(line) - 1; i >= 0 {
		b.lastByte = int(line[i])
		b.lastRuneSize = -1
	}

	return line, err
}

// ReadLineResult holds the result of a [BufferedReader.ReadLine] call.
type ReadLineResult struct {
	Line     []byte
	IsPrefix bool
	Err      error
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
func (b *BufferedReader) ReadLine() ReadLineResult {
	line, err := b.ReadSlice('\n')
	if err == ErrBufferFull {
		// Handle the case where "\r\n" straddles the buffer.
		if len(line) > 0 && line[len(line)-1] == '\r' {
			// Put the '\r' back on buf and drop it from line.
			// Let the next call to ReadLine check for "\r\n".
			if b.r == 0 {
				// should be unreachable
				panic("bufio: tried to rewind past start of buffer")
			}
			b.r--
			line = line[:len(line)-1]
		}
		return ReadLineResult{Line: line, IsPrefix: true}
	}

	if len(line) == 0 {
		if err != nil {
			return ReadLineResult{Err: err}
		}
		return ReadLineResult{}
	}

	if line[len(line)-1] == '\n' {
		drop := 1
		if len(line) > 1 && line[len(line)-2] == '\r' {
			drop = 2
		}
		line = line[:len(line)-drop]
	}
	return ReadLineResult{Line: line}
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
func (b *BufferedReader) ReadBytes(delim byte) ([]byte, error) {
	frag, err := b.ReadSlice(delim)
	if err != ErrBufferFull {
		// Fast path: delimiter found or non-full-buffer error.
		// Clone since frag points into internal buffer.
		line := mem.AllocSlice[byte](b.a, len(frag), len(frag))
		copy(line, frag)
		return line, err
	}
	// Slow path: accumulate fragments in a buffer.
	buf := bytes.NewBuffer(b.a, nil)
	for {
		buf.Write(frag)
		frag, err = b.ReadSlice(delim)
		if err != ErrBufferFull {
			break
		}
	}
	buf.Write(frag)
	result := bytes.Clone(b.a, buf.Bytes())
	buf.Free()
	return result, err
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
func (b *BufferedReader) ReadString(delim byte) (string, error) {
	data, err := b.ReadBytes(delim)
	return string(data), err
}

// WriteTo implements io.WriterTo.
// This may make multiple calls to the [BufferedReader.Read] method of the underlying [BufferedReader].
func (b *BufferedReader) WriteTo(w io.Writer) (int64, error) {
	b.lastByte = -1
	b.lastRuneSize = -1

	n := int64(0)

	if b.r < b.w {
		m, err := b.writeBuf(w)
		n += m
		if err != nil {
			return n, err
		}
	}

	if b.w-b.r < len(b.buf) {
		b.fill() // buffer not full
	}

	for b.r < b.w {
		// b.r < b.w => buffer is not empty
		m, err := b.writeBuf(w)
		n += m
		if err != nil {
			return n, err
		}
		b.fill() // buffer is empty
	}

	if b.err == io.EOF {
		b.err = nil
	}

	return n, b.readErr()
}

var errNegativeWrite = errors.New("bufio: writer returned negative count from Write")

// writeBuf writes the [BufferedReader]'s buffer to the writer.
func (b *BufferedReader) writeBuf(w io.Writer) (int64, error) {
	n, err := w.Write(b.buf[b.r:b.w])
	if n < 0 {
		panic(errNegativeWrite)
	}
	b.r += n
	return int64(n), err
}
