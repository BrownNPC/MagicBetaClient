package mc

import (
	"mbc/net"

	"solod.dev/so/encoding/binary"
	"solod.dev/so/io"
	"solod.dev/so/math"
	"solod.dev/so/mem"
)

type Decoder interface {
	Step(a mem.Allocator, rd io.Reader) (bool, error)
}
type Encoder interface {
	Write(io.Writer) error
}

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

	ucs2Reader net.SteppedReader16

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

	Unused String16
	unused String16Reader // unused string16

	WorldSeed int64
	worldSeed net.SteppedReader64

	Dimension uint8
	dimension net.SteppedReader

	step int
}

func (p *ClientboundLogin) Step(r io.Reader) (bool, error) {
	switch p.step {
	case 0:
		if ok, err := p.entityID.Step(r); !ok {
			return false, err
		}
		p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
		p.step = 1 //step
	case 1:
		if ok, err := p.unused.Step(mem.NoAlloc, r); !ok {
			return false, err
		}
		p.Unused = p.unused.Runes
		p.step++ //step
	case 2:
		if ok, err := p.worldSeed.Step(r); !ok {
			return false, err
		}
		p.WorldSeed = int64(binary.BigEndian.Uint64(p.worldSeed.Buf[:]))
		p.step++ //step
	case 3:
		if ok, err := p.dimension.Step(r); !ok {
			return false, err
		}
		p.Dimension = p.dimension.Buf[0]
		p.step++ //step
	case 4:
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

func (p *ClientboundPreLogin) Step(a mem.Allocator, rd io.Reader) (bool, error) {
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
	return WriteString16(w, p.Username)
}

type PacketChatMessage struct {
	Message String16
	message String16Reader
}

func (p *PacketChatMessage) Write(w io.Writer) error {
	return WriteString16(w, p.Message)
}
func (p *PacketChatMessage) Step(a mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.message.Step(a, rd); !ok {
		return false, err
	}
	p.Message = p.message.Runes
	return true, nil
}

type ClientboundSetTime struct {
	Time int64
	time net.SteppedReader64
}

func (p *ClientboundSetTime) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.time.Step(rd); !ok {
		return false, err
	}
	p.Time = int64(binary.BigEndian.Uint64(p.time.Buf[:]))
	return true, nil
}

type ClientboundSetEquipment struct {
	EntityID int32
	entityID net.SteppedReader32

	InventorySlot int16
	inventorySlot net.SteppedReader16

	ItemID int16
	itemID net.SteppedReader16

	ItemMetadata int16
	itemMetadata net.SteppedReader16

	step int
}

func (p *ClientboundSetEquipment) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	switch p.step {
	case 0:
		if ok, err := p.entityID.Step(rd); !ok {
			return false, err
		}
		p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
		p.step++

	case 1:
		if ok, err := p.inventorySlot.Step(rd); !ok {
			return false, err
		}
		p.InventorySlot = int16(binary.BigEndian.Uint16(p.inventorySlot.Buf[:]))
		p.step++

	case 2:
		if ok, err := p.itemID.Step(rd); !ok {
			return false, err
		}
		p.ItemID = int16(binary.BigEndian.Uint16(p.itemID.Buf[:]))
		p.step++

	case 3:
		if ok, err := p.itemMetadata.Step(rd); !ok {
			return false, err
		}
		p.ItemMetadata = int16(binary.BigEndian.Uint16(p.itemMetadata.Buf[:]))
		p.step++
	}

	return true, nil
}

type ClientboundSetSpawnPosition struct {
	X, Y, Z int32
	x, y, z net.SteppedReader32
}

func (p *ClientboundSetSpawnPosition) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.x.Step(rd); !ok {
		return false, err
	}
	p.X = int32(binary.BigEndian.Uint32(p.x.Buf[:]))
	if ok, err := p.y.Step(rd); !ok {
		return false, err
	}
	p.Y = int32(binary.BigEndian.Uint32(p.y.Buf[:]))
	if ok, err := p.z.Step(rd); !ok {
		return false, err
	}
	p.Z = int32(binary.BigEndian.Uint32(p.z.Buf[:]))
	return true, nil
}

