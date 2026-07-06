#include "mc.h"

// -- Variables and constants --
so_Error mc_IncompleteMetadataErr = errors_New("Tried parsing incomplete metadata");
static so_String packetIDStrings[256] = {[0x00] = so_str("KeepAlive"), [0x01] = so_str("Login"), [0x02] = so_str("PreLogin"), [0x03] = so_str("ChatMessage"), [0x04] = so_str("SetTime"), [0x05] = so_str("SetEquipment"), [0x06] = so_str("SetSpawnPosition"), [0x07] = so_str("InteractWithEntity"), [0x08] = so_str("SetHealth"), [0x09] = so_str("Respawn"), [0x0A] = so_str("PlayerMovement"), [0x0B] = so_str("PlayerPosition"), [0x0C] = so_str("PlayerRotation"), [0x0D] = so_str("PlayerPositionAndRotation"), [0x0E] = so_str("MineBlock"), [0x0F] = so_str("PlaceBlock"), [0x10] = so_str("SetHotbarSlot"), [0x11] = so_str("InteractWithBlock"), [0x12] = so_str("Animation"), [0x13] = so_str("PlayerAction"), [0x14] = so_str("SpawnPlayer"), [0x15] = so_str("SpawnItem"), [0x16] = so_str("CollectItem"), [0x17] = so_str("SpawnObject"), [0x18] = so_str("SpawnMob"), [0x19] = so_str("SpawnPainting"), [0x1B] = so_str("PlayerInput"), [0x1C] = so_str("EntityVelocity"), [0x1D] = so_str("DespawnEntity"), [0x1E] = so_str("EntityMovement"), [0x1F] = so_str("EntityPosition"), [0x20] = so_str("EntityRotation"), [0x21] = so_str("EntityPositionAndRotation"), [0x22] = so_str("TeleportEntity"), [0x26] = so_str("EntityEvent"), [0x27] = so_str("AddPassenger"), [0x28] = so_str("EntityMetadata"), [0x32] = so_str("SetChunkVisibility"), [0x33] = so_str("Chunk"), [0x34] = so_str("SetMultipleBlocks"), [0x35] = so_str("SetBlock"), [0x36] = so_str("BlockEvent"), [0x3C] = so_str("Explosion"), [0x3D] = so_str("WorldEvent"), [0x46] = so_str("GameEvent"), [0x47] = so_str("LightningBolt"), [0x64] = so_str("OpenContainer"), [0x65] = so_str("CloseContainer"), [0x66] = so_str("ClickSlot"), [0x67] = so_str("SetSlot"), [0x68] = so_str("FillContainer"), [0x69] = so_str("ContainerData"), [0x6A] = so_str("ContainerTransaction"), [0x82] = so_str("UpdateSign"), [0x83] = so_str("ItemData"), [0xC8] = so_str("IncrementStatistic"), [0xFF] = so_str("Disconnect")};
static so_byte __PacketIDStringbuf[50] = {0};

// -- blocks.go --

// -- entities.go --

// -- packet.go --

// rotation data is quantized to only a single 8-bit Byte.
float mc_UnquantizeAngle(so_byte angle) {
    return ((float)((int8_t)(angle)) / 255.0) * 360;
}

mc_EntityMetadata mc_MetadataReader_Parse(void* self, uint8_t e) {
    mc_MetadataReader* r = self;
    mc_EntityMetadata m = {0};
    for (so_int _ = 0; _ < so_len(r->metadataValues); _++) {
        mc_MetadataValue value = so_at(mc_MetadataValue, r->metadataValues, _);
        if (value.ID == (0)) {
            m.Burning = (value.Byte & 0x01) != 0;
            m.Sneaking = (value.Byte & 0x02) != 0;
            m.Riding = (value.Byte & 0x03) != 0;
        } else if (value.ID == (16)) {
            if (e == (mc_MOB_Pig)) {
                m.Saddled = value.Byte != 0;
            } else if (e == (mc_MOB_Creeper)) {
                m.BlowingUp = (int8_t)(value.Byte) != -1;
            } else if (e == (mc_MOB_Sheep)) {
                m.Sheared = value.Byte == 16;
                if (value.Byte < 16) {
                    m.SheepColor = value.Byte;
                }
            } else if (e == (mc_MOB_Slime)) {
                m.Size = value.Byte;
            } else if (e == (mc_MOB_Ghast)) {
                m.Attacking = value.Byte != 0;
            } else if (e == (mc_MOB_Wolf)) {
                m.Sitting = value.Byte != 0;
            }
        } else if (value.ID == (17)) {
            if (e == mc_MOB_Creeper) {
                m.Charged = value.Byte != 0;
            }
        } else if (value.ID == (18)) {
            if (e == mc_MOB_Wolf) {
                m.Health = value.Integer;
            }
        }
    }
    return m;
}

