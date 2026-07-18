package mc

import (
	"mbc/net"
	"mbc/net/zlib"

	"solod.dev/so/io"
	"solod.dev/so/mem"
	"solod.dev/so/slices"
)

// rotation data is quantized to only a single 8-bit Byte.
func UnquantizeAngle(angle int8) float32 {
	if angle == 0 {
		return 0
	}
	return (float32(angle) / 255.0) * 360
}
func UnquantizeCoordinate(coord int32) float32 {
	if coord == 0 {
		return 0
	}
	return float32(coord) / 32
}
func UnclampVelocity(vel int16) float32 {
	if vel == 0 {
		return 0
	}
	return (float32(vel) / 8000)
}

// everything implements the Decoder interface.
// It reads in a non-blocking fashion.
// returns (false,err) if there's an error.
// or (false,nil) if reading is still in progress.
//
// It returns (true,nil) if reading has finished.
type Decoder interface {
	Step(a mem.Allocator, rd *net.BufferedReader) (bool, error)
}

// An encoder should block until everything has been written.
// It should also write the packet id.
type Encoder interface {
	Write(io.Writer) error
}

type String16 = []rune

type String8Reader struct {
	len   uint16
	buf   []byte
	stage uint8
}

func (s String8Reader) String() string { return string(s.buf) }

func (s *String8Reader) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch s.stage {
		case 0:
			if !rd.ReadUint16(&s.len) {
				return false, rd.Err()
			}
			s.buf = slices.Make[byte](a, int(s.len))
			s.stage++
		case 1:
			if !rd.ReadFull(s.buf) {
				return false, rd.Err()
			}
		case 2:
			return true, rd.Err()
		}
		s.stage++
	}
}

type String16Reader struct {
	totalCharacters uint16
	Runes           []rune

	stage uint8
}

func (s *String16Reader) Reset() { *s = String16Reader{} }

func (s *String16Reader) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch s.stage {
		case 0:
			if !rd.ReadUint16(&s.totalCharacters) {
				return false, rd.Err()
			}
			s.Runes = slices.MakeCap[rune](a, 0, int(s.totalCharacters))
		case 1:
			for len(s.Runes) < int(s.totalCharacters) {
				var u uint16
				if !rd.ReadUint16(&u) {
					return false, rd.Err()
				}
				s.Runes = slices.Append(mem.NoAlloc, s.Runes, rune(u))
			}
		case 3:
			return true, nil
		}
		s.stage++
	}
}

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
type MetadataValue struct {
	ID       uint8 // Metadata ID from header
	DataType uint8 // 0-7 corresponding to field

	Byte         byte
	Integer      int32
	String       String16
	stringReader String16Reader
}

type MetadataReader struct {
	Values []MetadataValue

	dataType   uint8 // from header
	metadataID uint8 // from header

	// Current metadataValue being read
	metadata MetadataValue

	state uint8
}

func (r *MetadataReader) Parse(e MobType) EntityMetadata {
	var m EntityMetadata
	for _, value := range r.Values {
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
				m.BlowingUp = int8(value.Byte) != -1 // -1 is false, 1 is true
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

func (m *MetadataReader) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	const (
		READING_HEADER = iota
		// STANDARD DATATYPES
		READING_BYTE
		READING_INTEGER
		READING_STRING
		// 127 value recieved
		COMPLETED
	)
	for {
		switch m.state {
		case COMPLETED:
			return true, nil
		case READING_HEADER:
			var value byte
			if !rd.ReadUint8(&value) {
				return false, rd.Err()
			}
			if value == 127 {
				m.state = COMPLETED
				return true, nil
			}
			m.dataType = value >> 5
			m.metadataID = value & 0x1F
			switch m.dataType {
			case 0:
				m.state = READING_BYTE
			case 2:
				m.state = READING_INTEGER
			case 4:
				m.state = READING_STRING
			default:
				panic("server sent unexpect metadata value.")
			}
			m.metadata = MetadataValue{
				ID:       m.metadataID,
				DataType: m.dataType,
			}
		case READING_BYTE:
			if !rd.ReadUint8(&m.metadata.Byte) {
				return false, rd.Err()
			}
			m.Values = slices.Append(a, m.Values, m.metadata)
			m.state = READING_HEADER
		case READING_INTEGER:
			if !rd.ReadInt32(&m.metadata.Integer) {
				return false, rd.Err()
			}
			m.Values = slices.Append(a, m.Values, m.metadata)
			m.state = READING_HEADER
		case READING_STRING:
			if ok, err := m.metadata.stringReader.Step(a, rd); !ok {
				return false, err
			}
			m.metadata.String = m.metadata.stringReader.Runes
			m.Values = slices.Append(a, m.Values, m.metadata)
			m.state = READING_HEADER
		}
	}
}

// https://pixelbrush.dev/beta-wiki/networking/packets/000-keep-alive
type PacketKeepAlive struct {
	// no body
	_ byte
}

func (p *PacketKeepAlive) Step(mem.Allocator, *net.BufferedReader) (bool, error) {
	return true, nil
}

func (p PacketKeepAlive) Write(w io.Writer) error { return WriteByte(w, PKT_KeepAlive) }

// https://pixelbrush.dev/beta-wiki/networking/packets/001-login
type ClientboundLogin struct {
	EntityID int32

	Unused String16
	unused String16Reader

	WorldSeed int64

	Dimension uint8

	stage int
}

func (p *ClientboundLogin) Step(_ mem.Allocator, r *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !r.ReadInt32(&p.EntityID) {
				return false, r.Err()
			}
		case 1:
			if ok, err := p.unused.Step(mem.NoAlloc, r); !ok {
				return false, err
			}
		case 2:
			if !r.ReadInt64(&p.WorldSeed) {
				return false, r.Err()
			}
		case 3:
			if !r.ReadUint8(&p.Dimension) {
				return false, r.Err()
			}
		case 4:
			return true, nil

		}
		p.stage++
	}
}

