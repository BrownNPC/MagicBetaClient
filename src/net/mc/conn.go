package mc

import (
	"betamine/net"

	"solod.dev/so/bufio"
	"solod.dev/so/fmt"

	"solod.dev/so/encoding/binary"
	"solod.dev/so/io"
	"solod.dev/so/mem"
)

type ConnMode uint8

const (
	CONN_MODE_CLIENT ConnMode = iota
	CONN_MODE_SERVER
)

// Conn provides methods for creating / sending / receiving Minecraft server packets.
type Conn struct {
	Mode   ConnMode
	Sock   *net.Conn
	Closed bool
	rbuf   bufio.Reader
	wbuf   bufio.Writer
	Log    bool
}
type ConnResult struct {
	val Conn
	err error
}

// Dial creates a connection to the Minecraft server.
func Dial(a mem.Allocator, hostname string) (Conn, error) {
	conn, err := net.Dial(hostname)
	if err != nil {
		return Conn{}, err
	}
	conn_heap := mem.Alloc[net.Conn](a)
	*conn_heap = conn
	return Conn{
		Sock: conn_heap,
		rbuf: bufio.NewReader(a, conn_heap),
		wbuf: bufio.NewWriter(a, conn_heap),
	}, nil
}

func WriteByte(w io.Writer, v byte) error {
	_, err := w.Write([]byte{v})
	return err
}

func ReadByte(r io.Reader) (byte, error) {
	b := make([]byte, 1)
	_, err := r.Read(b)
	return b[0], err
}

func (conn *Conn) WriteUint16(v uint16) {
	var b [2]byte
	binary.BigEndian.PutUint16(b[:], v)
	conn.mustWrite(b[:])
}
func (conn *Conn) ReadUint16() uint16 {
	var b [2]byte
	conn.mustRead(b[:])
	return binary.BigEndian.Uint16(b[:])
}

func (conn *Conn) WriteInt16(v int16) { conn.WriteUint16(uint16(v)) }
func (conn *Conn) ReadInt16() int16   { return int16(conn.ReadUint16()) }

func (conn *Conn) WriteUint32(v uint32) {
	var b [4]byte
	binary.BigEndian.PutUint32(b[:], v)
	conn.mustWrite(b[:])
}
func (conn *Conn) ReadUint32() uint32 {
	var b [4]byte
	conn.mustRead(b[:])
	return binary.BigEndian.Uint32(b[:])
}

func (conn *Conn) WriteInt32(v int32) { conn.WriteUint32(uint32(v)) }
func (conn *Conn) ReadInt32() int32   { return int32(conn.ReadUint32()) }

func (conn *Conn) WriteUint64(v uint64) {
	var b [8]byte
	binary.BigEndian.PutUint64(b[:], v)
	conn.mustWrite(b[:])
}
func (conn *Conn) ReadUint64() uint64 {
	var b [8]byte
	conn.mustRead(b[:])
	return binary.BigEndian.Uint64(b[:])
}

func (conn *Conn) WriteInt64(v int64) { conn.WriteUint64(uint64(v)) }
func (conn *Conn) ReadInt64() int64   { return int64(conn.ReadUint64()) }

func (conn *Conn) WriteBool(v bool) {
	var b int8
	if v {
		b = 1
	}
	conn.WriteInt8(b)
}
func (conn *Conn) ReadBool() bool { return conn.ReadInt8() != 0 }

func (conn *Conn) WriteString8(s string) {
	b := []byte(s)
	length := uint16(len(s))
	conn.WriteUint16(length)
	conn.mustWrite(b)
}

func (conn *Conn) ReadString8(a mem.Allocator) string {
	length := conn.ReadUint16()
	if length == 0 {
		return ""
	}
	buf := mem.AllocSlice[byte](a, int(length), int(length))
	conn.mustRead(buf)
	return string(buf)
}

func (conn *Conn) WriteString16(s string) {
	runes := []rune(s)
	length := len(runes)

	conn.WriteInt16(int16(length))
	for _, r := range runes {
		conn.WriteUint16(uint16(r))
	}
}

func (conn *Conn) ReadString16(a mem.Allocator) string {
	length := int(conn.ReadUint16()) // number of characters UCS-2

	runes := mem.AllocSlice[rune](a, 0, length)
	// valid because Minecraft uses UCS-2 which has 2 byte pairs.
	for range length {
		u := conn.ReadUint16()
		runes = append(runes, rune(u))
	}

	return string(runes)
}

// does io.ReadFull while logging errors.
func (conn *Conn) mustRead(p []byte) {
	_, err := io.ReadFull(&conn.rbuf, p)
	if err != nil {
		println("net/mc/conn Conn.mustRead: failed to read", err)
		conn.Closed = true
	}
}

// does io.ReadFull while logging errors.
func (conn *Conn) mustWrite(p []byte) {
	_, err := conn.wbuf.Write(p)
	if conn.Log {
		fmt.Println("----------")
		for _, byte := range p {
			fmt.Printf("0x%02x | %d\n", byte, byte)
		}
	}
	if err != nil {
		conn.Closed = true
		println("net/mc/conn Conn.mustWrite: failed to write", err)
	}
}