type ServerboundInteractWithEntity struct {
	PlayerID, EntityID int32
	Attack             bool
}

type ClientboundSetHealth struct {
	Health int16
	health net.SteppedReader16
}

func (p *ClientboundSetHealth) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.health.Step(rd); !ok {
		return false, err
	}
	p.Health = int16(binary.BigEndian.Uint16(p.health.Buf[:]))
	return true, nil
}

type PacketRespawn struct {
	World int8
	world net.SteppedReader
}

func (p *PacketRespawn) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.world.Step(rd); !ok {
		return false, err
	}
	p.World = int8(p.world.Buf[0])
	return true, nil
}

type PacketPlayerMovement struct {
	OnGround bool
	onGround net.SteppedReader
}

func (p *PacketPlayerMovement) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.onGround.Step(rd); !ok {
		return false, err
	}
	p.OnGround = p.onGround.Buf[0] != 0
	return true, nil
}

type PacketPlayerPosition struct {
	X, Y, CameraY, Z float64
	x, y, cameraY, z net.SteppedReader64
	OnGround         bool
	onGround         net.SteppedReader
}

func (p *PacketPlayerPosition) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.x.Step(rd); !ok {
		return false, err
	}
	p.X = math.Float64frombits(binary.BigEndian.Uint64(p.x.Buf[:]))
	if ok, err := p.y.Step(rd); !ok {
		return false, err
	}
	p.Y = math.Float64frombits(binary.BigEndian.Uint64(p.y.Buf[:]))
	if ok, err := p.cameraY.Step(rd); !ok {
		return false, err
	}
	p.CameraY = math.Float64frombits(binary.BigEndian.Uint64(p.cameraY.Buf[:]))
	if ok, err := p.z.Step(rd); !ok {
		return false, err
	}
	p.Z = math.Float64frombits(binary.BigEndian.Uint64(p.z.Buf[:]))
	if ok, err := p.onGround.Step(rd); !ok {
		return false, err
	}
	p.OnGround = p.onGround.Buf[0] != 0
	return true, nil
}

type PacketPlayerRotation struct {
	Yaw, Pitch float32
	yaw, pitch net.SteppedReader32
	OnGround   bool
	onGround   net.SteppedReader
}

func (p *PacketPlayerRotation) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.yaw.Step(rd); !ok {
		return false, err
	}
	p.Yaw = math.Float32frombits(binary.BigEndian.Uint32(p.yaw.Buf[:]))
	if ok, err := p.pitch.Step(rd); !ok {
		return false, err
	}
	p.Pitch = math.Float32frombits(binary.BigEndian.Uint32(p.pitch.Buf[:]))
	if ok, err := p.onGround.Step(rd); !ok {
		return false, err
	}
	p.OnGround = p.onGround.Buf[0] != 0
	return true, nil
}

type PacketPlayerPositionAndRotation struct {
	Position PacketPlayerPosition
	Rotation PacketPlayerRotation
}

func (p *PacketPlayerPositionAndRotation) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.Position.Step(nil, rd); !ok {
		return false, err
	}
	if ok, err := p.Rotation.Step(nil, rd); !ok {
		return false, err
	}
	return true, nil
}

type ServerboundMineBlock struct {
	Status byte
	X      int32
	Y      byte
	Z      int32
	Face   byte
}

func (p ServerboundMineBlock) Write(w io.Writer) error {
	if err := WriteByte(w, p.Status); err != nil {
		return err
	}
	if err := WriteInteger(w, p.X); err != nil {
		return err
	}
	if err := WriteByte(w, p.Y); err != nil {
		return err
	}
	if err := WriteInteger(w, p.Z); err != nil {
		return err
	}
	if err := WriteByte(w, p.Face); err != nil {
		return err
	}
	return nil
}

type ServerboundPlaceBlock struct {
	X           int32
	Y           byte
	Z           int32
	Face        byte
	BlockItemID int16
	Amount      byte
	Metadata    int16
}