type ServerboundLogin struct {
	ProtocolVersion int32
	Username        String16
	_               int64
	__              byte
}

func (p ServerboundLogin) Write(w io.Writer) error {
	if err := WriteByte(w, PKT_Login); err != nil {
		return err
	}
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
	connectionHash String16Reader
}

func (p *ClientboundPreLogin) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	if ok, err := p.connectionHash.Step(a, rd); !ok {
		return ok, err
	}
	p.ConnectionHash = p.connectionHash.Runes
	return true, nil
}

type ServerboundPreLogin struct {
	Username String16
}

func (p ServerboundPreLogin) Write(w io.Writer) error {
	if err := WriteByte(w, PKT_PreLogin); err != nil {
		return err
	}
	return WriteString16(w, p.Username)
}

type ClientboundSetSpawnPosition struct {
	X, Y, Z int32
	stage   int
}

func (p *ClientboundSetSpawnPosition) Step(_ mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.X) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadInt32(&p.Y) {
				return false, rd.Err()
			}
		case 2:
			if !rd.ReadInt32(&p.Z) {
				return false, rd.Err()
			}
		case 3:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundSetTime struct {
	Time  int64
	stage int
}

func (p *ClientboundSetTime) Step(_ mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt64(&p.Time) {
				return false, rd.Err()
			}
		case 1:
			return true, nil
		}
		p.stage++
	}

}

type ClientboundSpawnMob struct {
	EntityID int32
	MobType  MobType
	X, Y, Z  float32

	Yaw, Pitch float32

	Metadata EntityMetadata

	rmetadata MetadataReader
	stage     int
}

func (p *ClientboundSpawnMob) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.EntityID) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadUint8(&p.MobType) {
				return false, rd.Err()
			}
		case 2:
			var coord int32
			if !rd.ReadInt32(&coord) {
				return false, rd.Err()
			}
			p.X = UnquantizeCoordinate(coord)
		case 3:
			var coord int32
			if !rd.ReadInt32(&coord) {
				return false, rd.Err()
			}
			p.Y = UnquantizeCoordinate(coord)
		case 4:
			var coord int32
			if !rd.ReadInt32(&coord) {
				return false, rd.Err()
			}
			p.Z = UnquantizeCoordinate(coord)
		case 5:
			var angle int8
			if !rd.ReadInt8(&angle) {
				return false, rd.Err()
			}
			p.Yaw = UnquantizeAngle(angle)
		case 6:
			var angle int8
			if !rd.ReadInt8(&angle) {
				return false, rd.Err()
			}
			p.Pitch = UnquantizeAngle(angle)
		case 7:
			if ok, err := p.rmetadata.Step(a, rd); !ok {
				return false, err
			}
			p.Metadata = p.rmetadata.Parse(p.MobType)
		case 8:
			return true, nil
		}
		p.stage++
	}

}

type ClientboundEntityVelocity struct {
	EntityID int32
	X, Y, Z  float32
	stage    int
}

