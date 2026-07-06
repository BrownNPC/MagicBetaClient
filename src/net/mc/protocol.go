package mc

import (
	"solod.dev/so/encoding/binary"
	"solod.dev/so/io"
	"solod.dev/so/math"
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

// -------------------- FLOAT / DOUBLE --------------------

func WriteFloat32(w io.Writer, v float32) error {
	var b [4]byte
	binary.BigEndian.PutUint32(b[:], math.Float32bits(v))
	_, err := w.Write(b[:])
	return err
}

func WriteFloat64(w io.Writer, v float64) error {
	var b [8]byte
	binary.BigEndian.PutUint64(b[:], math.Float64bits(v))
	_, err := w.Write(b[:])
	return err
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

func WriteString16(w io.Writer, s String16) error {
	if len(s) == 0 {
		return nil
	}
	runes := []rune(s)

	if err := WriteUnsignedShort(w, uint16(len(runes))); err != nil {
		return err
	}

	for _, r := range runes {
		if err := WriteUnsignedShort(w, uint16(r)); err != nil {
			return err
		}
	}

	return nil
}