func (p ServerboundPlaceBlock) Write(w io.Writer) error {
	if err := WriteInteger(w, p.X); err != nil {
		return err
	}
	if err := WriteByte(w, p.Y); err != nil {
		return err
	}
	if err := WriteInteger(w, p.Z); err != nil {
		return err
	}
	if err := WriteByte(w, p.Face); err != nil {
		return err
	}
	if err := WriteShort(w, p.BlockItemID); err != nil {
		return err
	}
	if err := WriteByte(w, p.Amount); err != nil {
		return err
	}
	if err := WriteShort(w, p.Metadata); err != nil {
		return err
	}
	return nil
}

type ServerboundSetHotbarSlot struct {
	Slot int16
}

func (p ServerboundSetHotbarSlot) Write(w io.Writer) error {
	return WriteShort(w, p.Slot)
}

// Serverbound: Interact With Entity (0x07)
func (p ServerboundInteractWithEntity) Write(w io.Writer) error {
	if err := WriteInteger(w, p.PlayerID); err != nil {
		return err
	}
	if err := WriteInteger(w, p.EntityID); err != nil {
		return err
	}
	if err := WriteBool(w, p.Attack); err != nil {
		return err
	}
	return nil
}

// Clientbound: Interact With Block / Sleep packet (0x11)
type ClientboundInteractWithBlock struct {
	EntityID int32
	Type     byte
	X, Y, Z  int32

	entityID net.SteppedReader32
	_type    net.SteppedReader
	x        net.SteppedReader32
	y        net.SteppedReader32
	z        net.SteppedReader32
}

func (p *ClientboundInteractWithBlock) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.entityID.Step(rd); !ok {
		return false, err
	}
	p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
	if ok, err := p._type.Step(rd); !ok {
		return false, err
	}
	p.Type = p._type.Buf[0]
	if ok, err := p.x.Step(rd); !ok {
		return false, err
	}
	p.X = int32(binary.BigEndian.Uint32(p.x.Buf[:]))
	if ok, err := p.y.Step(rd); !ok {
		return false, err
	}
	p.Y = int32(binary.BigEndian.Uint32(p.y.Buf[:]))
	if ok, err := p.z.Step(rd); !ok {
		return false, err
	}
	p.Z = int32(binary.BigEndian.Uint32(p.z.Buf[:]))
	return true, nil
}

// Both: Animation (0x12)
type PacketAnimation struct {
	PlayerID  int32
	Animation byte

	playerID  net.SteppedReader32
	animation net.SteppedReader
}

func (p *PacketAnimation) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.playerID.Step(rd); !ok {
		return false, err
	}
	p.PlayerID = int32(binary.BigEndian.Uint32(p.playerID.Buf[:]))
	if ok, err := p.animation.Step(rd); !ok {
		return false, err
	}
	p.Animation = p.animation.Buf[0]
	return true, nil
}

func (p PacketAnimation) Write(w io.Writer) error {
	if err := WriteInteger(w, p.PlayerID); err != nil {
		return err
	}
	if err := WriteByte(w, p.Animation); err != nil {
		return err
	}
	return nil
}

// Clientbound: Spawn Item (0x15)
type ClientboundSpawnItem struct {
	EntityID int32
	ItemID   int16
	Amount   byte
	Meta     int16
	X, Y, Z  int32
	Yaw      byte
	Pitch    byte
	Roll     byte

	entityID net.SteppedReader32
	itemID   net.SteppedReader16
	amount   net.SteppedReader
	meta     net.SteppedReader16
	x        net.SteppedReader32
	y        net.SteppedReader32
	z        net.SteppedReader32
	yaw      net.SteppedReader
	pitch    net.SteppedReader
	roll     net.SteppedReader
}