func (p *ClientboundEntityVelocity) Step(_ mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.EntityID) {
				return false, rd.Err()
			}
		case 1:
			var vel int16
			if !rd.ReadInt16(&vel) {
				return false, rd.Err()
			}
			p.X = UnclampVelocity(vel)
		case 2:
			var vel int16
			if !rd.ReadInt16(&vel) {
				return false, rd.Err()
			}
			p.Y = UnclampVelocity(vel)
		case 3:
			var vel int16
			if !rd.ReadInt16(&vel) {
				return false, rd.Err()
			}
			p.Z = UnclampVelocity(vel)
		case 4:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundSetChunkVisibility struct {
	X, Y  int32
	Load  bool
	stage int
}

func (p *ClientboundSetChunkVisibility) Step(_ mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.X) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadInt32(&p.Y) {
				return false, rd.Err()
			}
		case 2:
			if !rd.ReadBool(&p.Load) {
				return false, rd.Err()
			}
		case 3:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundSpawnItem struct {
	EntityID   int32
	ItemID     int16
	Amount     int8
	Metadata   int16
	X, Y, Z    float32
	Yaw, Pitch float32
	Roll       float32

	stage int
}

func (p *ClientboundSpawnItem) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.EntityID) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadInt16(&p.ItemID) {
				return false, rd.Err()
			}
		case 2:
			if !rd.ReadInt8(&p.Amount) {
				return false, rd.Err()
			}
		case 3:
			if !rd.ReadInt16(&p.Metadata) {
				return false, rd.Err()
			}
		case 4:
			var c int32
			if !rd.ReadInt32(&c) {
				return false, rd.Err()
			}
			p.X = UnquantizeCoordinate(c)
		case 5:
			var c int32
			if !rd.ReadInt32(&c) {
				return false, rd.Err()
			}
			p.Y = UnquantizeCoordinate(c)
		case 6:
			var c int32
			if !rd.ReadInt32(&c) {
				return false, rd.Err()
			}
			p.Z = UnquantizeCoordinate(c)
		case 7:
			var angle int8
			if !rd.ReadInt8(&angle) {
				return false, rd.Err()
			}
			p.Yaw = UnquantizeAngle(angle)
		case 8:
			var angle int8
			if !rd.ReadInt8(&angle) {
				return false, rd.Err()
			}
			p.Pitch = UnquantizeAngle(angle)
		case 9:
			var angle int8
			if !rd.ReadInt8(&angle) {
				return false, rd.Err()
			}
			p.Roll = UnquantizeAngle(angle)
		case 10:
			return true, nil
		}
		p.stage++
	}
}

type PacketDisconnect struct {
	Reason String16
	s16r   String16Reader
	stage  int
}

func (p *PacketDisconnect) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if ok, err := p.s16r.Step(a, rd); !ok {
				return false, err
			}
			p.Reason = p.s16r.Runes
		case 1:
			return true, nil
		}
		p.stage++
	}
}
func (p *PacketDisconnect) Write(w io.Writer) error {
	if err := WriteByte(w, PKT_Disconnect); err != nil {
		return err
	}
	return WriteString16(w, p.Reason)
}

type PacketPlayerPosition struct {
	X, Y,
	CameraY,
	Z float64
	OnGround bool
	stage    int
}

func (p *PacketPlayerPosition) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadFloat64(&p.X) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadFloat64(&p.Y) {
				return false, rd.Err()
			}
		case 2:
			if !rd.ReadFloat64(&p.CameraY) {
				return false, rd.Err()
			}
		case 3:
			if !rd.ReadFloat64(&p.Z) {
				return false, rd.Err()
			}
		case 4:
			if !rd.ReadBool(&p.OnGround) {
				return false, rd.Err()
			}
		case 5:
			return true, nil
		}
		p.stage++
	}
}
func (p *PacketPlayerPosition) Write(w io.Writer) error {
	if err := WriteByte(w, PKT_PlayerPosition); err != nil {
		return err
	}
	if err := WriteFloat64(w, p.X); err != nil {
		return err
	}
	if err := WriteFloat64(w, p.Y); err != nil {
		return err
	}
	if err := WriteFloat64(w, p.CameraY); err != nil {
		return err
	}
	if err := WriteFloat64(w, p.Z); err != nil {
		return err
	}
	if err := WriteBool(w, p.OnGround); err != nil {
		return err
	}
	return nil
}

type PacketPlayerRotation struct {
	Yaw, Pitch float32
	OnGround   bool
	stage      int
}

func (p *PacketPlayerRotation) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadFloat32(&p.Yaw) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadFloat32(&p.Pitch) {
				return false, rd.Err()
			}
		case 2:
			if !rd.ReadBool(&p.OnGround) {
				return false, rd.Err()
			}
		case 3:
			return true, nil
		}
		p.stage++
	}
}
func (p *PacketPlayerRotation) Write(w io.Writer) error {
	if err := WriteByte(w, PKT_PlayerRotation); err != nil {
		return err
	}
	if err := WriteFloat32(w, p.Yaw); err != nil {
		return err
	}
	if err := WriteFloat32(w, p.Pitch); err != nil {
		return err
	}
	if err := WriteBool(w, p.OnGround); err != nil {
		return err
	}
	return nil
}

