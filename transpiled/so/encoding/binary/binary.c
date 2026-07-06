#include "binary.h"

// -- Variables and constants --

// LittleEndian is the little-endian implementation of [ByteOrder] and [AppendByteOrder].
binary_LE binary_LittleEndian = {0};

// BigEndian is the big-endian implementation of [ByteOrder] and [AppendByteOrder].
binary_BE binary_BigEndian = {0};

// -- Implementation --

// Uint16 returns the uint16 representation of b[0:2].
uint16_t binary_LE_Uint16(binary_LE self, so_Slice b) {
    (void)self;
    // bounds check hint to compiler; see golang.org/issue/14808
    (void)so_at(so_byte, b, 1);
    return ((uint16_t)(so_at(so_byte, b, 0)) | ((uint16_t)(so_at(so_byte, b, 1)) << 8));
}

// PutUint16 stores v into b[0:2].
void binary_LE_PutUint16(binary_LE self, so_Slice b, uint16_t v) {
    (void)self;
    // early bounds check to guarantee safety of writes below
    (void)so_at(so_byte, b, 1);
    so_at(so_byte, b, 0) = (so_byte)(v);
    so_at(so_byte, b, 1) = (so_byte)(v >> 8);
}

// AppendUint16 appends the bytes of v to b and returns the appended slice.
// Requires at least 2 bytes of spare capacity in b.
so_Slice binary_LE_AppendUint16(binary_LE self, so_Slice b, uint16_t v) {
    (void)self;
    return so_append(so_byte, b, (so_byte)(v), (so_byte)(v >> 8));
}

// Uint32 returns the uint32 representation of b[0:4].
uint32_t binary_LE_Uint32(binary_LE self, so_Slice b) {
    (void)self;
    // bounds check hint to compiler; see golang.org/issue/14808
    (void)so_at(so_byte, b, 3);
    return ((((uint32_t)(so_at(so_byte, b, 0)) | ((uint32_t)(so_at(so_byte, b, 1)) << 8)) | ((uint32_t)(so_at(so_byte, b, 2)) << 16)) | ((uint32_t)(so_at(so_byte, b, 3)) << 24));
}

// PutUint32 stores v into b[0:4].
void binary_LE_PutUint32(binary_LE self, so_Slice b, uint32_t v) {
    (void)self;
    // early bounds check to guarantee safety of writes below
    (void)so_at(so_byte, b, 3);
    so_at(so_byte, b, 0) = (so_byte)(v);
    so_at(so_byte, b, 1) = (so_byte)(v >> 8);
    so_at(so_byte, b, 2) = (so_byte)(v >> 16);
    so_at(so_byte, b, 3) = (so_byte)(v >> 24);
}

// AppendUint32 appends the bytes of v to b and returns the appended slice.
// Requires at least 4 bytes of spare capacity in b.
so_Slice binary_LE_AppendUint32(binary_LE self, so_Slice b, uint32_t v) {
    (void)self;
    return so_append(so_byte, b, (so_byte)(v), (so_byte)(v >> 8), (so_byte)(v >> 16), (so_byte)(v >> 24));
}

// Uint64 returns the uint64 representation of b[0:8].
uint64_t binary_LE_Uint64(binary_LE self, so_Slice b) {
    (void)self;
    // bounds check hint to compiler; see golang.org/issue/14808
    (void)so_at(so_byte, b, 7);
    return ((((((((uint64_t)(so_at(so_byte, b, 0)) | ((uint64_t)(so_at(so_byte, b, 1)) << 8)) | ((uint64_t)(so_at(so_byte, b, 2)) << 16)) | ((uint64_t)(so_at(so_byte, b, 3)) << 24)) | ((uint64_t)(so_at(so_byte, b, 4)) << 32)) | ((uint64_t)(so_at(so_byte, b, 5)) << 40)) | ((uint64_t)(so_at(so_byte, b, 6)) << 48)) | ((uint64_t)(so_at(so_byte, b, 7)) << 56));
}

// PutUint64 stores v into b[0:8].
void binary_LE_PutUint64(binary_LE self, so_Slice b, uint64_t v) {
    (void)self;
    // early bounds check to guarantee safety of writes below
    (void)so_at(so_byte, b, 7);
    so_at(so_byte, b, 0) = (so_byte)(v);
    so_at(so_byte, b, 1) = (so_byte)(v >> 8);
    so_at(so_byte, b, 2) = (so_byte)(v >> 16);
    so_at(so_byte, b, 3) = (so_byte)(v >> 24);
    so_at(so_byte, b, 4) = (so_byte)(v >> 32);
    so_at(so_byte, b, 5) = (so_byte)(v >> 40);
    so_at(so_byte, b, 6) = (so_byte)(v >> 48);
    so_at(so_byte, b, 7) = (so_byte)(v >> 56);
}

// AppendUint64 appends the bytes of v to b and returns the appended slice.
// Requires at least 8 bytes of spare capacity in b.
so_Slice binary_LE_AppendUint64(binary_LE self, so_Slice b, uint64_t v) {
    (void)self;
    return so_append(so_byte, b, (so_byte)(v), (so_byte)(v >> 8), (so_byte)(v >> 16), (so_byte)(v >> 24), (so_byte)(v >> 32), (so_byte)(v >> 40), (so_byte)(v >> 48), (so_byte)(v >> 56));
}

