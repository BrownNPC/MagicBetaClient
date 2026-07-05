package mc

import (
	"mbc/net"
	"mbc/sdl"

	"solod.dev/so/c"
	"solod.dev/so/encoding/binary"
	"solod.dev/so/errors"
	"solod.dev/so/io"
	"solod.dev/so/mem"
	"solod.dev/so/slices"
)

type EntityMetadata struct {
	// All entities have this
	Burning, Sneaking, Riding bool

	X, Y, Z int

	SheepColor uint8 // 0-15 color ID
	// Sheep is sheared
	Sheared bool

	// Creeper blowing up
	BlowingUp bool
	//Creeper charged
	Charged bool
	// Used by Ghast
	Attacking bool
	// Slime size
	Size uint8
	// If pig is being ridden.
	Saddled bool
	// Wolf is sitting
	Sitting bool
	// Wolf health
	Health int32
}
type metadataValue struct {
	ID       uint8 // Metadata ID from header
	DataType uint8 // 0-7 corresponding to field

	Byte  byte
	rByte net.SteppedReader

	Short  int16
	rShort net.SteppedReader16

	Integer  int32
	rInteger net.SteppedReader32

	Float  float32
	rFloat net.SteppedReader32

	String  String16
	rString String16Reader

	Item struct {
		ID  int16
		rID net.SteppedReader16

		Quantity  uint8
		rQuantity net.SteppedReader

		Metadata  uint16
		rMetadata net.SteppedReader16
	}
	Coordinates struct {
		X, Y, Z    int32
		rX, rY, rZ net.SteppedReader32
	}
}

type MetadataReader struct {
	metadataValues []metadataValue

	header     net.SteppedReader
	dataType   uint8 // from header
	metadataID uint8 // from header

	// Current metadataValue being read
	metadata metadataValue

	state uint8
}

func (r *MetadataReader) Parse(e EntityType) EntityMetadata {
	var m EntityMetadata
	for _, value := range r.metadataValues {
		switch value.ID {
		case 0: // base flags
			m.Burning = value.Byte&0x01 != 0
			m.Sneaking = value.Byte&0x02 != 0
			m.Riding = value.Byte&0x03 != 0
		case 16:
			switch e {
			case MOB_Pig:
				m.Saddled = value.Byte != 0
			case MOB_Creeper:
				m.BlowingUp = value.Byte == 1
			case MOB_Sheep:
				m.Sheared = value.Byte == 16
				if value.Byte < 16 {
					m.SheepColor = value.Byte
				}
			case MOB_Slime:
				m.Size = value.Byte
			case MOB_Ghast:
				m.Attacking = value.Byte != 0
			case MOB_Wolf:
				m.Sitting = value.Byte != 0
			}
		case 17:
			if e == MOB_Creeper {
				m.Charged = value.Byte != 0
			}
		case 18:
			if e == MOB_Wolf {
				m.Health = value.Integer
			}
		}
	}
	return m
}

var IncompleteMetadataErr = errors.New("Tried parsing incomplete metadata")

