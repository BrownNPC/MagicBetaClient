#pragma once
#include "so/builtin/builtin.h"

// -- Types --

typedef struct binary_LE binary_LE;
typedef struct binary_BE binary_BE;

// A ByteOrder specifies how to convert byte slices into
// 16-, 32-, or 64-bit unsigned integers.
//
// It is implemented by [LittleEndian], [BigEndian], and [NativeEndian].
typedef struct binary_ByteOrder {
    void* self;
    void (*PutUint16)(void* self, so_Slice , uint16_t );
    void (*PutUint32)(void* self, so_Slice , uint32_t );
    void (*PutUint64)(void* self, so_Slice , uint64_t );
    so_String (*String)(void* self);
    uint16_t (*Uint16)(void* self, so_Slice );
    uint32_t (*Uint32)(void* self, so_Slice );
    uint64_t (*Uint64)(void* self, so_Slice );
} binary_ByteOrder;

// AppendByteOrder specifies how to append 16-, 32-, or 64-bit unsigned integers
// into a byte slice.
//
// It is implemented by [LittleEndian], [BigEndian], and [NativeEndian].
typedef struct binary_AppendByteOrder {
    void* self;
    so_Slice (*AppendUint16)(void* self, so_Slice , uint16_t );
    so_Slice (*AppendUint32)(void* self, so_Slice , uint32_t );
    so_Slice (*AppendUint64)(void* self, so_Slice , uint64_t );
    so_String (*String)(void* self);
} binary_AppendByteOrder;

typedef struct binary_LE {
    so_byte empty;
} binary_LE;

typedef struct binary_BE {
    so_byte empty;
} binary_BE;

// -- Variables and constants --

// LittleEndian is the little-endian implementation of [ByteOrder] and [AppendByteOrder].
extern binary_LE binary_LittleEndian;

// BigEndian is the big-endian implementation of [ByteOrder] and [AppendByteOrder].
extern binary_BE binary_BigEndian;

// -- Functions and methods --

// Uint16 returns the uint16 representation of b[0:2].
uint16_t binary_LE_Uint16(binary_LE self, so_Slice b);

// PutUint16 stores v into b[0:2].
void binary_LE_PutUint16(binary_LE self, so_Slice b, uint16_t v);

// AppendUint16 appends the bytes of v to b and returns the appended slice.
// Requires at least 2 bytes of spare capacity in b.
so_Slice binary_LE_AppendUint16(binary_LE self, so_Slice b, uint16_t v);

// Uint32 returns the uint32 representation of b[0:4].
uint32_t binary_LE_Uint32(binary_LE self, so_Slice b);

// PutUint32 stores v into b[0:4].
void binary_LE_PutUint32(binary_LE self, so_Slice b, uint32_t v);

// AppendUint32 appends the bytes of v to b and returns the appended slice.
// Requires at least 4 bytes of spare capacity in b.
so_Slice binary_LE_AppendUint32(binary_LE self, so_Slice b, uint32_t v);

// Uint64 returns the uint64 representation of b[0:8].
uint64_t binary_LE_Uint64(binary_LE self, so_Slice b);

// PutUint64 stores v into b[0:8].
void binary_LE_PutUint64(binary_LE self, so_Slice b, uint64_t v);

// AppendUint64 appends the bytes of v to b and returns the appended slice.
// Requires at least 8 bytes of spare capacity in b.
so_Slice binary_LE_AppendUint64(binary_LE self, so_Slice b, uint64_t v);
so_String binary_LE_String(binary_LE self);

// Uint16 returns the uint16 representation of b[0:2].
uint16_t binary_BE_Uint16(binary_BE self, so_Slice b);

// PutUint16 stores v into b[0:2].
void binary_BE_PutUint16(binary_BE self, so_Slice b, uint16_t v);

// AppendUint16 appends the bytes of v to b and returns the appended slice.
// Requires at least 2 bytes of spare capacity in b.
so_Slice binary_BE_AppendUint16(binary_BE self, so_Slice b, uint16_t v);

// Uint32 returns the uint32 representation of b[0:4].
uint32_t binary_BE_Uint32(binary_BE self, so_Slice b);

// PutUint32 stores v into b[0:4].
void binary_BE_PutUint32(binary_BE self, so_Slice b, uint32_t v);

// AppendUint32 appends the bytes of v to b and returns the appended slice.
// Requires at least 4 bytes of spare capacity in b.
so_Slice binary_BE_AppendUint32(binary_BE self, so_Slice b, uint32_t v);

// Uint64 returns the uint64 representation of b[0:8].
uint64_t binary_BE_Uint64(binary_BE self, so_Slice b);

// PutUint64 stores v into b[0:8].
void binary_BE_PutUint64(binary_BE self, so_Slice b, uint64_t v);

// AppendUint64 appends the bytes of v to b and returns the appended slice.
// Requires at least 8 bytes of spare capacity in b.
so_Slice binary_BE_AppendUint64(binary_BE self, so_Slice b, uint64_t v);
so_String binary_BE_String(binary_BE self);
