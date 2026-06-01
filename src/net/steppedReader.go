package net

import (
	"solod.dev/so/io"
)

// A reader that reads in steps
// zero value is a valid reader.
type SteppedReader struct {
	Buf [1]byte
	n   int
}

// Will read sizeof(T) bytes.
// Step is a no-op if the reading has completed.
func (r *SteppedReader) Step(rd io.Reader) (bool, error) {
	for r.n < len(r.Buf) { // no-op if fully read sizeof(T)
		n, err := rd.Read(r.Buf[r.n:])
		r.n += n
		if err != nil {
			if err == io.ErrNoProgress {
				return false, nil
			}
			return false, err
		}
		if n == 0 {
			return false, nil
		}
	}
	return true, nil
}

// // makes the steppedReader reusable
func (r *SteppedReader) Reset()   { r.n = 0 }
func (r *SteppedReader) Len() int { return len(r.Buf) } // return size of the value being read.
// A reader that reads in steps
// zero value will cause Step to always be a no-op.
type SteppedReader16 struct {
	Buf [2]byte
	n   int
}

// Will read sizeof(T) bytes.
// Step is a no-op if the reading has completed.
func (r *SteppedReader16) Step(rd io.Reader) (bool, error) {
	for r.n < len(r.Buf) { // no-op if fully read sizeof(T)
		n, err := rd.Read(r.Buf[r.n:])
		r.n += n
		if err != nil {
			if err == io.ErrNoProgress {
				return false, nil
			}
			return false, err
		}
		if n == 0 {
			return false, nil
		}
	}
	return true, nil
}

// // makes the steppedReader reusable
func (r *SteppedReader16) Reset()   { r.n = 0 }
func (r *SteppedReader16) Len() int { return len(r.Buf) } // return size of the value being read.
// A reader that reads in steps
// zero value will cause Step to always be a no-op.
type SteppedReader32 struct {
	Buf [4]byte
	n   int
}

// Will read sizeof(T) bytes.
// Step is a no-op if the reading has completed.
func (r *SteppedReader32) Step(rd io.Reader) (bool, error) {
	for r.n < len(r.Buf) { // no-op if fully read sizeof(T)
		n, err := rd.Read(r.Buf[r.n:])
		r.n += n
		if err != nil {
			if err == io.ErrNoProgress {
				return false, nil
			}
			return false, err
		}
		if n == 0 {
			return false, nil
		}
	}
	return true, nil
}

// // makes the steppedReader reusable
func (r *SteppedReader32) Reset()   { r.n = 0 }
func (r *SteppedReader32) Len() int { return len(r.Buf) } // return size of the value being read.
// A reader that reads in steps
// zero value will cause Step to always be a no-op.
type SteppedReader64 struct {
	Buf [8]byte
	n   int
}

// Will read sizeof(T) bytes.
// Step is a no-op if the reading has completed.
func (r *SteppedReader64) Step(rd io.Reader) (bool, error) {
	for r.n < len(r.Buf) { // no-op if fully read sizeof(T)
		n, err := rd.Read(r.Buf[r.n:])
		r.n += n
		if err != nil {
			if err == io.ErrNoProgress {
				return false, nil
			}
			return false, err
		}
		if n == 0 {
			return false, nil
		}
	}
	return true, nil
}

// // makes the steppedReader reusable
func (r *SteppedReader64) Reset()   { r.n = 0 }
func (r *SteppedReader64) Len() int { return len(r.Buf) } // return size of the value being read.