type PacketPlayerPositionAndRotation struct {
	X, Y,
	CameraY,
	Z float64
	Yaw, Pitch float32
	OnGround   bool
	stage      int
}

func (p *PacketPlayerPositionAndRotation) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadFloat64(&p.X) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadFloat64(&p.Y) {
				return false, rd.Err()
			}
		case 2:
			if !rd.ReadFloat64(&p.CameraY) {
				return false, rd.Err()
			}
		case 3:
			if !rd.ReadFloat64(&p.Z) {
				return false, rd.Err()
			}
		case 4:
			if !rd.ReadFloat32(&p.Yaw) {
				return false, rd.Err()
			}
		case 5:
			if !rd.ReadFloat32(&p.Pitch) {
				return false, rd.Err()
			}
		case 6:
			if !rd.ReadBool(&p.OnGround) {
				return false, rd.Err()
			}
		}
		p.stage++
	}
}
func (p *PacketPlayerPositionAndRotation) Write(w io.Writer) error {
	if err := WriteByte(w, PKT_PlayerPositionAndRotation); err != nil {
		return err
	}
	if err := WriteFloat64(w, p.X); err != nil {
		return err
	}
	if err := WriteFloat64(w, p.Y); err != nil {
		return err
	}
	if err := WriteFloat64(w, p.CameraY); err != nil {
		return err
	}
	if err := WriteFloat64(w, p.Z); err != nil {
		return err
	}
	if err := WriteFloat32(w, p.Yaw); err != nil {
		return err
	}
	if err := WriteFloat32(w, p.Pitch); err != nil {
		return err
	}
	if err := WriteBool(w, p.OnGround); err != nil {
		return err
	}
	return nil
}

type ItemStack struct {
	ID       ItemID
	Amount   int8
	Metadata int16
	stage    int
}

func (p *ItemStack) Step(_ mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt16(&p.ID) {
				return false, rd.Err()
			}
		case 1:
			if p.ID != ITEM_INVALID {
				if !rd.ReadInt8(&p.Amount) {
					return false, rd.Err()
				}
			}
		case 2:
			if p.ID != ITEM_INVALID {
				if !rd.ReadInt16(&p.Metadata) {
					return false, rd.Err()
				}
			}
		case 3:
			return true, nil
		}
		p.stage++
	}
}

// Used to set inventory values.
type ClientboundFillContainer struct {
	WindowID int8 // The incremental ID of the window. Ranges from 0 to 99

	totalSlots int16 // Number of slots in the inventory
	Slots      []ItemStack

	currentItem ItemStack // current item being read.

	stage int
}

func (p *ClientboundFillContainer) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt8(&p.WindowID) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadInt16(&p.totalSlots) {
				return false, rd.Err()
			}
			p.Slots = slices.MakeCap[ItemStack](a, 0, int(p.totalSlots))
		case 2:
			for int(p.totalSlots) > len(p.Slots) {
				ok, err := p.currentItem.Step(a, rd)
				if !ok {
					return false, err
				}
				p.Slots = slices.Append(mem.NoAlloc, p.Slots, p.currentItem)
				p.currentItem.stage = 0
			}
		case 3:
			return true, nil
		}
		p.stage++
	}
}

// If both the window ID and slot ID are -1, then the item that's currently held by the mouse is affected.
// If window ID is 0, then inventory is affected.
type ClientboundSetSlot struct {
	WindowID int8
	Slot     int16
	Item     ItemStack
	stage    int
}

func (p *ClientboundSetSlot) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt8(&p.WindowID) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadInt16(&p.Slot) {
				return false, rd.Err()
			}
		case 2:
			if ok, err := p.Item.Step(a, rd); !ok {
				return false, err
			}
		case 3:
			return true, nil
		}
		p.stage++
	}
}

// This is sent by the server to control precipitation or to notify that their bed is missing or obstructed. The game state is 0 for an invalid bed, 1 to start raining, and 2 to stop raining.
type ClientboundGameEvent struct {
	Type  int8
	stage int
}

func (p *ClientboundGameEvent) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt8(&p.Type) {
				return false, rd.Err()
			}
		case 1:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundEntityPosition struct {
	EntityID int32
	X, Y, Z  float32 // relative to last PlayerPostion sent by server
	nested   bool
	stage    int
}