func (p *ClientboundSpawnItem) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.entityID.Step(rd); !ok {
		return false, err
	}
	p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
	if ok, err := p.itemID.Step(rd); !ok {
		return false, err
	}
	p.ItemID = int16(binary.BigEndian.Uint16(p.itemID.Buf[:]))
	if ok, err := p.amount.Step(rd); !ok {
		return false, err
	}
	p.Amount = p.amount.Buf[0]
	if ok, err := p.meta.Step(rd); !ok {
		return false, err
	}
	p.Meta = int16(binary.BigEndian.Uint16(p.meta.Buf[:]))
	if ok, err := p.x.Step(rd); !ok {
		return false, err
	}
	p.X = int32(binary.BigEndian.Uint32(p.x.Buf[:]))
	if ok, err := p.y.Step(rd); !ok {
		return false, err
	}
	p.Y = int32(binary.BigEndian.Uint32(p.y.Buf[:]))
	if ok, err := p.z.Step(rd); !ok {
		return false, err
	}
	p.Z = int32(binary.BigEndian.Uint32(p.z.Buf[:]))
	if ok, err := p.yaw.Step(rd); !ok {
		return false, err
	}
	p.Yaw = p.yaw.Buf[0]
	if ok, err := p.pitch.Step(rd); !ok {
		return false, err
	}
	p.Pitch = p.pitch.Buf[0]
	if ok, err := p.roll.Step(rd); !ok {
		return false, err
	}
	p.Roll = p.roll.Buf[0]
	return true, nil
}

// Clientbound: Collect Item (0x16)
type ClientboundCollectItem struct {
	ItemEntityID      int32
	CollectorEntityID int32

	itemEntityID      net.SteppedReader32
	collectorEntityID net.SteppedReader32
}

func (p *ClientboundCollectItem) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.itemEntityID.Step(rd); !ok {
		return false, err
	}
	p.ItemEntityID = int32(binary.BigEndian.Uint32(p.itemEntityID.Buf[:]))
	if ok, err := p.collectorEntityID.Step(rd); !ok {
		return false, err
	}
	p.CollectorEntityID = int32(binary.BigEndian.Uint32(p.collectorEntityID.Buf[:]))
	return true, nil
}

// Clientbound: Spawn Object (0x17)
type ClientboundSpawnObject struct {
	EntityID   int32
	ObjectType byte
	X, Y, Z    int32
	Pitch      byte
	Yaw        byte

	entityID   net.SteppedReader32
	objectType net.SteppedReader
	x          net.SteppedReader32
	y          net.SteppedReader32
	z          net.SteppedReader32
	pitch      net.SteppedReader
	yaw        net.SteppedReader
}

func (p *ClientboundSpawnObject) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.entityID.Step(rd); !ok {
		return false, err
	}
	p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
	if ok, err := p.objectType.Step(rd); !ok {
		return false, err
	}
	p.ObjectType = p.objectType.Buf[0]
	if ok, err := p.x.Step(rd); !ok {
		return false, err
	}
	p.X = int32(binary.BigEndian.Uint32(p.x.Buf[:]))
	if ok, err := p.y.Step(rd); !ok {
		return false, err
	}
	p.Y = int32(binary.BigEndian.Uint32(p.y.Buf[:]))
	if ok, err := p.z.Step(rd); !ok {
		return false, err
	}
	p.Z = int32(binary.BigEndian.Uint32(p.z.Buf[:]))
	if ok, err := p.pitch.Step(rd); !ok {
		return false, err
	}
	p.Pitch = p.pitch.Buf[0]
	if ok, err := p.yaw.Step(rd); !ok {
		return false, err
	}
	p.Yaw = p.yaw.Buf[0]
	return true, nil
}

// Clientbound: Spawn Painting (0x19)
type ClientboundSpawnPainting struct {
	EntityID int32
	Title    String16

	titleReader String16Reader
	entityID    net.SteppedReader32
}

func (p *ClientboundSpawnPainting) Step(a mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.entityID.Step(rd); !ok {
		return false, err
	}
	p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
	if ok, err := p.titleReader.Step(a, rd); !ok {
		return false, err
	}
	p.Title = p.titleReader.Runes
	return true, nil
}

