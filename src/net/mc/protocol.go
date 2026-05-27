package mc

import (
	"solod.dev/so/encoding/binary"
	"solod.dev/so/io"
	"solod.dev/so/mem"
)

// -------------------- BYTE --------------------

func WriteByte(w io.Writer, v byte) error {
	_, err := w.Write([]byte{v})
	return err
}

// -------------------- UINT16 / INT16 --------------------

func WriteUnsignedShort(w io.Writer, v uint16) error {
	var b [2]byte
	binary.BigEndian.PutUint16(b[:], v)
	_, err := w.Write(b[:])
	return err
}

func WriteShort(w io.Writer, v int16) error {
	return WriteUnsignedShort(w, uint16(v))
}

// -------------------- UINT32 / INT32 --------------------

func WriteUnsignedInteger(w io.Writer, v uint32) error {
	var b [4]byte
	binary.BigEndian.PutUint32(b[:], v)
	_, err := w.Write(b[:])
	return err
}

func WriteInteger(w io.Writer, v int32) error {
	return WriteUnsignedInteger(w, uint32(v))
}

// -------------------- UINT64 / INT64 --------------------

func WriteUnsignedLong(w io.Writer, v uint64) error {
	var b [8]byte
	binary.BigEndian.PutUint64(b[:], v)
	_, err := w.Write(b[:])
	return err
}

func WriteLong(w io.Writer, v int64) error {
	return WriteUnsignedLong(w, uint64(v))
}

// -------------------- BOOL --------------------

func WriteBool(w io.Writer, v bool) error {
	var b byte
	if v {
		b = 1
	}
	return WriteByte(w, b)
}

// -------------------- STRING8 (UTF-8) --------------------

func WriteString8(w io.Writer, s string) error {
	if len(s) == 0 {
		return nil
	}
	if err := WriteUnsignedShort(w, uint16(len(s))); err != nil {
		return err
	}
	_, err := w.Write([]byte(s))
	return err
}

func (r *String8Reader) Step(a mem.Allocator, rd io.Reader) (bool, error) {
	switch r.step {
	case 0:
		if ok, err := r.lenReader.Step(rd); !ok {
			return ok, err
		}
		r.length = int(binary.BigEndian.Uint16(r.lenReader.Buf[:]))

		if r.length == 0 {
			return true, nil
		}

		bytes, err := mem.TryAllocSlice[byte](a, int(r.length), int(r.length))
		if err != nil {
			return false, err
		}
		r.bytes = bytes

		r.step++ //step
	case 1:
		for r.bytesIndex < r.length {
			if ok, err := r.byteReader.Step(rd); !ok {
				return ok, err
			}

			r.bytes[r.bytesIndex] = r.byteReader.Buf[0]
			r.byteReader.Reset()

			r.bytesIndex++
		}
		r.step++ //step
	}
	return true, nil
}

// -------------------- STRING16 (UCS-2 / UTF-16 subset) --------------------

func (r *String16Reader) Step(a mem.Allocator, rd io.Reader) (bool, error) {
	switch r.step {
	case 0:
		if ok, err := r.lenReader.Step(rd); !ok {
			return ok, err
		}
		r.length = int(binary.BigEndian.Uint16(r.lenReader.Buf[:]))

		if r.length == 0 {
			return true, nil
		}

		runes, err := mem.TryAllocSlice[rune](a, int(r.length), int(r.length))
		if err != nil {
			return false, err
		}
		r.Runes = runes

		r.step++ //step
	case 1:
		for r.runesIndex < r.length {
			if ok, err := r.runeReader.Step(rd); !ok {
				return ok, err
			}

			v := binary.BigEndian.Uint16(r.runeReader.Buf[:])
			r.runeReader.Reset()

			r.Runes[r.runesIndex] = rune(v)
			r.runesIndex++
		}
		r.step++ //step
	}
	return true, nil
}

func WriteString16(w io.Writer, s String16) error {
	if len(s) == 0 {
		return nil
	}
	runes := []rune(s)

	if err := WriteShort(w, int16(len(runes))); err != nil {
		return err
	}

	for _, r := range runes {
		if err := WriteUnsignedShort(w, uint16(r)); err != nil {
			return err
		}
	}

	return nil
}