func (p *ClientboundEntityPosition) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !p.nested {
				if !rd.ReadInt32(&p.EntityID) {
					return false, rd.Err()
				}
			}
		case 1:
			var coord int8
			if !rd.ReadInt8(&coord) {
				return false, rd.Err()
			}
			p.X = UnquantizeCoordinate(int32(coord))
		case 2:
			var coord int8
			if !rd.ReadInt8(&coord) {
				return false, rd.Err()
			}
			p.Y = UnquantizeCoordinate(int32(coord))
		case 3:
			var coord int8
			if !rd.ReadInt8(&coord) {
				return false, rd.Err()
			}
			p.Z = UnquantizeCoordinate(int32(coord))
		case 4:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundEntityRotation struct {
	EntityID int32
	Yaw      float32
	Pitch    float32
	nested   bool
	stage    int
}

func (p *ClientboundEntityRotation) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !p.nested {
				if !rd.ReadInt32(&p.EntityID) {
					return false, rd.Err()
				}
			}
		case 1:
			var angle int8
			if !rd.ReadInt8(&angle) {
				return false, rd.Err()
			}
			p.Yaw = UnquantizeAngle(angle)
		case 2:
			var angle int8
			if !rd.ReadInt8(&angle) {
				return false, rd.Err()
			}
			p.Pitch = UnquantizeAngle(angle)
		case 3:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundEntityPositionAndRotation struct {
	EntityID int32
	Pos      ClientboundEntityPosition
	Rot      ClientboundEntityRotation
	stage    int
}

func (p *ClientboundEntityPositionAndRotation) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	p.Pos.nested = true
	p.Rot.nested = true
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.EntityID) {
				return false, rd.Err()
			}
		case 1:
			if ok, err := p.Pos.Step(a, rd); !ok {
				return false, err
			}
		case 2:
			if ok, err := p.Rot.Step(a, rd); !ok {
				return false, err
			}
		case 3:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundDespawnEntity struct {
	EntityID int32
	stage    int
}

func (p *ClientboundDespawnEntity) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.EntityID) {
				return false, rd.Err()
			}
		case 1:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundEntityEvent struct {
	EntityID int32
	Action   int8
	stage    int
}

func (p *ClientboundEntityEvent) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.EntityID) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadInt8(&p.Action) {
				return false, rd.Err()
			}
		case 2:
			return true, nil
		}
		p.stage++
	}
}

type PacketPlayerMovment struct {
	OnGround bool
	stage    int
}

func (p *PacketPlayerMovment) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadBool(&p.OnGround) {
				return false, rd.Err()
			}
		case 1:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundTeleportEntity struct {
	EntityID   int32
	X, Y, Z    float32
	Yaw, Pitch float32
	stage      int
}

func (p *ClientboundTeleportEntity) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.EntityID) {
				return false, rd.Err()
			}
		case 1:
			var coord int32
			if !rd.ReadInt32(&coord) {
				return false, rd.Err()
			}
			p.X = UnquantizeCoordinate(coord)
		case 2:
			var coord int32
			if !rd.ReadInt32(&coord) {
				return false, rd.Err()
			}
			p.Y = UnquantizeCoordinate(coord)
		case 3:
			var coord int32
			if !rd.ReadInt32(&coord) {
				return false, rd.Err()
			}
			p.Z = UnquantizeCoordinate(coord)
		case 4:
			var angle int8
			if !rd.ReadInt8(&angle) {
				return false, rd.Err()
			}
			p.Yaw = UnquantizeAngle(angle)
		case 5:
			var angle int8
			if !rd.ReadInt8(&angle) {
				return false, rd.Err()
			}
			p.Pitch = UnquantizeAngle(angle)
		case 6:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundSetBlock struct {
	X        int32
	Y        int8
	Z        int32
	Type     BlockID
	Metadata int8

	stage int
}

func (p *ClientboundSetBlock) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.X) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadInt8(&p.Y) {
				return false, rd.Err()
			}
		case 2:
			if !rd.ReadInt32(&p.Z) {
				return false, rd.Err()
			}
		case 3:
			if !rd.ReadUint8(&p.Type) {
				return false, rd.Err()
			}
		case 4:
			if !rd.ReadInt8(&p.Metadata) {
				return false, rd.Err()
			}
		case 5:
			return true, nil
		}
		p.stage++
	}
}

type PackedCoordinate int16

func (p PackedCoordinate) Unpack(x, y, z *int8) {
	*x = int8((p >> 12) & 0x0F)
	*z = int8((p >> 8) & 0x0F)
	*y = int8((p) & 0x0F)
}