// Clientbound: Entity Velocity (0x1C)
type ClientboundEntityVelocity struct {
	EntityID   int32
	XV, YV, ZV int16

	entityID net.SteppedReader32
	xv       net.SteppedReader16
	yv       net.SteppedReader16
	zv       net.SteppedReader16
}

func (p *ClientboundEntityVelocity) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.entityID.Step(rd); !ok {
		return false, err
	}
	p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
	if ok, err := p.xv.Step(rd); !ok {
		return false, err
	}
	p.XV = int16(binary.BigEndian.Uint16(p.xv.Buf[:]))
	if ok, err := p.yv.Step(rd); !ok {
		return false, err
	}
	p.YV = int16(binary.BigEndian.Uint16(p.yv.Buf[:]))
	if ok, err := p.zv.Step(rd); !ok {
		return false, err
	}
	p.ZV = int16(binary.BigEndian.Uint16(p.zv.Buf[:]))
	return true, nil
}

// Clientbound: Despawn Entity (0x1D)
type ClientboundDespawnEntity struct {
	EntityID int32

	entityID net.SteppedReader32
}

func (p *ClientboundDespawnEntity) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.entityID.Step(rd); !ok {
		return false, err
	}
	p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
	return true, nil
}

// Clientbound: Entity Position (0x1F)
type ClientboundEntityPosition struct {
	EntityID int32
	X, Y, Z  float32

	entityID net.SteppedReader32
	x        net.SteppedReader32
	y        net.SteppedReader32
	z        net.SteppedReader32
}

func (p *ClientboundEntityPosition) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.entityID.Step(rd); !ok {
		return false, err
	}
	p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
	if ok, err := p.x.Step(rd); !ok {
		return false, err
	}
	p.X = math.Float32frombits(binary.BigEndian.Uint32(p.x.Buf[:]))
	if ok, err := p.y.Step(rd); !ok {
		return false, err
	}
	p.Y = math.Float32frombits(binary.BigEndian.Uint32(p.y.Buf[:]))
	if ok, err := p.z.Step(rd); !ok {
		return false, err
	}
	p.Z = math.Float32frombits(binary.BigEndian.Uint32(p.z.Buf[:]))
	return true, nil
}

// Clientbound: Entity Rotation (0x20)
type ClientboundEntityRotation struct {
	EntityID   int32
	Yaw, Pitch byte

	entityID net.SteppedReader32
	yaw      net.SteppedReader
	pitch    net.SteppedReader
}

func (p *ClientboundEntityRotation) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.entityID.Step(rd); !ok {
		return false, err
	}
	p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
	if ok, err := p.yaw.Step(rd); !ok {
		return false, err
	}
	p.Yaw = p.yaw.Buf[0]
	if ok, err := p.pitch.Step(rd); !ok {
		return false, err
	}
	p.Pitch = p.pitch.Buf[0]
	return true, nil
}

// Clientbound: Entity Position and Rotation (0x21)
type ClientboundEntityPositionAndRotation struct {
	EntityID   int32
	X, Y, Z    float32
	Yaw, Pitch byte

	entityID net.SteppedReader32
	x        net.SteppedReader32
	y        net.SteppedReader32
	z        net.SteppedReader32
	yaw      net.SteppedReader
	pitch    net.SteppedReader
}

func (p *ClientboundEntityPositionAndRotation) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.entityID.Step(rd); !ok {
		return false, err
	}
	p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
	if ok, err := p.x.Step(rd); !ok {
		return false, err
	}
	p.X = math.Float32frombits(binary.BigEndian.Uint32(p.x.Buf[:]))
	if ok, err := p.y.Step(rd); !ok {
		return false, err
	}
	p.Y = math.Float32frombits(binary.BigEndian.Uint32(p.y.Buf[:]))
	if ok, err := p.z.Step(rd); !ok {
		return false, err
	}
	p.Z = math.Float32frombits(binary.BigEndian.Uint32(p.z.Buf[:]))
	if ok, err := p.yaw.Step(rd); !ok {
		return false, err
	}
	p.Yaw = p.yaw.Buf[0]
	if ok, err := p.pitch.Step(rd); !ok {
		return false, err
	}
	p.Pitch = p.pitch.Buf[0]
	return true, nil
}

