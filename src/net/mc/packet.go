package mc

import (
	"mbc/net"

	"solod.dev/so/encoding/binary"
	"solod.dev/so/io"
	"solod.dev/so/mem"
)

type String16 = []rune

// a zero value string16Reader is valid to use.
type String8Reader struct {
	step int

	length    int
	lenReader net.SteppedReader16

	byteReader net.SteppedReader

	bytesIndex int
	bytes      []byte
}
// a zero value String16Reader is valid to use.
type String16Reader struct {
	step int

	length    int
	lenReader net.SteppedReader16

	runeReader net.SteppedReader16

	runesIndex int
	Runes      []rune
}
// https://pixelbrush.dev/beta-wiki/networking/packets/000-keep-alive
type PacketKeepAlive struct {
	// no body
	_ byte
}

// Read implements [ClientBoundPacket].
func (p *PacketKeepAlive) Step(mem.Allocator, io.Reader) (bool, error) {
	return true, nil
}

// Write implements [ServerBoundPacket].
func (p PacketKeepAlive) Write(io.Writer) error {
	return nil
}

// https://pixelbrush.dev/beta-wiki/networking/packets/001-login
type ClientboundLogin struct {
	entityID net.SteppedReader32
	EntityID int32

	_      String16 //unused
	unused String16Reader

	WorldSeed int64
	worldSeed net.SteppedReader64

	Dimension uint8
	dimension net.SteppedReader64

	step int
}

func (p *ClientboundLogin) Step(a mem.Allocator, r io.Reader) (bool, error) {
	switch p.step {
	case 0:
		if ok, err := p.entityID.Step( r); !ok {
			return ok, err
		}
		p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
		p.step++ //step
	case 1:
		if ok, err := p.unused.Step(a, r); !ok {
			return ok, err
		}
		p.step++ //step
	case 2:
		if ok, err := p.worldSeed.Step( r); !ok {
			return ok, err
		}
		p.WorldSeed = int64(binary.BigEndian.Uint64(p.worldSeed.Buf[:]))
		p.step++ //step
	case 3:
		if ok, err := p.dimension.Step( r); !ok {
			return ok, err
		}
		p.Dimension = p.dimension.Buf[0]
		p.step++ //step
	}
	return true, nil
}

type ServerboundLogin struct {
	ProtocolVersion int32
	Username        String16
	_               int64
	__              byte
}

func (p ServerboundLogin) Write(w io.Writer) error {
	if err := WriteInteger(w, p.ProtocolVersion); err != nil {
		return err
	}
	if err := WriteString16(w, p.Username); err != nil {
		return err
	}
	if err := WriteLong(w, 0); err != nil {
		return err
	}
	if err := WriteByte(w, 0); err != nil {
		return err
	}
	return nil
}

type ClientboundPreLogin struct {
	ConnectionHash String16
}

type ServerboundPreLogin struct {
	Username String16
}

func (p ServerboundPreLogin) Write(w io.Writer) error {
	if err := WriteString16(w, p.Username); err != nil {
		return err
	}
	return nil
}

type PacketChatMessage struct {
	Message String16
}

func (p *PacketChatMessage) Write(w io.Writer) error {
	if err := WriteString16(w, p.Message); err != nil {
		return err
	}
	return nil
}