type ClientboundSetMultipleBlocks struct {
	X, Z        int32 // chunk position
	TotalBlocks int

	BlockPosition []PackedCoordinate
	Block         []BlockID
	Metadata      []int8
	stage         int
}

func (p *ClientboundSetMultipleBlocks) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.X) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadInt32(&p.Z) {
				return false, rd.Err()
			}
		case 2:
			var l int16
			if !rd.ReadInt16(&l) {
				return false, rd.Err()
			}
			p.TotalBlocks = int(l)

			p.Block = slices.MakeCap[BlockID](a, 0, p.TotalBlocks)
			p.BlockPosition = slices.MakeCap[PackedCoordinate](a, 0, p.TotalBlocks)
			p.Metadata = slices.MakeCap[int8](a, 0, p.TotalBlocks)
		case 3:
			for len(p.BlockPosition) < p.TotalBlocks {
				var pos int16
				if !rd.ReadInt16(&pos) {
					return false, rd.Err()
				}
				p.BlockPosition = append(p.BlockPosition, PackedCoordinate(pos))
			}
		case 4:
			for len(p.Block) < p.TotalBlocks {
				var id BlockID
				if !rd.ReadUint8(&id) {
					return false, rd.Err()
				}
				p.Block = append(p.Block, id)
			}
		case 5:
			for len(p.Metadata) < p.TotalBlocks {
				var metadata int8
				if !rd.ReadInt8(&metadata) {
					return false, rd.Err()
				}
				p.Metadata = append(p.Metadata, metadata)
			}
		case 6:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundEntityMetadata struct {
	EntityID int32
	Metadata MetadataReader // NOTE: call .Parse()
	stage    int
}

func (p *ClientboundEntityMetadata) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.EntityID) {
				return false, rd.Err()
			}
		case 1:
			if ok, err := p.Metadata.Step(a, rd); !ok {
				return false, err
			}
		case 2:
			return true, nil
		}
		p.stage++
	}
}

// The X,Y,Z coordinates are in world block space.
type ClientboundChunk struct {
	X int32
	Y int16
	Z int32

	Width, Height, Length int8

	compressedSize int32
	CompressedData []byte // Zlib compressed data.

	stage int
}

const CHUNK_SIZE_XZ = 16 // chunk width
const CHUNK_SIZE_Y = 128 // chunk height
const CHUNK_SIZE = CHUNK_SIZE_XZ * CHUNK_SIZE_XZ * CHUNK_SIZE_Y

// A chunk containing blocks.
// Different from the Chunk packet which contains compressed data.
type DecompressedChunkData struct {
	X, Z       int
	Blocks     [CHUNK_SIZE]BlockID
	Metadata   [CHUNK_SIZE]uint8
	BlockLight [CHUNK_SIZE]uint8
	SkyLight   [CHUNK_SIZE]uint8
}

func (c *DecompressedChunkData) IsAir(x, y, z int) bool {
	if y < 0 || y >= CHUNK_SIZE_Y {
		return true
	}
	return c.Blocks[ChunkIndex(x, y, z)] == BLOCK_Air
}

// Local chunk coordinate to index.
func ChunkIndex(x, y, z int) int { return (y << 8) | ((z & 15) << 4) | (x & 15) }

// each block contains 1 byte block id 1.5 bytes (3 nibbles) lighting and metadata.
var chunkDataDecompressBuffer [CHUNK_SIZE +
	// 3 nibbles
	3*(CHUNK_SIZE/2)]uint8

func readNibble(data []byte, i int) uint8 {
	b := data[i>>1]
	if i&1 == 0 {
		return b & 0x0F
	}
	return b >> 4
}

// Process clientbound chunk data.
func (d *DecompressedChunkData) ProcessChunkData(c *ClientboundChunk) error {
	if c == nil {
		return nil
	}
	// Decompress chunk data
	n, err := zlib.DecompressData(c.CompressedData, chunkDataDecompressBuffer[:])
	if err != nil {
		return err
	}
	uncompressed := chunkDataDecompressBuffer[:n]

	// Reference: https://github.com/OfficialPixelBrush/BetrockPlusPlus/blob/7e18479c055a9d7b43871d00b7026944053b2faf/src/bpp_server/chunk_IO/chunk_serializer.h#L15
	width := int(c.Width) + 1
	height := int(c.Height) + 1
	length := int(c.Length) + 1

	blocks := width * height * length
	nibbles := (blocks + 1) / 2

	blockData := uncompressed[:blocks]
	metaData := uncompressed[blocks : blocks+nibbles]
	blockLight := uncompressed[blocks+nibbles : blocks+2*nibbles]
	skyLight := uncompressed[blocks+2*nibbles : blocks+3*nibbles]

	// WIKI: https://pixelbrush.dev/beta-wiki/worlds/chunk#block-ordering
	// Blocks are stored as vertical columns (Y-Axis).
	// We iterate up to width/length/height to safely support partial chunk updates.
	var i int
	for x := range width {
		for z := range length {
			for y := range height {
				localX := int(c.X) + x
				localZ := int(c.Z) + z
				localY := int(c.Y) + y

				idx := ChunkIndex(localX, localY, localZ)

				// copy over the data.
				d.Blocks[idx] = BlockID(blockData[i])
				d.Metadata[idx] = readNibble(metaData, i)
				d.BlockLight[idx] = readNibble(blockLight, i)
				d.SkyLight[idx] = readNibble(skyLight, i)

				i++
			}
		}
	}

	return nil
}