// Clientbound: Teleport Entity (0x22)
type ClientboundTeleportEntity struct {
	EntityID   int32
	X, Y, Z    int32
	Yaw, Pitch byte

	entityID net.SteppedReader32
	x        net.SteppedReader32
	y        net.SteppedReader32
	z        net.SteppedReader32
	yaw      net.SteppedReader
	pitch    net.SteppedReader
}

func (p *ClientboundTeleportEntity) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.entityID.Step(rd); !ok {
		return false, err
	}
	p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
	if ok, err := p.x.Step(rd); !ok {
		return false, err
	}
	p.X = int32(binary.BigEndian.Uint32(p.x.Buf[:]))
	if ok, err := p.y.Step(rd); !ok {
		return false, err
	}
	p.Y = int32(binary.BigEndian.Uint32(p.y.Buf[:]))
	if ok, err := p.z.Step(rd); !ok {
		return false, err
	}
	p.Z = int32(binary.BigEndian.Uint32(p.z.Buf[:]))
	if ok, err := p.yaw.Step(rd); !ok {
		return false, err
	}
	p.Yaw = p.yaw.Buf[0]
	if ok, err := p.pitch.Step(rd); !ok {
		return false, err
	}
	p.Pitch = p.pitch.Buf[0]
	return true, nil
}

// Clientbound: Entity Event (0x26)
type ClientboundEntityEvent struct {
	EntityID int32
	Action   byte

	entityID net.SteppedReader32
	action   net.SteppedReader
}

func (p *ClientboundEntityEvent) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.entityID.Step(rd); !ok {
		return false, err
	}
	p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
	if ok, err := p.action.Step(rd); !ok {
		return false, err
	}
	p.Action = p.action.Buf[0]
	return true, nil
}

// Clientbound: Add Passenger (0x27)
type ClientboundAddPassenger struct {
	EntityID  int32
	VehicleID int32

	entityID  net.SteppedReader32
	vehicleID net.SteppedReader32
}

func (p *ClientboundAddPassenger) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.entityID.Step(rd); !ok {
		return false, err
	}
	p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
	if ok, err := p.vehicleID.Step(rd); !ok {
		return false, err
	}
	p.VehicleID = int32(binary.BigEndian.Uint32(p.vehicleID.Buf[:]))
	return true, nil
}

// Serverbound: Player Action (0x13) struct and writer
type ServerboundPlayerAction struct {
	EntityID int32
	Action   byte
}

func (p ServerboundPlayerAction) Write(w io.Writer) error {
	if err := WriteInteger(w, p.EntityID); err != nil {
		return err
	}
	if err := WriteByte(w, p.Action); err != nil {
		return err
	}
	return nil
}

// Clientbound: Set Chunk Visibility (0x32)
type ClientboundSetChunkVisibility struct {
	X, Z int32
	Load bool

	x net.SteppedReader32
	z net.SteppedReader32
	l net.SteppedReader
}

func (p *ClientboundSetChunkVisibility) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.x.Step(rd); !ok {
		return false, err
	}
	p.X = int32(binary.BigEndian.Uint32(p.x.Buf[:]))
	if ok, err := p.z.Step(rd); !ok {
		return false, err
	}
	p.Z = int32(binary.BigEndian.Uint32(p.z.Buf[:]))
	if ok, err := p.l.Step(rd); !ok {
		return false, err
	}
	p.Load = p.l.Buf[0] != 0
	return true, nil
}

// Clientbound: Set Block (0x35)
type ClientboundSetBlock struct {
	X        int32
	Y        int16
	Z        int32
	Type     byte
	Metadata byte

	x     net.SteppedReader32
	y     net.SteppedReader16
	z     net.SteppedReader32
	typeR net.SteppedReader
	meta  net.SteppedReader
}