so_String binary_LE_String(binary_LE self) {
    (void)self;
    return so_str("LittleEndian");
}

// Uint16 returns the uint16 representation of b[0:2].
uint16_t binary_BE_Uint16(binary_BE self, so_Slice b) {
    (void)self;
    // bounds check hint to compiler; see golang.org/issue/14808
    (void)so_at(so_byte, b, 1);
    return ((uint16_t)(so_at(so_byte, b, 1)) | ((uint16_t)(so_at(so_byte, b, 0)) << 8));
}

// PutUint16 stores v into b[0:2].
void binary_BE_PutUint16(binary_BE self, so_Slice b, uint16_t v) {
    (void)self;
    // early bounds check to guarantee safety of writes below
    (void)so_at(so_byte, b, 1);
    so_at(so_byte, b, 0) = (so_byte)(v >> 8);
    so_at(so_byte, b, 1) = (so_byte)(v);
}

// AppendUint16 appends the bytes of v to b and returns the appended slice.
// Requires at least 2 bytes of spare capacity in b.
so_Slice binary_BE_AppendUint16(binary_BE self, so_Slice b, uint16_t v) {
    (void)self;
    return so_append(so_byte, b, (so_byte)(v >> 8), (so_byte)(v));
}

// Uint32 returns the uint32 representation of b[0:4].
uint32_t binary_BE_Uint32(binary_BE self, so_Slice b) {
    (void)self;
    // bounds check hint to compiler; see golang.org/issue/14808
    (void)so_at(so_byte, b, 3);
    return ((((uint32_t)(so_at(so_byte, b, 3)) | ((uint32_t)(so_at(so_byte, b, 2)) << 8)) | ((uint32_t)(so_at(so_byte, b, 1)) << 16)) | ((uint32_t)(so_at(so_byte, b, 0)) << 24));
}

// PutUint32 stores v into b[0:4].
void binary_BE_PutUint32(binary_BE self, so_Slice b, uint32_t v) {
    (void)self;
    // early bounds check to guarantee safety of writes below
    (void)so_at(so_byte, b, 3);
    so_at(so_byte, b, 0) = (so_byte)(v >> 24);
    so_at(so_byte, b, 1) = (so_byte)(v >> 16);
    so_at(so_byte, b, 2) = (so_byte)(v >> 8);
    so_at(so_byte, b, 3) = (so_byte)(v);
}

// AppendUint32 appends the bytes of v to b and returns the appended slice.
// Requires at least 4 bytes of spare capacity in b.
so_Slice binary_BE_AppendUint32(binary_BE self, so_Slice b, uint32_t v) {
    (void)self;
    return so_append(so_byte, b, (so_byte)(v >> 24), (so_byte)(v >> 16), (so_byte)(v >> 8), (so_byte)(v));
}

// Uint64 returns the uint64 representation of b[0:8].
uint64_t binary_BE_Uint64(binary_BE self, so_Slice b) {
    (void)self;
    // bounds check hint to compiler; see golang.org/issue/14808
    (void)so_at(so_byte, b, 7);
    return ((((((((uint64_t)(so_at(so_byte, b, 7)) | ((uint64_t)(so_at(so_byte, b, 6)) << 8)) | ((uint64_t)(so_at(so_byte, b, 5)) << 16)) | ((uint64_t)(so_at(so_byte, b, 4)) << 24)) | ((uint64_t)(so_at(so_byte, b, 3)) << 32)) | ((uint64_t)(so_at(so_byte, b, 2)) << 40)) | ((uint64_t)(so_at(so_byte, b, 1)) << 48)) | ((uint64_t)(so_at(so_byte, b, 0)) << 56));
}

// PutUint64 stores v into b[0:8].
void binary_BE_PutUint64(binary_BE self, so_Slice b, uint64_t v) {
    (void)self;
    // early bounds check to guarantee safety of writes below
    (void)so_at(so_byte, b, 7);
    so_at(so_byte, b, 0) = (so_byte)(v >> 56);
    so_at(so_byte, b, 1) = (so_byte)(v >> 48);
    so_at(so_byte, b, 2) = (so_byte)(v >> 40);
    so_at(so_byte, b, 3) = (so_byte)(v >> 32);
    so_at(so_byte, b, 4) = (so_byte)(v >> 24);
    so_at(so_byte, b, 5) = (so_byte)(v >> 16);
    so_at(so_byte, b, 6) = (so_byte)(v >> 8);
    so_at(so_byte, b, 7) = (so_byte)(v);
}

// AppendUint64 appends the bytes of v to b and returns the appended slice.
// Requires at least 8 bytes of spare capacity in b.
so_Slice binary_BE_AppendUint64(binary_BE self, so_Slice b, uint64_t v) {
    (void)self;
    return so_append(so_byte, b, (so_byte)(v >> 56), (so_byte)(v >> 48), (so_byte)(v >> 40), (so_byte)(v >> 32), (so_byte)(v >> 24), (so_byte)(v >> 16), (so_byte)(v >> 8), (so_byte)(v));
}

so_String binary_BE_String(binary_BE self) {
    (void)self;
    return so_str("BigEndian");
}