func (m *MetadataReader) Step(a mem.Allocator, rd io.Reader) (bool, error) {
	const (
		READING_HEADER = iota
		// STANDARD DATATYPES
		READING_BYTE
		READING_SHORT
		READING_INTEGER
		READING_FLOAT
		READING_STRING
		// ITEM
		READING_ITEM_ID
		READING_ITEM_QUANTITY
		READING_ITEM_METADATA
		//COORDINATES
		READING_COORDINATE_X
		READING_COORDINATE_Y
		READING_COORDINATE_Z
		// 127 value recieved
		COMPLETED
	)

	switch m.state {
	case COMPLETED:
		return true, nil
	case READING_BYTE:
		ok, err := m.metadata.rByte.Step(rd)
		if !ok {
			return false, err
		}
		m.metadata.Byte = m.metadata.rByte.Buf[0]
		// save
		m.metadataValues = slices.Append(a, m.metadataValues, m.metadata)
		// read next header
		m.state = READING_HEADER
	case READING_SHORT:
		ok, err := m.metadata.rShort.Step(rd)
		if !ok {
			return false, err
		}
		m.metadata.Short = int16(binary.BigEndian.Uint16(m.metadata.rShort.Buf[:]))
		// save
		m.metadataValues = slices.Append(a, m.metadataValues, m.metadata)
		// read next header
		m.state = READING_HEADER
	case READING_INTEGER:
		ok, err := m.metadata.rInteger.Step(rd)
		if !ok {
			return false, err
		}
		m.metadata.Integer = int32(binary.BigEndian.Uint32(m.metadata.rInteger.Buf[:]))
		// save
		m.metadataValues = slices.Append(a, m.metadataValues, m.metadata)
		// read next header
		m.state = READING_HEADER
	case READING_FLOAT:
		// beta 1.7.3 servers do not actually send this, but it is good to implement it
		ok, err := m.metadata.rFloat.Step(rd)
		if !ok {
			return false, err
		}
		m.metadata.Float = float32(binary.BigEndian.Uint32(m.metadata.rFloat.Buf[:]))
		// save
		m.metadataValues = slices.Append(a, m.metadataValues, m.metadata)
		// read next header
		m.state = READING_HEADER
		sdl.Log("Server sent float32 metadata. Vanilla servers do not do this.")
	case READING_STRING:
		// this is only sent for wolf owner.
		// This data is not used. we just parse it because we need to
		// not break the stream.
		//
		// The reason we dont use the wolf owner string is because
		// dealing with memory allocations for something that's not used
		// is pointless annoyance.
		ok, err := m.metadata.rString.Step(a, rd)
		if !ok {
			return false, err
		}
		m.metadata.String = m.metadata.rString.Runes
		// save
		m.metadataValues = slices.Append(a, m.metadataValues, m.metadata)
		// read next header
		m.state = READING_HEADER
	case READING_ITEM_ID:
		ok, err := m.metadata.Item.rID.Step(rd)
		if !ok {
			return false, err
		}
		m.metadata.Item.ID = int16(binary.BigEndian.Uint16(m.metadata.Item.rID.Buf[:]))
		// read next field
		m.state = READING_ITEM_QUANTITY
	case READING_ITEM_QUANTITY:
		ok, err := m.metadata.Item.rQuantity.Step(rd)
		if !ok {
			return false, err
		}
		m.metadata.Item.Quantity = m.metadata.Item.rQuantity.Buf[0]
		// read next field
		m.state = READING_ITEM_METADATA
	case READING_ITEM_METADATA:
		ok, err := m.metadata.Item.rMetadata.Step(rd)
		if !ok {
			return false, err
		}
		m.metadata.Item.Metadata = binary.BigEndian.Uint16(m.metadata.Item.rMetadata.Buf[:])
		// save
		m.metadataValues = slices.Append(a, m.metadataValues, m.metadata)
		// read next header
		m.state = READING_HEADER
	case READING_COORDINATE_X:
		ok, err := m.metadata.Coordinates.rX.Step(rd)
		if !ok {
			return false, err
		}
		m.metadata.Coordinates.X = int32(binary.BigEndian.Uint32(m.metadata.Coordinates.rX.Buf[:]))
		m.state = READING_COORDINATE_Y
	case READING_COORDINATE_Y:
		ok, err := m.metadata.Coordinates.rY.Step(rd)
		if !ok {
			return false, err
		}
		m.metadata.Coordinates.Y = int32(binary.BigEndian.Uint32(m.metadata.Coordinates.rY.Buf[:]))
		m.state = READING_COORDINATE_Z
	case READING_COORDINATE_Z:

		ok, err := m.metadata.Coordinates.rZ.Step(rd)
		if !ok {
			return false, err
		}
		m.metadata.Coordinates.Z = int32(binary.BigEndian.Uint32(m.metadata.Coordinates.rZ.Buf[:]))
		// save
		m.metadataValues = slices.Append(a, m.metadataValues, m.metadata)
		// read next header
		m.state = READING_HEADER
	case READING_HEADER:
		ok, err := m.header.Step(rd)
		if !ok {
			return false, err
		}
		value := m.header.Buf[0]
		if value == 127 {
			m.state = COMPLETED
			return true, nil
		}
		m.dataType = value >> 5
		m.metadataID = value & 0x1F
		switch m.dataType {
		case 0, 1, 2, 3, 4:
			m.state = READING_BYTE + m.dataType
		case 5:
			m.state = READING_ITEM_ID
		case 6:
			m.state = READING_COORDINATE_X
		default:
			panic("INVALID METADATA TYPE")
		}
		m.header.Reset()
		m.metadata = c.Zero[metadataValue]()
		m.metadata.DataType = m.dataType
		m.metadata.ID = m.metadataID
	}
	return false, nil
}