func (p *ClientboundSetBlock) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.x.Step(rd); !ok {
		return false, err
	}
	p.X = int32(binary.BigEndian.Uint32(p.x.Buf[:]))
	if ok, err := p.y.Step(rd); !ok {
		return false, err
	}
	p.Y = int16(binary.BigEndian.Uint16(p.y.Buf[:]))
	if ok, err := p.z.Step(rd); !ok {
		return false, err
	}
	p.Z = int32(binary.BigEndian.Uint32(p.z.Buf[:]))
	if ok, err := p.typeR.Step(rd); !ok {
		return false, err
	}
	p.Type = p.typeR.Buf[0]
	if ok, err := p.meta.Step(rd); !ok {
		return false, err
	}
	p.Metadata = p.meta.Buf[0]
	return true, nil
}

// Clientbound: Block Event (0x36)
type ClientboundBlockEvent struct {
	X int32
	Y int16
	Z int32
	A byte
	B byte

	x net.SteppedReader32
	y net.SteppedReader16
	z net.SteppedReader32
	a net.SteppedReader
	b net.SteppedReader
}

func (p *ClientboundBlockEvent) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.x.Step(rd); !ok {
		return false, err
	}
	p.X = int32(binary.BigEndian.Uint32(p.x.Buf[:]))
	if ok, err := p.y.Step(rd); !ok {
		return false, err
	}
	p.Y = int16(binary.BigEndian.Uint16(p.y.Buf[:]))
	if ok, err := p.z.Step(rd); !ok {
		return false, err
	}
	p.Z = int32(binary.BigEndian.Uint32(p.z.Buf[:]))
	if ok, err := p.a.Step(rd); !ok {
		return false, err
	}
	p.A = p.a.Buf[0]
	if ok, err := p.b.Step(rd); !ok {
		return false, err
	}
	p.B = p.b.Buf[0]
	return true, nil
}

// Clientbound: World Event (0x3D)
type ClientboundWorldEvent struct {
	EffectID int32
	X        int32
	Y        byte
	Z        int32
	Data     int32

	effectID net.SteppedReader32
	x        net.SteppedReader32
	y        net.SteppedReader
	z        net.SteppedReader32
	data     net.SteppedReader32
}

func (p *ClientboundWorldEvent) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.effectID.Step(rd); !ok {
		return false, err
	}
	p.EffectID = int32(binary.BigEndian.Uint32(p.effectID.Buf[:]))
	if ok, err := p.x.Step(rd); !ok {
		return false, err
	}
	p.X = int32(binary.BigEndian.Uint32(p.x.Buf[:]))
	if ok, err := p.y.Step(rd); !ok {
		return false, err
	}
	p.Y = p.y.Buf[0]
	if ok, err := p.z.Step(rd); !ok {
		return false, err
	}
	p.Z = int32(binary.BigEndian.Uint32(p.z.Buf[:]))
	if ok, err := p.data.Step(rd); !ok {
		return false, err
	}
	p.Data = int32(binary.BigEndian.Uint32(p.data.Buf[:]))
	return true, nil
}

// Clientbound: Game Event (0x46)
type ClientboundGameEvent struct {
	Type  byte
	typeR net.SteppedReader
}

func (p *ClientboundGameEvent) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.typeR.Step(rd); !ok {
		return false, err
	}
	p.Type = p.typeR.Buf[0]
	return true, nil
}

// Clientbound: Lightning Bolt (0x47)
type ClientboundLightningBolt struct {
	EntityID   int32
	EntityType byte
	X          int32
	Y          int32
	Z          int32

	entityID   net.SteppedReader32
	entityType net.SteppedReader
	x          net.SteppedReader32
	y          net.SteppedReader32
	z          net.SteppedReader32
}