func (p *ClientboundChunk) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.X) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadInt16(&p.Y) {
				return false, rd.Err()
			}
		case 2:
			if !rd.ReadInt32(&p.Z) {
				return false, rd.Err()
			}
		case 3:
			if !rd.ReadInt8(&p.Width) {
				return false, rd.Err()
			}
		case 4:
			if !rd.ReadInt8(&p.Height) {
				return false, rd.Err()
			}
		case 5:
			if !rd.ReadInt8(&p.Length) {
				return false, rd.Err()
			}
		case 6:
			if !rd.ReadInt32(&p.compressedSize) {
				return false, rd.Err()
			}
			p.CompressedData = slices.MakeCap[byte](a, 0, int(p.compressedSize))
		case 7:
			for len(p.CompressedData) < int(p.compressedSize) {
				var b byte
				if !rd.ReadUint8(&b) {
					return false, rd.Err()
				}
				p.CompressedData = append(p.CompressedData, b)
			}
		case 8:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundIncrementStatistic struct {
	StatisticID int32
	Amount      int8
	stage       int
}

func (p *ClientboundIncrementStatistic) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.StatisticID) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadInt8(&p.Amount) {
				return false, rd.Err()
			}
		case 2:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundSpawnPlayer struct {
	EntityID   int32
	Username   String16
	username   String16Reader
	X, Y, Z    int32
	Yaw, Pitch float32
	HeldItem   ItemID

	stage int
}

func (p *ClientboundSpawnPlayer) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.EntityID) {
				return false, rd.Err()
			}
		case 1:
			if ok, err := p.username.Step(a, rd); !ok {
				return false, err
			}
			p.Username = p.username.Runes
		case 2:
			if !rd.ReadInt32(&p.X) {
				return false, rd.Err()
			}
		case 3:
			if !rd.ReadInt32(&p.Y) {
				return false, rd.Err()
			}
		case 4:
			if !rd.ReadInt32(&p.Z) {
				return false, rd.Err()
			}
		case 5:
			var angle int8
			if !rd.ReadInt8(&angle) {
				return false, rd.Err()
			}
			p.Yaw = UnquantizeAngle(angle)
		case 6:
			var angle int8
			if !rd.ReadInt8(&angle) {
				return false, rd.Err()
			}
			p.Pitch = UnquantizeAngle(angle)
		case 7:
			if !rd.ReadInt16(&p.HeldItem) {
				return false, rd.Err()
			}
		case 8:
			return true, nil
		}
		p.stage++
	}
}

type PacketChatMessage struct {
	msg     String16Reader
	Message String16
	stage   int
}

func (p *PacketChatMessage) Write(w io.Writer) error {
	if err := WriteByte(w, PKT_ChatMessage); err != nil {
		return err
	}
	if err := WriteString16(w, p.Message); err != nil {
		return err
	}
	return nil
}

func (p *PacketChatMessage) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if ok, err := p.msg.Step(a, rd); !ok {
				return false, err
			}
			p.Message = p.msg.Runes
		case 1:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundSetEquipment struct {
	EntityID      int32
	InventorySlot int16
	ItemID        ItemID
	Metadata      int16

	stage int
}

func (p *ClientboundSetEquipment) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.EntityID) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadInt16(&p.InventorySlot) {
				return false, rd.Err()
			}
		case 2:
			if !rd.ReadInt16(&p.ItemID) {
				return false, rd.Err()
			}
		case 3:
			if !rd.ReadInt16(&p.Metadata) {
				return false, rd.Err()
			}
		case 4:
			return true, nil
		}
		p.stage++
	}
}

type PacketAnimation struct {
	PlayerID  int32
	Animation int8
	stage     int
}

func (p *PacketAnimation) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.PlayerID) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadInt8(&p.Animation) {
				return false, rd.Err()
			}
		case 2:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundWorldEvent struct {
	EffectID int32
	X, Y, Z  int32
	Data     int32
	stage    int
}

