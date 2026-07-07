package mc

import (
	"mbc/net"

	"solod.dev/so/errors"
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

type Decoder interface {
	Step(a mem.Allocator, rd *net.BufferedReader) (bool, error)
}
type Encoder interface {
	Write(io.Writer) error
}

type String16 = []rune

type String8Reader struct {
	len   uint16
	buf   []byte
	stage uint8
}

func (s *String8Reader) Reset()        { *s = String8Reader{} }
func (s String8Reader) String() string { return string(s.buf) }

func (s *String8Reader) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	if s.stage == 0 {
		if !rd.ReadUint16(&s.len) {
			return false, rd.Err()
		}
		s.buf = slices.Make[byte](a, int(s.len))
		s.stage++
	}
	if s.stage == 1 {
		if !rd.ReadFull(s.buf) {
			return false, rd.Err()
		}
		s.stage++
	}
	if s.stage == 2 {
		return true, rd.Err()
	}
	return false, rd.Err()
}

type String16Reader struct {
	len uint16

	Runes []rune
	n     int
	stage uint8
}

func (s *String16Reader) Reset() { *s = String16Reader{} }

func (s *String16Reader) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	if s.stage == 0 {
		if !rd.ReadUint16(&s.len) {
			return false, rd.Err()
		}
		s.Runes = slices.MakeCap[rune](a, 0, int(s.len))
		s.stage++
	}
	if s.stage == 1 {
		for s.n < int(s.len) {
			var u uint16
			if !rd.ReadUint16(&u) {
				return false, rd.Err()
			}
			s.Runes = slices.Append(mem.NoAlloc, s.Runes, rune(u))
			s.n += 1
		}
		s.stage++
	}
	if s.stage == 2 {
		return true, rd.Err()
	}
	return false, rd.Err()
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
	values []MetadataValue

	dataType   uint8 // from header
	metadataID uint8 // from header

	// Current metadataValue being read
	metadata MetadataValue

	state uint8
}

func (r *MetadataReader) Parse(e MobType) EntityMetadata {
	var m EntityMetadata
	for _, value := range r.values {
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

var IncompleteMetadataErr = errors.New("Tried parsing incomplete metadata")

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
			m.values = slices.Append(a, m.values, m.metadata)
			m.state = READING_HEADER
		case READING_INTEGER:
			if !rd.ReadInt32(&m.metadata.Integer) {
				return false, rd.Err()
			}
			m.values = slices.Append(a, m.values, m.metadata)
			m.state = READING_HEADER
		case READING_STRING:
			if ok, err := m.metadata.stringReader.Step(a, rd); !ok {
				return false, err
			}
			m.metadata.String = m.metadata.stringReader.Runes
			m.values = slices.Append(a, m.values, m.metadata)
			m.state = READING_HEADER
		}
	}
}

// https://pixelbrush.dev/beta-wiki/networking/packets/000-keep-alive
type PacketKeepAlive struct {
	// no body
	_ byte
}

// Read implements [ClientBoundPacket].
func (p *PacketKeepAlive) Step(mem.Allocator, *net.BufferedReader) (bool, error) {
	return true, nil
}

// Write implements [ServerBoundPacket].
func (p PacketKeepAlive) Write(io.Writer) error {
	return nil
}

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
	if p.stage == 0 {
		if !r.ReadInt32(&p.EntityID) {
			return false, r.Err()
		}
		p.stage++
	}
	if p.stage == 1 {
		if ok, err := p.unused.Step(mem.NoAlloc, r); !ok {
			return false, err
		}
		p.stage++
	}
	if p.stage == 2 {
		if !r.ReadInt64(&p.WorldSeed) {
			return false, r.Err()
		}
		p.stage++
	}
	if p.stage == 3 {
		if !r.ReadUint8(&p.Dimension) {
			return false, r.Err()
		}
		p.stage++
	}
	if p.stage == 4 {
		return true, nil
	}
	return false, nil
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
	X, Y, Z  int32

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
			p.Pitch = UnquantizeAngle(angle)
		case 8:
			var angle int8
			if !rd.ReadInt8(&angle) {
				return false, rd.Err()
			}
			p.Yaw = UnquantizeAngle(angle)
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
	nested   bool // nested inside another packet
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
			if !p.nested {
				if !rd.ReadBool(&p.OnGround) {
					return false, rd.Err()
				}
			}
		case 5:
			return true, nil
		}
		p.stage++
	}
}
func (p *PacketPlayerPosition) Write(w io.Writer) error {
	if !p.nested {
		if err := WriteByte(w, PKT_PlayerPosition); err != nil {
			return err
		}
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
	nested     bool //packet is nested inside another packet
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
	if !p.nested {
		if err := WriteByte(w, PKT_PlayerRotation); err != nil {
			return err
		}
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
	Pos   PacketPlayerPosition
	Rot   PacketPlayerRotation
	stage int
}

func (p *PacketPlayerPositionAndRotation) Step(a mem.Allocator, rd *net.BufferedReader) (bool, error) {
	p.Pos.nested = true
	p.Rot.nested = true
	for {
		switch p.stage {
		case 0:
			if ok, err := p.Pos.Step(a, rd); !ok {
				return false, err
			}
		case 1:
			if ok, err := p.Rot.Step(a, rd); !ok {
				return false, err
			}
		case 3:
			return true, nil
		}
		p.stage++
	}
}
func (p *PacketPlayerPositionAndRotation) Write(w io.Writer) error {
	p.Pos.nested = true // stop the packets from writing their own id
	p.Rot.nested = true
	if err := WriteByte(w, PKT_PlayerPositionAndRotation); err != nil {
		return err
	}
	if err := p.Pos.Write(w); err != nil {
		return err
	}
	if err := p.Rot.Write(w); err != nil {
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
			println("ItemID=", p.Item.ID)
		case 3:
			return true, nil
		}
		p.stage++
	}
}

// Returns a decoder for the given packet id. It is the user's job to free the decoder.
// Returns nil if packetID is invalid.
func NewDecoder(a mem.Allocator, packetID PacketID) Decoder {
	switch packetID {
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
	}
	return nil
}