func (p *ClientboundLightningBolt) Step(_ mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.entityID.Step(rd); !ok {
		return false, err
	}
	p.EntityID = int32(binary.BigEndian.Uint32(p.entityID.Buf[:]))
	if ok, err := p.entityType.Step(rd); !ok {
		return false, err
	}
	p.EntityType = p.entityType.Buf[0]
	if ok, err := p.x.Step(rd); !ok {
		return false, err
	}
	p.X = int32(binary.BigEndian.Uint32(p.x.Buf[:]))
	if ok, err := p.y.Step(rd); !ok {
		return false, err
	}
	p.Y = int32(binary.BigEndian.Uint32(p.y.Buf[:]))
	if ok, err := p.z.Step(rd); !ok {
		return false, err
	}
	p.Z = int32(binary.BigEndian.Uint32(p.z.Buf[:]))
	return true, nil
}

// Both: Disconnect (0xFF)
type PacketDisconnect struct {
	Reason String16
	reason String16Reader
}

func (p *PacketDisconnect) Step(a mem.Allocator, rd io.Reader) (bool, error) {
	if ok, err := p.reason.Step(a, rd); !ok {
		return false, err
	}
	p.Reason = p.reason.Runes
	return true, nil
}

func (p PacketDisconnect) Write(w io.Writer) error {
	return WriteString16(w, p.Reason)
}

// PacketRespawn (both directions)
func (p *PacketRespawn) Write(w io.Writer) error {
	return WriteByte(w, byte(p.World))
}

// PacketPlayerMovement (both directions)
func (p *PacketPlayerMovement) Write(w io.Writer) error {
	return WriteBool(w, p.OnGround)
}

// PacketPlayerPosition (both directions)
func (p *PacketPlayerPosition) Write(w io.Writer) error {
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

// PacketPlayerRotation (both directions)
func (p *PacketPlayerRotation) Write(w io.Writer) error {
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

// PacketPlayerPositionAndRotation (both directions)
func (p *PacketPlayerPositionAndRotation) Write(w io.Writer) error {
	if err := p.Position.Write(w); err != nil {
		return err
	}
	if err := p.Rotation.Write(w); err != nil {
		return err
	}
	return nil
}

// Returns a decoder for the given packet id. It is the user's job to free the decoder.
// Returns nil if packetID is invalid.
func NewDecoder(a mem.Allocator, packetID PacketID) Decoder {
	switch packetID {
	case PKT_SetSpawnPosition:
		return mem.Alloc[ClientboundSetSpawnPosition](a)
	}
	return nil
}

// TODO: UNIMPLEMENTED PACKETS
// The following packets are referenced in packet_id.go but are not yet implemented
// in this file. Implementations may require variable-length parsing, nested
// metadata, or container/item format handling. These are left as TODOs so
// future work can pick them up incrementally.
//
// - PKT_SpawnPlayer (0x14) — Clientbound spawn player (entity + metadata)
// - PKT_SpawnMob (0x18) — Clientbound spawn mob (complex entity fields)
// - PKT_PlayerInput (0x1B) — Client-only / unused
// - PKT_EntityMetadata (0x28) — Clientbound entity metadata (variable-length metadata)
// - PKT_Chunk (0x33) — Clientbound chunk data (length-prefixed, compressed payload)
// - PKT_SetMultipleBlocks (0x34) — Clientbound multiple block updates (count + entries)
// - PKT_Explosion (0x3C) — Clientbound explosion (variable-length records)
// - PKT_OpenContainer .. PKT_ContainerTransaction (0x64-0x6A) — Container/inventory packets
// - PKT_UpdateSign (0x82) & PKT_ItemData (0x83) — Sign and item-data handling
// - PKT_IncrementStatistic (0xC8) — Statistic increments
// - (Others) Any packet IDs present in packet_id.go that do not yet have
//   corresponding types/Step/Write implementations in this file.
//
// Implementation notes:
// - Use the existing net.SteppedReader* types and String16Reader/WriteString16
//   helpers for incremental, non-blocking parsing and writing.
// - Chunk, EntityMetadata, Container and ItemData require bespoke parsing loops
//   and often allocate slices; prefer mem.TryAllocSlice where appropriate.
// - Add unit tests / parsing fuzz tests for complex packets once implemented.
// - When adding implementations, update beta-wiki links above the type with
//   the appropriate documentation file path for easy cross-reference.