so_R_bool_err mc_MetadataReader_Step(void* self, mem_Allocator a, net_BufferedReader* rd) {
    mc_MetadataReader* m = self;
    const int64_t READING_HEADER = 0;
    const int64_t READING_BYTE = 1;
    const int64_t READING_SHORT = 2;
    const int64_t READING_INTEGER = 3;
    const int64_t READING_FLOAT = 4;
    const int64_t READING_STRING = 5;
    const int64_t READING_ITEM_ID = 6;
    const int64_t READING_ITEM_QUANTITY = 7;
    const int64_t READING_ITEM_METADATA = 8;
    const int64_t READING_COORDINATE_X = 9;
    const int64_t READING_COORDINATE_Y = 10;
    const int64_t READING_COORDINATE_Z = 11;
    const int64_t COMPLETED = 12;
    if (m->state == (COMPLETED)) {
        return (so_R_bool_err){.val = true, .err = (so_Error){0}};
    } else if (m->state == (READING_BYTE)) {
        so_R_bool_err _res1 = net_SteppedReader_Step(&m->metadata.rByte, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
        m->metadata.Byte = m->metadata.rByte.Buf[0];
        // save
        m->metadataValues = slices_Append(mc_MetadataValue, (a), (m->metadataValues), (m->metadata));
        // read next header
        m->state = READING_HEADER;
    } else if (m->state == (READING_SHORT)) {
        so_R_bool_err _res2 = net_SteppedReader16_Step(&m->metadata.rShort, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
        m->metadata.Short = (int16_t)(binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, m->metadata.rShort.Buf, 0, 2, 2)));
        // save
        m->metadataValues = slices_Append(mc_MetadataValue, (a), (m->metadataValues), (m->metadata));
        // read next header
        m->state = READING_HEADER;
    } else if (m->state == (READING_INTEGER)) {
        so_R_bool_err _res3 = net_SteppedReader32_Step(&m->metadata.rInteger, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
        m->metadata.Integer = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, m->metadata.rInteger.Buf, 0, 4, 4)));
        // save
        m->metadataValues = slices_Append(mc_MetadataValue, (a), (m->metadataValues), (m->metadata));
        // read next header
        m->state = READING_HEADER;
    } else if (m->state == (READING_FLOAT)) {
        // beta 1.7.3 servers do not actually send this, but it is good to implement it
        so_R_bool_err _res4 = net_SteppedReader32_Step(&m->metadata.rFloat, rd);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
        m->metadata.Float = (float)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, m->metadata.rFloat.Buf, 0, 4, 4)));
        // save
        m->metadataValues = slices_Append(mc_MetadataValue, (a), (m->metadataValues), (m->metadata));
        // read next header
        m->state = READING_HEADER;
        SDL_Log("Server sent float32 metadata. Vanilla servers do not do this.");
    } else if (m->state == (READING_STRING)) {
        // this is only sent for wolf owner.
        // This data is not used. we just parse it because we need to
        // not break the stream.
        //
        // The reason we dont use the wolf owner string is because
        // dealing with memory allocations for something that's not used
        // is pointless.
        so_R_bool_err _res5 = mc_String16Reader_Step(&m->metadata.rString, a, rd);
        bool ok = _res5.val;
        so_Error err = _res5.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
        m->metadata.String = m->metadata.rString.Runes;
        // save
        m->metadataValues = slices_Append(mc_MetadataValue, (a), (m->metadataValues), (m->metadata));
        // read next header
        m->state = READING_HEADER;
    } else if (m->state == (READING_ITEM_ID)) {
        so_R_bool_err _res6 = net_SteppedReader16_Step(&m->metadata.Item.rID, rd);
        bool ok = _res6.val;
        so_Error err = _res6.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
        m->metadata.Item.ID = (int16_t)(binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, m->metadata.Item.rID.Buf, 0, 2, 2)));
        // read next field
        m->state = READING_ITEM_QUANTITY;
    } else if (m->state == (READING_ITEM_QUANTITY)) {
        so_R_bool_err _res7 = net_SteppedReader_Step(&m->metadata.Item.rQuantity, rd);
        bool ok = _res7.val;
        so_Error err = _res7.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
        m->metadata.Item.Quantity = m->metadata.Item.rQuantity.Buf[0];
        // read next field
        m->state = READING_ITEM_METADATA;
    } else if (m->state == (READING_ITEM_METADATA)) {
        so_R_bool_err _res8 = net_SteppedReader16_Step(&m->metadata.Item.rMetadata, rd);
        bool ok = _res8.val;
        so_Error err = _res8.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
        m->metadata.Item.Metadata = binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, m->metadata.Item.rMetadata.Buf, 0, 2, 2));
        // save
        m->metadataValues = slices_Append(mc_MetadataValue, (a), (m->metadataValues), (m->metadata));
        // read next header
        m->state = READING_HEADER;
    } else if (m->state == (READING_COORDINATE_X)) {
        so_R_bool_err _res9 = net_SteppedReader32_Step(&m->metadata.Coordinates.rX, rd);
        bool ok = _res9.val;
        so_Error err = _res9.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
        m->metadata.Coordinates.X = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, m->metadata.Coordinates.rX.Buf, 0, 4, 4)));
        m->state = READING_COORDINATE_Y;
    } else if (m->state == (READING_COORDINATE_Y)) {
        so_R_bool_err _res10 = net_SteppedReader32_Step(&m->metadata.Coordinates.rY, rd);
        bool ok = _res10.val;
        so_Error err = _res10.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
        m->metadata.Coordinates.Y = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, m->metadata.Coordinates.rY.Buf, 0, 4, 4)));
        m->state = READING_COORDINATE_Z;
    } else if (m->state == (READING_COORDINATE_Z)) {
        so_R_bool_err _res11 = net_SteppedReader32_Step(&m->metadata.Coordinates.rZ, rd);
        bool ok = _res11.val;
        so_Error err = _res11.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
        m->metadata.Coordinates.Z = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, m->metadata.Coordinates.rZ.Buf, 0, 4, 4)));
        // save
        m->metadataValues = slices_Append(mc_MetadataValue, (a), (m->metadataValues), (m->metadata));
        // read next header
        m->state = READING_HEADER;
    } else if (m->state == (READING_HEADER)) {
        so_R_bool_err _res12 = net_SteppedReader_Step(&m->header, rd);
        bool ok = _res12.val;
        so_Error err = _res12.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
        so_byte value = m->header.Buf[0];
        if (value == 127) {
            m->state = COMPLETED;
            return (so_R_bool_err){.val = true, .err = (so_Error){0}};
        }
        m->dataType = (value >> 5);
        m->metadataID = (value & 0x1F);
        if (m->dataType == (0) || m->dataType == (1) || m->dataType == (2) || m->dataType == (3) || m->dataType == (4)) {
            m->state = READING_BYTE + m->dataType;
        } else if (m->dataType == (5)) {
            m->state = READING_ITEM_ID;
        } else if (m->dataType == (6)) {
            m->state = READING_COORDINATE_X;
        } else {
            so_panic("INVALID METADATA TYPE");
        }
        net_SteppedReader_Reset(&m->header);
        m->metadata = c_Zero(mc_MetadataValue);
        m->metadata.DataType = m->dataType;
        m->metadata.ID = m->metadataID;
    }
    return (so_R_bool_err){.val = false, .err = (so_Error){0}};
}

