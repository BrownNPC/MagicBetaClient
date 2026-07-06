package net

// copied the implementation here to set maxConsecutiveReads to 0.
import (
	"solod.dev/so/errors"
	"solod.dev/so/io"
	"solod.dev/so/mem"
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
func (b *BufferedReader) Size() int  { return len(b.buf) }
func (b *BufferedReader) Err() error { return b.readErr() }

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
}

func (b *BufferedReader) readErr() error {
	err := b.err
	b.err = nil
	return err
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

// Buffered returns the number of bytes that can be read from the current buffer.
func (b *BufferedReader) Buffered() int { return b.w - b.r }

// Read n bytes in full.
func (b *BufferedReader) ReadFull(buf []byte) bool {
	if len(buf) == 0 {
		return true
	}
	if !b.require(len(buf)) {
		return false
	}
	n, err := b.Read(buf)
	if err != nil {
		b.err = err
		return false
	}
	if n != len(buf) {
		panic("Should have been able to fully read into buf")
	}
	return true
}

// returns true if we have n bytes in the buffer.
func (b *BufferedReader) require(n int) bool {
	if b.w-b.r < n {
		b.fill()
		if b.w-b.r < n {
			return false
		}
	}
	return true
}
func (b *BufferedReader) ReadUint8(dst *uint8) bool {
	if b.r >= b.w && !b.require(1) {
		return false
	}
	*dst = b.buf[b.r]
	b.r++
	return true
}
func (b *BufferedReader) ReadBool(dst *bool) bool {
	var v uint8
	if !b.ReadUint8(&v) {
		return false
	}
	*dst = v != 0
	return true
}
func (b *BufferedReader) ReadInt8(dst *int8) bool {
	if b.r >= b.w && !b.require(1) {
		return false
	}
	*dst = int8(b.buf[b.r])
	b.r++
	return true
}
func (b *BufferedReader) ReadUint16(dst *uint16) bool {
	if !b.require(2) {
		return false
	}

	p := b.buf[b.r:]

	*dst =
		uint16(p[0])<<8 |
			uint16(p[1])

	b.r += 2
	return true
}
func (b *BufferedReader) ReadUint32(dst *uint32) bool {
	if !b.require(4) {
		return false
	}

	p := b.buf[b.r:]

	*dst =
		uint32(p[0])<<24 |
			uint32(p[1])<<16 |
			uint32(p[2])<<8 |
			uint32(p[3])

	b.r += 4
	return true
}
func (b *BufferedReader) ReadUint64(dst *uint64) bool {
	if !b.require(8) {
		return false
	}

	p := b.buf[b.r:]

	*dst =
		uint64(p[0])<<56 |
			uint64(p[1])<<48 |
			uint64(p[2])<<40 |
			uint64(p[3])<<32 |
			uint64(p[4])<<24 |
			uint64(p[5])<<16 |
			uint64(p[6])<<8 |
			uint64(p[7])

	b.r += 8
	return true
}
func (b *BufferedReader) ReadInt16(dst *int16) bool {
	var v uint16
	if !b.ReadUint16(&v) {
		return false
	}
	*dst = int16(v)
	return true
}
func (b *BufferedReader) ReadInt32(dst *int32) bool {
	var v uint32
	if !b.ReadUint32(&v) {
		return false
	}
	*dst = int32(v)
	return true
}

func (b *BufferedReader) ReadInt64(dst *int64) bool {
	var v uint64
	if !b.ReadUint64(&v) {
		return false
	}
	*dst = int64(v)
	return true
}