func (p *ClientboundWorldEvent) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.EffectID) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadInt32(&p.X) {
				return false, rd.Err()
			}
		case 2:
			var y int8
			if !rd.ReadInt8(&y) {
				return false, rd.Err()
			}
			p.Y = int32(y)
		case 3:
			if !rd.ReadInt32(&p.Z) {
				return false, rd.Err()
			}
		case 4:
			if !rd.ReadInt32(&p.Data) {
				return false, rd.Err()
			}
		case 5:
			return true, nil
		}
		p.stage++
	}
}

type ClientboundCollectItem struct {
	EntityID, Collector int32
	stage               int
}

func (p *ClientboundCollectItem) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	for {
		switch p.stage {
		case 0:
			if !rd.ReadInt32(&p.EntityID) {
				return false, rd.Err()
			}
		case 1:
			if !rd.ReadInt32(&p.Collector) {
				return false, rd.Err()
			}
		case 2:
			return true, nil
		}
		p.stage++
	}
}

// Returns a decoder for the given packet id. It is the user's job to free the decoder.
// Returns nil if packetID is invalid.
func NewDecoder(a mem.Allocator, packetID PacketID) Decoder {
	switch packetID {
	// Bare minimum packets needed to join the game: 26
	case PKT_PreLogin:
		return mem.Alloc[ClientboundPreLogin](a)
	case PKT_Login:
		return mem.Alloc[ClientboundLogin](a)
	case PKT_SetSpawnPosition:
		return mem.Alloc[ClientboundSetSpawnPosition](a)
	case PKT_SetTime:
		return mem.Alloc[ClientboundSetTime](a)
	case PKT_SpawnMob:
		return mem.Alloc[ClientboundSpawnMob](a)
	case PKT_EntityVelocity:
		return mem.Alloc[ClientboundEntityVelocity](a)
	case PKT_SetChunkVisibility:
		return mem.Alloc[ClientboundSetChunkVisibility](a)
	case PKT_SpawnItem:
		return mem.Alloc[ClientboundSpawnItem](a)
	case PKT_PlayerPosition:
		return mem.Alloc[PacketPlayerPosition](a)
	case PKT_PlayerRotation:
		return mem.Alloc[PacketPlayerRotation](a)
	case PKT_PlayerPositionAndRotation:
		return mem.Alloc[PacketPlayerPositionAndRotation](a)
	case PKT_Disconnect:
		return mem.Alloc[PacketDisconnect](a)
	case PKT_FillContainer:
		return mem.Alloc[ClientboundFillContainer](a)
	case PKT_SetSlot:
		return mem.Alloc[ClientboundSetSlot](a)
	case PKT_GameEvent:
		return mem.Alloc[ClientboundGameEvent](a)
	case PKT_EntityPosition:
		return mem.Alloc[ClientboundEntityPosition](a)
	case PKT_EntityRotation:
		return mem.Alloc[ClientboundEntityRotation](a)
	case PKT_EntityPositionAndRotation:
		return mem.Alloc[ClientboundEntityPositionAndRotation](a)
	case PKT_DespawnEntity:
		return mem.Alloc[ClientboundDespawnEntity](a)
	case PKT_EntityEvent:
		return mem.Alloc[ClientboundEntityEvent](a)
	case PKT_PlayerMovement:
		return mem.Alloc[PacketPlayerMovment](a)
	case PKT_TeleportEntity:
		return mem.Alloc[ClientboundTeleportEntity](a)
	case PKT_SetBlock:
		return mem.Alloc[ClientboundSetBlock](a)
	case PKT_SetMultipleBlocks:
		return mem.Alloc[ClientboundSetMultipleBlocks](a)
	case PKT_EntityMetadata:
		return mem.Alloc[ClientboundEntityMetadata](a)
	case PKT_Chunk:
		return mem.Alloc[ClientboundChunk](a)
	case PKT_IncrementStatistic:
		return mem.Alloc[ClientboundIncrementStatistic](a)
	case PKT_SpawnPlayer:
		return mem.Alloc[ClientboundSpawnPlayer](a)
	case PKT_ChatMessage:
		return mem.Alloc[PacketChatMessage](a)
	case PKT_SetEquipment:
		return mem.Alloc[ClientboundSetEquipment](a)
		// end of bare minimum packets
	case PKT_Animation:
		return mem.Alloc[PacketAnimation](a)
	case PKT_WorldEvent:
		return mem.Alloc[ClientboundWorldEvent](a)
	case PKT_CollectItem:
		return mem.Alloc[ClientboundCollectItem](a)

	}
	return nil
}