// Read implements [ClientBoundPacket].
so_R_bool_err mc_PacketKeepAlive_Step(void* self) {
    mc_PacketKeepAlive* p = self;
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

// Write implements [ServerBoundPacket].
so_Error mc_PacketKeepAlive_Write(mc_PacketKeepAlive p) {
    return (so_Error){0};
}

so_R_bool_err mc_ClientboundLogin_Step(void* self, mem_Allocator _, net_BufferedReader* r) {
    mc_ClientboundLogin* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, r);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = mc_String16Reader_Step(&p->unused, mem_NoAlloc, r);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Unused = p->unused.Runes;
    {
        so_R_bool_err _res3 = net_SteppedReader64_Step(&p->worldSeed, r);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->WorldSeed = (int64_t)(binary_BE_Uint64(binary_BigEndian, so_array_slice(so_byte, p->worldSeed.Buf, 0, 8, 8)));
    {
        so_R_bool_err _res4 = net_SteppedReader_Step(&p->dimension, r);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Dimension = p->dimension.Buf[0];
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_Error mc_ServerboundLogin_Write(mc_ServerboundLogin p, io_Writer w) {
    {
        so_Error err = mc_WriteInteger(w, p.ProtocolVersion);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteString16(w, p.Username);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteLong(w, 0);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteByte(w, 0);
        if (err.self != NULL) {
            return err;
        }
    }
    return (so_Error){0};
}

so_R_bool_err mc_ClientboundPreLogin_Step(void* self, mem_Allocator a, net_BufferedReader* rd) {
    mc_ClientboundPreLogin* p = self;
    {
        so_R_bool_err _res1 = mc_String16Reader_Step(&p->connectionHash, a, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = ok, .err = err};
        }
    }
    p->ConnectionHash = p->connectionHash.Runes;
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_Error mc_ServerboundPreLogin_Write(mc_ServerboundPreLogin p, io_Writer w) {
    return mc_WriteString16(w, p.Username);
}

so_Error mc_PacketChatMessage_Write(void* self, io_Writer w) {
    mc_PacketChatMessage* p = self;
    return mc_WriteString16(w, p->Message);
}

so_R_bool_err mc_PacketChatMessage_Step(void* self, mem_Allocator a, net_BufferedReader* rd) {
    mc_PacketChatMessage* p = self;
    {
        so_R_bool_err _res1 = mc_String16Reader_Step(&p->message, a, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Message = p->message.Runes;
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundSetTime_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundSetTime* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader64_Step(&p->time, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Time = (int64_t)(binary_BE_Uint64(binary_BigEndian, so_array_slice(so_byte, p->time.Buf, 0, 8, 8)));
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundSetEquipment_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundSetEquipment* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader16_Step(&p->inventorySlot, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->InventorySlot = (int16_t)(binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, p->inventorySlot.Buf, 0, 2, 2)));
    {
        so_R_bool_err _res3 = net_SteppedReader16_Step(&p->itemID, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->ItemID = (int16_t)(binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, p->itemID.Buf, 0, 2, 2)));
    {
        so_R_bool_err _res4 = net_SteppedReader16_Step(&p->itemMetadata, rd);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->ItemMetadata = (int16_t)(binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, p->itemMetadata.Buf, 0, 2, 2)));
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundSetSpawnPosition_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundSetSpawnPosition* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->x, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->X = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->x.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader32_Step(&p->y, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Y = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->y.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res3 = net_SteppedReader32_Step(&p->z, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Z = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->z.Buf, 0, 4, 4)));
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundSetHealth_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundSetHealth* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader16_Step(&p->health, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Health = (int16_t)(binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, p->health.Buf, 0, 2, 2)));
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_PacketRespawn_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_PacketRespawn* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader_Step(&p->world, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->World = (int8_t)(p->world.Buf[0]);
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_PacketPlayerMovement_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_PacketPlayerMovement* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader_Step(&p->onGround, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->OnGround = p->onGround.Buf[0] != 0;
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_PacketPlayerPosition_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_PacketPlayerPosition* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader64_Step(&p->x, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->X = math_Float64frombits(binary_BE_Uint64(binary_BigEndian, so_array_slice(so_byte, p->x.Buf, 0, 8, 8)));
    {
        so_R_bool_err _res2 = net_SteppedReader64_Step(&p->y, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Y = math_Float64frombits(binary_BE_Uint64(binary_BigEndian, so_array_slice(so_byte, p->y.Buf, 0, 8, 8)));
    {
        so_R_bool_err _res3 = net_SteppedReader64_Step(&p->cameraY, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->CameraY = math_Float64frombits(binary_BE_Uint64(binary_BigEndian, so_array_slice(so_byte, p->cameraY.Buf, 0, 8, 8)));
    {
        so_R_bool_err _res4 = net_SteppedReader64_Step(&p->z, rd);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Z = math_Float64frombits(binary_BE_Uint64(binary_BigEndian, so_array_slice(so_byte, p->z.Buf, 0, 8, 8)));
    {
        so_R_bool_err _res5 = net_SteppedReader_Step(&p->onGround, rd);
        bool ok = _res5.val;
        so_Error err = _res5.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->OnGround = p->onGround.Buf[0] != 0;
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_PacketPlayerRotation_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_PacketPlayerRotation* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->yaw, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Yaw = math_Float32frombits(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->yaw.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader32_Step(&p->pitch, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Pitch = math_Float32frombits(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->pitch.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res3 = net_SteppedReader_Step(&p->onGround, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->OnGround = p->onGround.Buf[0] != 0;
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_PacketPlayerPositionAndRotation_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_PacketPlayerPositionAndRotation* p = self;
    {
        so_R_bool_err _res1 = mc_PacketPlayerPosition_Step(&p->Position, (mem_Allocator){0}, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    {
        so_R_bool_err _res2 = mc_PacketPlayerRotation_Step(&p->Rotation, (mem_Allocator){0}, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_Error mc_ServerboundMineBlock_Write(mc_ServerboundMineBlock p, io_Writer w) {
    {
        so_Error err = mc_WriteByte(w, p.Status);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteInteger(w, p.X);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteByte(w, p.Y);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteInteger(w, p.Z);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteByte(w, p.Face);
        if (err.self != NULL) {
            return err;
        }
    }
    return (so_Error){0};
}

so_Error mc_ServerboundPlaceBlock_Write(mc_ServerboundPlaceBlock p, io_Writer w) {
    {
        so_Error err = mc_WriteInteger(w, p.X);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteByte(w, p.Y);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteInteger(w, p.Z);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteByte(w, p.Face);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteShort(w, p.BlockItemID);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteByte(w, p.Amount);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteShort(w, p.Metadata);
        if (err.self != NULL) {
            return err;
        }
    }
    return (so_Error){0};
}

so_Error mc_ServerboundSetHotbarSlot_Write(mc_ServerboundSetHotbarSlot p, io_Writer w) {
    return mc_WriteShort(w, p.Slot);
}

// Serverbound: Interact With Entity (0x07)
so_Error mc_ServerboundInteractWithEntity_Write(mc_ServerboundInteractWithEntity p, io_Writer w) {
    {
        so_Error err = mc_WriteInteger(w, p.PlayerID);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteInteger(w, p.EntityID);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteBool(w, p.Attack);
        if (err.self != NULL) {
            return err;
        }
    }
    return (so_Error){0};
}

so_R_bool_err mc_ClientboundInteractWithBlock_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundInteractWithBlock* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader_Step(&p->_type, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Type = p->_type.Buf[0];
    {
        so_R_bool_err _res3 = net_SteppedReader32_Step(&p->x, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->X = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->x.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res4 = net_SteppedReader32_Step(&p->y, rd);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Y = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->y.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res5 = net_SteppedReader32_Step(&p->z, rd);
        bool ok = _res5.val;
        so_Error err = _res5.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Z = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->z.Buf, 0, 4, 4)));
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_PacketAnimation_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_PacketAnimation* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->playerID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->PlayerID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->playerID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader_Step(&p->animation, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Animation = p->animation.Buf[0];
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_Error mc_PacketAnimation_Write(mc_PacketAnimation p, io_Writer w) {
    {
        so_Error err = mc_WriteInteger(w, p.PlayerID);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteByte(w, p.Animation);
        if (err.self != NULL) {
            return err;
        }
    }
    return (so_Error){0};
}

so_R_bool_err mc_ClientboundSpawnItem_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundSpawnItem* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader16_Step(&p->itemID, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->ItemID = (int16_t)(binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, p->itemID.Buf, 0, 2, 2)));
    {
        so_R_bool_err _res3 = net_SteppedReader_Step(&p->amount, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Amount = p->amount.Buf[0];
    {
        so_R_bool_err _res4 = net_SteppedReader16_Step(&p->meta, rd);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Meta = (int16_t)(binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, p->meta.Buf, 0, 2, 2)));
    {
        so_R_bool_err _res5 = net_SteppedReader32_Step(&p->x, rd);
        bool ok = _res5.val;
        so_Error err = _res5.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->X = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->x.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res6 = net_SteppedReader32_Step(&p->y, rd);
        bool ok = _res6.val;
        so_Error err = _res6.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Y = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->y.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res7 = net_SteppedReader32_Step(&p->z, rd);
        bool ok = _res7.val;
        so_Error err = _res7.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Z = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->z.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res8 = net_SteppedReader_Step(&p->yaw, rd);
        bool ok = _res8.val;
        so_Error err = _res8.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Yaw = p->yaw.Buf[0];
    {
        so_R_bool_err _res9 = net_SteppedReader_Step(&p->pitch, rd);
        bool ok = _res9.val;
        so_Error err = _res9.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Pitch = p->pitch.Buf[0];
    {
        so_R_bool_err _res10 = net_SteppedReader_Step(&p->roll, rd);
        bool ok = _res10.val;
        so_Error err = _res10.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Roll = p->roll.Buf[0];
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundCollectItem_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundCollectItem* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->itemEntityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->ItemEntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->itemEntityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader32_Step(&p->collectorEntityID, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->CollectorEntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->collectorEntityID.Buf, 0, 4, 4)));
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundSpawnObject_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundSpawnObject* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader_Step(&p->objectType, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->ObjectType = p->objectType.Buf[0];
    {
        so_R_bool_err _res3 = net_SteppedReader32_Step(&p->x, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->X = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->x.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res4 = net_SteppedReader32_Step(&p->y, rd);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Y = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->y.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res5 = net_SteppedReader32_Step(&p->z, rd);
        bool ok = _res5.val;
        so_Error err = _res5.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Z = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->z.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res6 = net_SteppedReader_Step(&p->pitch, rd);
        bool ok = _res6.val;
        so_Error err = _res6.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Pitch = p->pitch.Buf[0];
    {
        so_R_bool_err _res7 = net_SteppedReader_Step(&p->yaw, rd);
        bool ok = _res7.val;
        so_Error err = _res7.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Yaw = p->yaw.Buf[0];
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientBoundSpawnMob_Step(void* self, mem_Allocator a, net_BufferedReader* rd) {
    mc_ClientBoundSpawnMob* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader_Step(&p->mobType, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->MobType = p->mobType.Buf[0];
    {
        so_R_bool_err _res3 = net_SteppedReader32_Step(&p->x, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->X = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->x.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res4 = net_SteppedReader32_Step(&p->y, rd);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Y = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->y.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res5 = net_SteppedReader32_Step(&p->z, rd);
        bool ok = _res5.val;
        so_Error err = _res5.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Z = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->y.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res6 = net_SteppedReader_Step(&p->yaw, rd);
        bool ok = _res6.val;
        so_Error err = _res6.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Yaw = mc_UnquantizeAngle(p->yaw.Buf[0]);
    {
        so_R_bool_err _res7 = net_SteppedReader_Step(&p->pitch, rd);
        bool ok = _res7.val;
        so_Error err = _res7.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Pitch = mc_UnquantizeAngle(p->pitch.Buf[0]);
    {
        so_R_bool_err _res8 = mc_MetadataReader_Step(&p->metadata, a, rd);
        bool ok = _res8.val;
        so_Error err = _res8.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Metadata = mc_MetadataReader_Parse(&p->metadata, p->MobType);
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundSpawnPainting_Step(void* self, mem_Allocator a, net_BufferedReader* rd) {
    mc_ClientboundSpawnPainting* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = mc_String16Reader_Step(&p->titleReader, a, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Title = p->titleReader.Runes;
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundEntityVelocity_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundEntityVelocity* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader16_Step(&p->xv, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->XV = (int16_t)(binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, p->xv.Buf, 0, 2, 2)));
    {
        so_R_bool_err _res3 = net_SteppedReader16_Step(&p->yv, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->YV = (int16_t)(binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, p->yv.Buf, 0, 2, 2)));
    {
        so_R_bool_err _res4 = net_SteppedReader16_Step(&p->zv, rd);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->ZV = (int16_t)(binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, p->zv.Buf, 0, 2, 2)));
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundDespawnEntity_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundDespawnEntity* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundEntityPosition_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundEntityPosition* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader32_Step(&p->x, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->X = math_Float32frombits(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->x.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res3 = net_SteppedReader32_Step(&p->y, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Y = math_Float32frombits(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->y.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res4 = net_SteppedReader32_Step(&p->z, rd);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Z = math_Float32frombits(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->z.Buf, 0, 4, 4)));
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundEntityRotation_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundEntityRotation* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader_Step(&p->yaw, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Yaw = p->yaw.Buf[0];
    {
        so_R_bool_err _res3 = net_SteppedReader_Step(&p->pitch, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Pitch = p->pitch.Buf[0];
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundEntityPositionAndRotation_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundEntityPositionAndRotation* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader32_Step(&p->x, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->X = math_Float32frombits(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->x.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res3 = net_SteppedReader32_Step(&p->y, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Y = math_Float32frombits(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->y.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res4 = net_SteppedReader32_Step(&p->z, rd);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Z = math_Float32frombits(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->z.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res5 = net_SteppedReader_Step(&p->yaw, rd);
        bool ok = _res5.val;
        so_Error err = _res5.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Yaw = p->yaw.Buf[0];
    {
        so_R_bool_err _res6 = net_SteppedReader_Step(&p->pitch, rd);
        bool ok = _res6.val;
        so_Error err = _res6.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Pitch = p->pitch.Buf[0];
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundTeleportEntity_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundTeleportEntity* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader32_Step(&p->x, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->X = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->x.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res3 = net_SteppedReader32_Step(&p->y, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Y = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->y.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res4 = net_SteppedReader32_Step(&p->z, rd);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Z = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->z.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res5 = net_SteppedReader_Step(&p->yaw, rd);
        bool ok = _res5.val;
        so_Error err = _res5.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Yaw = p->yaw.Buf[0];
    {
        so_R_bool_err _res6 = net_SteppedReader_Step(&p->pitch, rd);
        bool ok = _res6.val;
        so_Error err = _res6.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Pitch = p->pitch.Buf[0];
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundEntityEvent_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundEntityEvent* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader_Step(&p->action, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Action = p->action.Buf[0];
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundAddPassenger_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundAddPassenger* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader32_Step(&p->vehicleID, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->VehicleID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->vehicleID.Buf, 0, 4, 4)));
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_Error mc_ServerboundPlayerAction_Write(mc_ServerboundPlayerAction p, io_Writer w) {
    {
        so_Error err = mc_WriteInteger(w, p.EntityID);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteByte(w, p.Action);
        if (err.self != NULL) {
            return err;
        }
    }
    return (so_Error){0};
}

so_R_bool_err mc_ClientboundSetChunkVisibility_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundSetChunkVisibility* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->x, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->X = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->x.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader32_Step(&p->z, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Z = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->z.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res3 = net_SteppedReader_Step(&p->l, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Load = p->l.Buf[0] != 0;
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundSetBlock_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundSetBlock* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->x, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->X = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->x.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader16_Step(&p->y, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Y = (int16_t)(binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, p->y.Buf, 0, 2, 2)));
    {
        so_R_bool_err _res3 = net_SteppedReader32_Step(&p->z, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Z = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->z.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res4 = net_SteppedReader_Step(&p->typeR, rd);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Type = p->typeR.Buf[0];
    {
        so_R_bool_err _res5 = net_SteppedReader_Step(&p->meta, rd);
        bool ok = _res5.val;
        so_Error err = _res5.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Metadata = p->meta.Buf[0];
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundBlockEvent_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundBlockEvent* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->x, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->X = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->x.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader16_Step(&p->y, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Y = (int16_t)(binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, p->y.Buf, 0, 2, 2)));
    {
        so_R_bool_err _res3 = net_SteppedReader32_Step(&p->z, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Z = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->z.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res4 = net_SteppedReader_Step(&p->a, rd);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->A = p->a.Buf[0];
    {
        so_R_bool_err _res5 = net_SteppedReader_Step(&p->b, rd);
        bool ok = _res5.val;
        so_Error err = _res5.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->B = p->b.Buf[0];
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundWorldEvent_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundWorldEvent* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->effectID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EffectID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->effectID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader32_Step(&p->x, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->X = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->x.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res3 = net_SteppedReader_Step(&p->y, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Y = p->y.Buf[0];
    {
        so_R_bool_err _res4 = net_SteppedReader32_Step(&p->z, rd);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Z = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->z.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res5 = net_SteppedReader32_Step(&p->data, rd);
        bool ok = _res5.val;
        so_Error err = _res5.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Data = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->data.Buf, 0, 4, 4)));
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundGameEvent_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundGameEvent* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader_Step(&p->typeR, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Type = p->typeR.Buf[0];
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_ClientboundLightningBolt_Step(void* self, mem_Allocator _, net_BufferedReader* rd) {
    mc_ClientboundLightningBolt* p = self;
    {
        so_R_bool_err _res1 = net_SteppedReader32_Step(&p->entityID, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityID = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->entityID.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res2 = net_SteppedReader_Step(&p->entityType, rd);
        bool ok = _res2.val;
        so_Error err = _res2.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->EntityType = p->entityType.Buf[0];
    {
        so_R_bool_err _res3 = net_SteppedReader32_Step(&p->x, rd);
        bool ok = _res3.val;
        so_Error err = _res3.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->X = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->x.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res4 = net_SteppedReader32_Step(&p->y, rd);
        bool ok = _res4.val;
        so_Error err = _res4.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Y = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->y.Buf, 0, 4, 4)));
    {
        so_R_bool_err _res5 = net_SteppedReader32_Step(&p->z, rd);
        bool ok = _res5.val;
        so_Error err = _res5.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Z = (int32_t)(binary_BE_Uint32(binary_BigEndian, so_array_slice(so_byte, p->z.Buf, 0, 4, 4)));
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_R_bool_err mc_PacketDisconnect_Step(void* self, mem_Allocator a, net_BufferedReader* rd) {
    mc_PacketDisconnect* p = self;
    {
        so_R_bool_err _res1 = mc_String16Reader_Step(&p->reason, a, rd);
        bool ok = _res1.val;
        so_Error err = _res1.err;
        if (!ok) {
            return (so_R_bool_err){.val = false, .err = err};
        }
    }
    p->Reason = p->reason.Runes;
    return (so_R_bool_err){.val = true, .err = (so_Error){0}};
}

so_Error mc_PacketDisconnect_Write(mc_PacketDisconnect p, io_Writer w) {
    return mc_WriteString16(w, p.Reason);
}

// PacketRespawn (both directions)
so_Error mc_PacketRespawn_Write(void* self, io_Writer w) {
    mc_PacketRespawn* p = self;
    return mc_WriteByte(w, (so_byte)(p->World));
}

// PacketPlayerMovement (both directions)
so_Error mc_PacketPlayerMovement_Write(void* self, io_Writer w) {
    mc_PacketPlayerMovement* p = self;
    return mc_WriteBool(w, p->OnGround);
}

// PacketPlayerPosition (both directions)
so_Error mc_PacketPlayerPosition_Write(void* self, io_Writer w) {
    mc_PacketPlayerPosition* p = self;
    {
        so_Error err = mc_WriteFloat64(w, p->X);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteFloat64(w, p->Y);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteFloat64(w, p->CameraY);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteFloat64(w, p->Z);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteBool(w, p->OnGround);
        if (err.self != NULL) {
            return err;
        }
    }
    return (so_Error){0};
}

// PacketPlayerRotation (both directions)
so_Error mc_PacketPlayerRotation_Write(void* self, io_Writer w) {
    mc_PacketPlayerRotation* p = self;
    {
        so_Error err = mc_WriteFloat32(w, p->Yaw);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteFloat32(w, p->Pitch);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_WriteBool(w, p->OnGround);
        if (err.self != NULL) {
            return err;
        }
    }
    return (so_Error){0};
}

// PacketPlayerPositionAndRotation (both directions)
so_Error mc_PacketPlayerPositionAndRotation_Write(void* self, io_Writer w) {
    mc_PacketPlayerPositionAndRotation* p = self;
    {
        so_Error err = mc_PacketPlayerPosition_Write(&p->Position, w);
        if (err.self != NULL) {
            return err;
        }
    }
    {
        so_Error err = mc_PacketPlayerRotation_Write(&p->Rotation, w);
        if (err.self != NULL) {
            return err;
        }
    }
    return (so_Error){0};
}

// Returns a decoder for the given packet id. It is the user's job to free the decoder.
// Returns nil if packetID is invalid.
mc_Decoder mc_NewDecoder(mem_Allocator a, so_byte packetID) {
    if (packetID == (mc_PKT_SetSpawnPosition)) {
        return (mc_Decoder){.self = mem_Alloc(mc_ClientboundSetSpawnPosition, (a)), .Step = mc_ClientboundSetSpawnPosition_Step};
    } else if (packetID == (mc_PKT_SetTime)) {
        return (mc_Decoder){.self = mem_Alloc(mc_ClientboundSetTime, (a)), .Step = mc_ClientboundSetTime_Step};
    } else if (packetID == (mc_PKT_SpawnMob)) {
        return (mc_Decoder){.self = mem_Alloc(mc_ClientBoundSpawnMob, (a)), .Step = mc_ClientBoundSpawnMob_Step};
    } else if (packetID == (mc_PKT_EntityVelocity)) {
        return (mc_Decoder){.self = mem_Alloc(mc_ClientboundEntityVelocity, (a)), .Step = mc_ClientboundEntityVelocity_Step};
    } else if (packetID == (mc_PKT_SetChunkVisibility)) {
        return (mc_Decoder){.self = mem_Alloc(mc_ClientboundSetChunkVisibility, (a)), .Step = mc_ClientboundSetChunkVisibility_Step};
    } else if (packetID == (mc_PKT_SpawnItem)) {
        return (mc_Decoder){.self = mem_Alloc(mc_ClientboundSpawnItem, (a)), .Step = mc_ClientboundSpawnItem_Step};
    }
    return (mc_Decoder){0};
}

// -- packet_id.go --

// -- packetid_string.go --

so_String mc_PacketIDString(so_byte p) {
    if ((so_int)(p) < 256) {
        {
            so_String s = packetIDStrings[p];
            if (so_string_ne(s, so_str(""))) {
                return s;
            }
        }
    }
    fmt_Buffer buf = fmt_BufferFrom(so_array_slice(so_byte, __PacketIDStringbuf, 0, 50, 50));
    return fmt_Sprintf(buf, "PacketID(0x%X)", p);
}

// -- protocol.go --

// -------------------- BYTE --------------------
so_Error mc_WriteByte(io_Writer w, so_byte v) {
    so_R_int_err _res1 = w.Write(w.self, (so_Slice){(so_byte[1]){v}, 1, 1});
    so_Error err = _res1.err;
    return err;
}

// -------------------- UINT16 / INT16 --------------------
so_Error mc_WriteUnsignedShort(io_Writer w, uint16_t v) {
    so_byte b[2] = {0};
    binary_BE_PutUint16(binary_BigEndian, so_array_slice(so_byte, b, 0, 2, 2), v);
    so_R_int_err _res1 = w.Write(w.self, so_array_slice(so_byte, b, 0, 2, 2));
    so_Error err = _res1.err;
    return err;
}

so_Error mc_WriteShort(io_Writer w, int16_t v) {
    return mc_WriteUnsignedShort(w, (uint16_t)(v));
}

// -------------------- UINT32 / INT32 --------------------
so_Error mc_WriteUnsignedInteger(io_Writer w, uint32_t v) {
    so_byte b[4] = {0};
    binary_BE_PutUint32(binary_BigEndian, so_array_slice(so_byte, b, 0, 4, 4), v);
    so_R_int_err _res1 = w.Write(w.self, so_array_slice(so_byte, b, 0, 4, 4));
    so_Error err = _res1.err;
    return err;
}

so_Error mc_WriteInteger(io_Writer w, int32_t v) {
    return mc_WriteUnsignedInteger(w, (uint32_t)(v));
}

// -------------------- UINT64 / INT64 --------------------
so_Error mc_WriteUnsignedLong(io_Writer w, uint64_t v) {
    so_byte b[8] = {0};
    binary_BE_PutUint64(binary_BigEndian, so_array_slice(so_byte, b, 0, 8, 8), v);
    so_R_int_err _res1 = w.Write(w.self, so_array_slice(so_byte, b, 0, 8, 8));
    so_Error err = _res1.err;
    return err;
}

so_Error mc_WriteLong(io_Writer w, int64_t v) {
    return mc_WriteUnsignedLong(w, (uint64_t)(v));
}

// -------------------- BOOL --------------------
so_Error mc_WriteBool(io_Writer w, bool v) {
    so_byte b = 0;
    if (v) {
        b = 1;
    }
    return mc_WriteByte(w, b);
}

// -------------------- FLOAT / DOUBLE --------------------
so_Error mc_WriteFloat32(io_Writer w, float v) {
    so_byte b[4] = {0};
    binary_BE_PutUint32(binary_BigEndian, so_array_slice(so_byte, b, 0, 4, 4), math_Float32bits(v));
    so_R_int_err _res1 = w.Write(w.self, so_array_slice(so_byte, b, 0, 4, 4));
    so_Error err = _res1.err;
    return err;
}

so_Error mc_WriteFloat64(io_Writer w, double v) {
    so_byte b[8] = {0};
    binary_BE_PutUint64(binary_BigEndian, so_array_slice(so_byte, b, 0, 8, 8), math_Float64bits(v));
    so_R_int_err _res1 = w.Write(w.self, so_array_slice(so_byte, b, 0, 8, 8));
    so_Error err = _res1.err;
    return err;
}

// -------------------- STRING8 (UTF-8) --------------------
so_Error mc_WriteString8(io_Writer w, so_String s) {
    if (so_len(s) == 0) {
        return (so_Error){0};
    }
    {
        so_Error err = mc_WriteUnsignedShort(w, (uint16_t)(so_len(s)));
        if (err.self != NULL) {
            return err;
        }
    }
    so_R_int_err _res1 = w.Write(w.self, so_string_bytes(s));
    so_Error err = _res1.err;
    return err;
}

so_R_bool_err mc_String8Reader_Step(void* self, mem_Allocator a, net_BufferedReader* rd) {
    mc_String8Reader* r = self;
    if (r->step == (0)) {
        {
            so_R_bool_err _res1 = net_SteppedReader16_Step(&r->lenReader, rd);
            bool ok = _res1.val;
            so_Error err = _res1.err;
            if (!ok) {
                return (so_R_bool_err){.val = ok, .err = err};
            }
        }
        r->length = (so_int)(binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, r->lenReader.Buf, 0, 2, 2)));
        if (r->length == 0) {
            return (so_R_bool_err){.val = true, .err = (so_Error){0}};
        }
        so_R_slice_err _res2 = mem_TryAllocSlice(so_byte, (a), ((so_int)(r->length)), ((so_int)(r->length)));
        so_Slice bytes = _res2.val;
        so_Error err = _res2.err;
        if (err.self != NULL) {
            return (so_R_bool_err){.val = false, .err = err};
        }
        r->bytes = bytes;
        r->step++;
    } else if (r->step == (1)) {
        for (; r->bytesIndex < r->length;) {
            {
                so_R_bool_err _res3 = net_SteppedReader_Step(&r->byteReader, rd);
                bool ok = _res3.val;
                so_Error err = _res3.err;
                if (!ok) {
                    return (so_R_bool_err){.val = ok, .err = err};
                }
            }
            so_at(so_byte, r->bytes, r->bytesIndex) = r->byteReader.Buf[0];
            net_SteppedReader_Reset(&r->byteReader);
            r->bytesIndex++;
        }
        r->step++;
    } else if (r->step == (2)) {
        return (so_R_bool_err){.val = true, .err = (so_Error){0}};
    }
    return (so_R_bool_err){.val = false, .err = (so_Error){0}};
}

// -------------------- STRING16 (UCS-2 / UTF-16 subset) --------------------
so_R_bool_err mc_String16Reader_Step(void* self, mem_Allocator a, net_BufferedReader* rd) {
    mc_String16Reader* r = self;
    if (r->step == (0)) {
        {
            so_R_bool_err _res1 = net_SteppedReader16_Step(&r->lenReader, rd);
            bool ok = _res1.val;
            so_Error err = _res1.err;
            if (!ok) {
                return (so_R_bool_err){.val = ok, .err = err};
            }
        }
        r->length = (so_int)(binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, r->lenReader.Buf, 0, 2, 2)));
        if (r->length == 0) {
            return (so_R_bool_err){.val = true, .err = (so_Error){0}};
        }
        so_R_slice_err _res2 = mem_TryAllocSlice(so_rune, (a), ((so_int)(r->length)), ((so_int)(r->length)));
        so_Slice runes = _res2.val;
        so_Error err = _res2.err;
        if (err.self != NULL) {
            return (so_R_bool_err){.val = false, .err = err};
        }
        r->Runes = runes;
        r->step++;
    } else if (r->step == (1)) {
        for (; !(r->runesIndex >= r->length);) {
            {
                so_R_bool_err _res3 = net_SteppedReader16_Step(&r->ucs2Reader, rd);
                bool ok = _res3.val;
                so_Error err = _res3.err;
                if (!ok) {
                    return (so_R_bool_err){.val = ok, .err = err};
                }
            }
            uint16_t v = binary_BE_Uint16(binary_BigEndian, so_array_slice(so_byte, r->ucs2Reader.Buf, 0, 2, 2));
            so_at(so_rune, r->Runes, r->runesIndex) = (so_rune)(v);
            r->runesIndex++;
            net_SteppedReader16_Reset(&r->ucs2Reader);
        }
        // Move to finished state
        r->step++;
        return (so_R_bool_err){.val = true, .err = (so_Error){0}};
    } else if (r->step == (2)) {
        return (so_R_bool_err){.val = true, .err = (so_Error){0}};
    }
    return (so_R_bool_err){.val = false, .err = (so_Error){0}};
}

so_Error mc_WriteString16(io_Writer w, so_Slice s) {
    if (so_len(s) == 0) {
        return (so_Error){0};
    }
    so_Slice runes = (so_Slice)(s);
    {
        so_Error err = mc_WriteUnsignedShort(w, (uint16_t)(so_len(runes)));
        if (err.self != NULL) {
            return err;
        }
    }
    for (so_int _ = 0; _ < so_len(runes); _++) {
        so_rune r = so_at(so_rune, runes, _);
        {
            so_Error err = mc_WriteUnsignedShort(w, (uint16_t)(r));
            if (err.self != NULL) {
                return err;
            }
        }
    }
    return (so_Error){0};
}
