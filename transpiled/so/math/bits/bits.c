#include "bits.h"

// -- Variables and constants --
static const so_String ntz8tab = so_str("" "\x08\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00" "\x04\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00" "\x05\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00" "\x04\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00" "\x06\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00" "\x04\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00" "\x05\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00" "\x04\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00" "\x07\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00" "\x04\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00" "\x05\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00" "\x04\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00" "\x06\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00" "\x04\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00" "\x05\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00" "\x04\x00\x01\x00\x02\x00\x01\x00\x03\x00\x01\x00\x02\x00\x01\x00");
static const so_String pop8tab = so_str("" "\x00\x01\x01\x02\x01\x02\x02\x03\x01\x02\x02\x03\x02\x03\x03\x04" "\x01\x02\x02\x03\x02\x03\x03\x04\x02\x03\x03\x04\x03\x04\x04\x05" "\x01\x02\x02\x03\x02\x03\x03\x04\x02\x03\x03\x04\x03\x04\x04\x05" "\x02\x03\x03\x04\x03\x04\x04\x05\x03\x04\x04\x05\x04\x05\x05\x06" "\x01\x02\x02\x03\x02\x03\x03\x04\x02\x03\x03\x04\x03\x04\x04\x05" "\x02\x03\x03\x04\x03\x04\x04\x05\x03\x04\x04\x05\x04\x05\x05\x06" "\x02\x03\x03\x04\x03\x04\x04\x05\x03\x04\x04\x05\x04\x05\x05\x06" "\x03\x04\x04\x05\x04\x05\x05\x06\x04\x05\x05\x06\x05\x06\x06\x07" "\x01\x02\x02\x03\x02\x03\x03\x04\x02\x03\x03\x04\x03\x04\x04\x05" "\x02\x03\x03\x04\x03\x04\x04\x05\x03\x04\x04\x05\x04\x05\x05\x06" "\x02\x03\x03\x04\x03\x04\x04\x05\x03\x04\x04\x05\x04\x05\x05\x06" "\x03\x04\x04\x05\x04\x05\x05\x06\x04\x05\x05\x06\x05\x06\x06\x07" "\x02\x03\x03\x04\x03\x04\x04\x05\x03\x04\x04\x05\x04\x05\x05\x06" "\x03\x04\x04\x05\x04\x05\x05\x06\x04\x05\x05\x06\x05\x06\x06\x07" "\x03\x04\x04\x05\x04\x05\x05\x06\x04\x05\x05\x06\x05\x06\x06\x07" "\x04\x05\x05\x06\x05\x06\x06\x07\x05\x06\x06\x07\x06\x07\x07\x08");
static const so_String rev8tab = so_str("" "\x00\x80\x40\xc0\x20\xa0\x60\xe0\x10\x90\x50\xd0\x30\xb0\x70\xf0" "\x08\x88\x48\xc8\x28\xa8\x68\xe8\x18\x98\x58\xd8\x38\xb8\x78\xf8" "\x04\x84\x44\xc4\x24\xa4\x64\xe4\x14\x94\x54\xd4\x34\xb4\x74\xf4" "\x0c\x8c\x4c\xcc\x2c\xac\x6c\xec\x1c\x9c\x5c\xdc\x3c\xbc\x7c\xfc" "\x02\x82\x42\xc2\x22\xa2\x62\xe2\x12\x92\x52\xd2\x32\xb2\x72\xf2" "\x0a\x8a\x4a\xca\x2a\xaa\x6a\xea\x1a\x9a\x5a\xda\x3a\xba\x7a\xfa" "\x06\x86\x46\xc6\x26\xa6\x66\xe6\x16\x96\x56\xd6\x36\xb6\x76\xf6" "\x0e\x8e\x4e\xce\x2e\xae\x6e\xee\x1e\x9e\x5e\xde\x3e\xbe\x7e\xfe" "\x01\x81\x41\xc1\x21\xa1\x61\xe1\x11\x91\x51\xd1\x31\xb1\x71\xf1" "\x09\x89\x49\xc9\x29\xa9\x69\xe9\x19\x99\x59\xd9\x39\xb9\x79\xf9" "\x05\x85\x45\xc5\x25\xa5\x65\xe5\x15\x95\x55\xd5\x35\xb5\x75\xf5" "\x0d\x8d\x4d\xcd\x2d\xad\x6d\xed\x1d\x9d\x5d\xdd\x3d\xbd\x7d\xfd" "\x03\x83\x43\xc3\x23\xa3\x63\xe3\x13\x93\x53\xd3\x33\xb3\x73\xf3" "\x0b\x8b\x4b\xcb\x2b\xab\x6b\xeb\x1b\x9b\x5b\xdb\x3b\xbb\x7b\xfb" "\x07\x87\x47\xc7\x27\xa7\x67\xe7\x17\x97\x57\xd7\x37\xb7\x77\xf7" "\x0f\x8f\x4f\xcf\x2f\xaf\x6f\xef\x1f\x9f\x5f\xdf\x3f\xbf\x7f\xff");
static const so_String len8tab = so_str("" "\x00\x01\x02\x02\x03\x03\x03\x03\x04\x04\x04\x04\x04\x04\x04\x04" "\x05\x05\x05\x05\x05\x05\x05\x05\x05\x05\x05\x05\x05\x05\x05\x05" "\x06\x06\x06\x06\x06\x06\x06\x06\x06\x06\x06\x06\x06\x06\x06\x06" "\x06\x06\x06\x06\x06\x06\x06\x06\x06\x06\x06\x06\x06\x06\x06\x06" "\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07" "\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07" "\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07" "\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07\x07" "\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08" "\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08" "\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08" "\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08" "\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08" "\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08" "\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08" "\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08");

// --- TrailingZeros ---
// See http://keithandkatie.com/keith/papers/debruijn.html
static const int64_t deBruijn32 = 0x077CB531;
static so_byte deBruijn32tab[32] = {0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9};
static const int64_t deBruijn64 = 0x03f79d71b4ca8b09;
static so_byte deBruijn64tab[64] = {0, 1, 56, 2, 57, 49, 28, 3, 61, 58, 42, 50, 38, 29, 17, 4, 62, 47, 59, 36, 45, 43, 51, 22, 53, 39, 33, 30, 24, 18, 12, 5, 63, 55, 48, 27, 60, 41, 37, 16, 46, 35, 44, 21, 52, 32, 23, 11, 54, 26, 40, 15, 34, 20, 31, 10, 25, 14, 19, 9, 13, 8, 7, 6};

// --- OnesCount ---
// 01010101 ...
static const int64_t m0 = 0x5555555555555555;

// 00110011 ...
static const int64_t m1 = 0x3333333333333333;

// 00001111 ...
static const int64_t m2 = 0x0f0f0f0f0f0f0f0f;

// etc.
static const int64_t m3 = 0x00ff00ff00ff00ff;
static const int64_t m4 = 0x0000ffff0000ffff;

// -- atables.go --

// -- bits.go --

// --- LeadingZeros ---
// LeadingZeros returns the number of leading zero bits in x; the result is [UintSize] for x == 0.
so_int bits_LeadingZeros(so_uint x) {
    return bits_UintSize - bits_Len(x);
}

// LeadingZeros8 returns the number of leading zero bits in x; the result is 8 for x == 0.
so_int bits_LeadingZeros8(uint8_t x) {
    return 8 - bits_Len8(x);
}

// LeadingZeros16 returns the number of leading zero bits in x; the result is 16 for x == 0.
so_int bits_LeadingZeros16(uint16_t x) {
    return 16 - bits_Len16(x);
}

// LeadingZeros32 returns the number of leading zero bits in x; the result is 32 for x == 0.
so_int bits_LeadingZeros32(uint32_t x) {
    return 32 - bits_Len32(x);
}

// LeadingZeros64 returns the number of leading zero bits in x; the result is 64 for x == 0.
so_int bits_LeadingZeros64(uint64_t x) {
    return 64 - bits_Len64(x);
}

// TrailingZeros returns the number of trailing zero bits in x; the result is [UintSize] for x == 0.
so_int bits_TrailingZeros(so_uint x) {
    if (bits_UintSize == 32) {
        return bits_TrailingZeros32((uint32_t)(x));
    }
    return bits_TrailingZeros64((uint64_t)(x));
}

// TrailingZeros8 returns the number of trailing zero bits in x; the result is 8 for x == 0.
so_int bits_TrailingZeros8(uint8_t x) {
    return (so_int)(so_at(so_byte, ntz8tab, x));
}

// TrailingZeros16 returns the number of trailing zero bits in x; the result is 16 for x == 0.
so_int bits_TrailingZeros16(uint16_t x) {
    if (x == 0) {
        return 16;
    }
    // see comment in TrailingZeros64
    return (so_int)(deBruijn32tab[((uint32_t)(x & -x) * deBruijn32 >> (32 - 5))]);
}

// TrailingZeros32 returns the number of trailing zero bits in x; the result is 32 for x == 0.
so_int bits_TrailingZeros32(uint32_t x) {
    if (x == 0) {
        return 32;
    }
    // see comment in TrailingZeros64
    return (so_int)(deBruijn32tab[((x & -x) * deBruijn32 >> (32 - 5))]);
}

// TrailingZeros64 returns the number of trailing zero bits in x; the result is 64 for x == 0.
so_int bits_TrailingZeros64(uint64_t x) {
    if (x == 0) {
        return 64;
    }
    // If popcount is fast, replace code below with return popcount(^x & (x - 1)).
    //
    // x & -x leaves only the right-most bit set in the word. Let k be the
    // index of that bit. Since only a single bit is set, the value is two
    // to the power of k. Multiplying by a power of two is equivalent to
    // left shifting, in this case by k bits. The de Bruijn (64 bit) constant
    // is such that all six bit, consecutive substrings are distinct.
    // Therefore, if we have a left shifted version of this constant we can
    // find by how many bits it was shifted by looking at which six bit
    // substring ended up at the top of the word.
    // (Knuth, volume 4, section 7.3.1)
    return (so_int)(deBruijn64tab[((x & -x) * deBruijn64 >> (64 - 6))]);
}

// OnesCount returns the number of one bits ("population count") in x.
so_int bits_OnesCount(so_uint x) {
    if (bits_UintSize == 32) {
        return bits_OnesCount32((uint32_t)(x));
    }
    return bits_OnesCount64((uint64_t)(x));
}

// OnesCount8 returns the number of one bits ("population count") in x.
so_int bits_OnesCount8(uint8_t x) {
    return (so_int)(so_at(so_byte, pop8tab, x));
}

// OnesCount16 returns the number of one bits ("population count") in x.
so_int bits_OnesCount16(uint16_t x) {
    return (so_int)(so_at(so_byte, pop8tab, (x >> 8)) + so_at(so_byte, pop8tab, (x & 0xff)));
}

// OnesCount32 returns the number of one bits ("population count") in x.
so_int bits_OnesCount32(uint32_t x) {
    return (so_int)(so_at(so_byte, pop8tab, (x >> 24)) + so_at(so_byte, pop8tab, ((x >> 16) & 0xff)) + so_at(so_byte, pop8tab, ((x >> 8) & 0xff)) + so_at(so_byte, pop8tab, (x & 0xff)));
}

// OnesCount64 returns the number of one bits ("population count") in x.
so_int bits_OnesCount64(uint64_t x) {
    // Implementation: Parallel summing of adjacent bits.
    // See "Hacker's Delight", Chap. 5: Counting Bits.
    // The following pattern shows the general approach:
    //
    //   x = x>>1&(m0&m) + x&(m0&m)
    //   x = x>>2&(m1&m) + x&(m1&m)
    //   x = x>>4&(m2&m) + x&(m2&m)
    //   x = x>>8&(m3&m) + x&(m3&m)
    //   x = x>>16&(m4&m) + x&(m4&m)
    //   x = x>>32&(m5&m) + x&(m5&m)
    //   return int(x)
    //
    // Masking (& operations) can be left away when there's no
    // danger that a field's sum will carry over into the next
    // field: Since the result cannot be > 64, 8 bits is enough
    // and we can ignore the masks for the shifts by 8 and up.
    // Per "Hacker's Delight", the first line can be simplified
    // more, but it saves at best one instruction, so we leave
    // it alone for clarity.
    // all bits set; 1<<64 - 1 would overflow in C
    const int64_t m = -1;
    x = ((x >> 1) & (m0 & m)) + (x & (m0 & m));
    x = ((x >> 2) & (m1 & m)) + (x & (m1 & m));
    x = (((x >> 4) + x) & (m2 & m));
    x += (x >> 8);
    x += (x >> 16);
    x += (x >> 32);
    return ((so_int)(x) & (((int64_t)1 << 7) - 1));
}

// --- RotateLeft ---
// RotateLeft returns the value of x rotated left by (k mod [UintSize]) bits.
// To rotate x right by k bits, call RotateLeft(x, -k).
//
// This function's execution time does not depend on the inputs.
so_uint bits_RotateLeft(so_uint x, so_int k) {
    if (bits_UintSize == 32) {
        return (so_uint)(bits_RotateLeft32((uint32_t)(x), k));
    }
    return (so_uint)(bits_RotateLeft64((uint64_t)(x), k));
}

// RotateLeft8 returns the value of x rotated left by (k mod 8) bits.
// To rotate x right by k bits, call RotateLeft8(x, -k).
//
// This function's execution time does not depend on the inputs.
uint8_t bits_RotateLeft8(uint8_t x, so_int k) {
    const int64_t n = 8;
    so_uint s = ((so_uint)(k) & (n - 1));
    return ((x << s) | (x >> (n - s)));
}

// RotateLeft16 returns the value of x rotated left by (k mod 16) bits.
// To rotate x right by k bits, call RotateLeft16(x, -k).
//
// This function's execution time does not depend on the inputs.
uint16_t bits_RotateLeft16(uint16_t x, so_int k) {
    const int64_t n = 16;
    so_uint s = ((so_uint)(k) & (n - 1));
    return ((x << s) | (x >> (n - s)));
}

// RotateLeft32 returns the value of x rotated left by (k mod 32) bits.
// To rotate x right by k bits, call RotateLeft32(x, -k).
//
// This function's execution time does not depend on the inputs.
uint32_t bits_RotateLeft32(uint32_t x, so_int k) {
    const int64_t n = 32;
    so_uint s = ((so_uint)(k) & (n - 1));
    return ((x << s) | (x >> (n - s)));
}

// RotateLeft64 returns the value of x rotated left by (k mod 64) bits.
// To rotate x right by k bits, call RotateLeft64(x, -k).
//
// This function's execution time does not depend on the inputs.
uint64_t bits_RotateLeft64(uint64_t x, so_int k) {
    const int64_t n = 64;
    so_uint s = ((so_uint)(k) & (n - 1));
    return ((x << s) | (x >> (n - s)));
}

// --- Reverse ---
// Reverse returns the value of x with its bits in reversed order.
so_uint bits_Reverse(so_uint x) {
    if (bits_UintSize == 32) {
        return (so_uint)(bits_Reverse32((uint32_t)(x)));
    }
    return (so_uint)(bits_Reverse64((uint64_t)(x)));
}

// Reverse8 returns the value of x with its bits in reversed order.
uint8_t bits_Reverse8(uint8_t x) {
    return so_at(so_byte, rev8tab, x);
}

// Reverse16 returns the value of x with its bits in reversed order.
uint16_t bits_Reverse16(uint16_t x) {
    return ((uint16_t)(so_at(so_byte, rev8tab, (x >> 8))) | ((uint16_t)(so_at(so_byte, rev8tab, (x & 0xff))) << 8));
}

// Reverse32 returns the value of x with its bits in reversed order.
uint32_t bits_Reverse32(uint32_t x) {
    const int64_t m = ((int64_t)1 << 32) - 1;
    x = (((x >> 1) & (m0 & m)) | ((x & (m0 & m)) << 1));
    x = (((x >> 2) & (m1 & m)) | ((x & (m1 & m)) << 2));
    x = (((x >> 4) & (m2 & m)) | ((x & (m2 & m)) << 4));
    return bits_ReverseBytes32(x);
}

// Reverse64 returns the value of x with its bits in reversed order.
uint64_t bits_Reverse64(uint64_t x) {
    // all bits set; 1<<64 - 1 would overflow in C
    const int64_t m = -1;
    x = (((x >> 1) & (m0 & m)) | ((x & (m0 & m)) << 1));
    x = (((x >> 2) & (m1 & m)) | ((x & (m1 & m)) << 2));
    x = (((x >> 4) & (m2 & m)) | ((x & (m2 & m)) << 4));
    return bits_ReverseBytes64(x);
}

// --- ReverseBytes ---
// ReverseBytes returns the value of x with its bytes in reversed order.
//
// This function's execution time does not depend on the inputs.
so_uint bits_ReverseBytes(so_uint x) {
    if (bits_UintSize == 32) {
        return (so_uint)(bits_ReverseBytes32((uint32_t)(x)));
    }
    return (so_uint)(bits_ReverseBytes64((uint64_t)(x)));
}

// ReverseBytes16 returns the value of x with its bytes in reversed order.
//
// This function's execution time does not depend on the inputs.
uint16_t bits_ReverseBytes16(uint16_t x) {
    return ((x >> 8) | (x << 8));
}

// ReverseBytes32 returns the value of x with its bytes in reversed order.
//
// This function's execution time does not depend on the inputs.
uint32_t bits_ReverseBytes32(uint32_t x) {
    const int64_t m = ((int64_t)1 << 32) - 1;
    x = (((x >> 8) & (m3 & m)) | ((x & (m3 & m)) << 8));
    return ((x >> 16) | (x << 16));
}

// ReverseBytes64 returns the value of x with its bytes in reversed order.
//
// This function's execution time does not depend on the inputs.
uint64_t bits_ReverseBytes64(uint64_t x) {
    // all bits set; 1<<64 - 1 would overflow in C
    const int64_t m = -1;
    x = (((x >> 8) & (m3 & m)) | ((x & (m3 & m)) << 8));
    x = (((x >> 16) & (m4 & m)) | ((x & (m4 & m)) << 16));
    return ((x >> 32) | (x << 32));
}

// --- Len ---
// Len returns the minimum number of bits required to represent x; the result is 0 for x == 0.
so_int bits_Len(so_uint x) {
    if (bits_UintSize == 32) {
        return bits_Len32((uint32_t)(x));
    }
    return bits_Len64((uint64_t)(x));
}

// Len8 returns the minimum number of bits required to represent x; the result is 0 for x == 0.
so_int bits_Len8(uint8_t x) {
    return (so_int)(so_at(so_byte, len8tab, x));
}

// Len16 returns the minimum number of bits required to represent x; the result is 0 for x == 0.
so_int bits_Len16(uint16_t x) {
    so_int n = 0;
    if (x >= ((uint16_t)1 << 8)) {
        x >>= 8;
        n = 8;
    }
    return n + (so_int)(so_at(so_byte, len8tab, (uint8_t)(x)));
}

// Len32 returns the minimum number of bits required to represent x; the result is 0 for x == 0.
so_int bits_Len32(uint32_t x) {
    so_int n = 0;
    if (x >= ((uint32_t)1 << 16)) {
        x >>= 16;
        n = 16;
    }
    if (x >= ((uint32_t)1 << 8)) {
        x >>= 8;
        n += 8;
    }
    return n + (so_int)(so_at(so_byte, len8tab, (uint8_t)(x)));
}

// Len64 returns the minimum number of bits required to represent x; the result is 0 for x == 0.
so_int bits_Len64(uint64_t x) {
    so_int n = 0;
    if (x >= ((uint64_t)1 << 32)) {
        x >>= 32;
        n = 32;
    }
    if (x >= ((uint64_t)1 << 16)) {
        x >>= 16;
        n += 16;
    }
    if (x >= ((uint64_t)1 << 8)) {
        x >>= 8;
        n += 8;
    }
    return n + (so_int)(so_at(so_byte, len8tab, (uint8_t)(x)));
}

// --- Add with carry ---
// Add returns the sum with carry of x, y and carry: sum = x + y + carry.
// The carry input must be 0 or 1; otherwise the behavior is undefined.
// The carryOut output is guaranteed to be 0 or 1.
//
// This function's execution time does not depend on the inputs.
so_R_uint_uint bits_Add(so_uint x, so_uint y, so_uint carry) {
    if (bits_UintSize == 32) {
        so_R_u32_u32 _res1 = bits_Add32((uint32_t)(x), (uint32_t)(y), (uint32_t)(carry));
        uint32_t s32 = _res1.val;
        uint32_t c32 = _res1.val2;
        return (so_R_uint_uint){.val = (so_uint)(s32), .val2 = (so_uint)(c32)};
    }
    so_R_u64_u64 _res2 = bits_Add64((uint64_t)(x), (uint64_t)(y), (uint64_t)(carry));
    uint64_t s64 = _res2.val;
    uint64_t c64 = _res2.val2;
    return (so_R_uint_uint){.val = (so_uint)(s64), .val2 = (so_uint)(c64)};
}

// Add32 returns the sum with carry of x, y and carry: sum = x + y + carry.
// The carry input must be 0 or 1; otherwise the behavior is undefined.
// The carryOut output is guaranteed to be 0 or 1.
//
// This function's execution time does not depend on the inputs.
so_R_u32_u32 bits_Add32(uint32_t x, uint32_t y, uint32_t carry) {
    uint64_t sum64 = (uint64_t)(x) + (uint64_t)(y) + (uint64_t)(carry);
    uint32_t sum = (uint32_t)(sum64);
    uint32_t carryOut = (uint32_t)(sum64 >> 32);
    return (so_R_u32_u32){.val = sum, .val2 = carryOut};
}

// Add64 returns the sum with carry of x, y and carry: sum = x + y + carry.
// The carry input must be 0 or 1; otherwise the behavior is undefined.
// The carryOut output is guaranteed to be 0 or 1.
//
// This function's execution time does not depend on the inputs.
so_R_u64_u64 bits_Add64(uint64_t x, uint64_t y, uint64_t carry) {
    uint64_t sum = x + y + carry;
    // The sum will overflow if both top bits are set (x & y) or if one of them
    // is (x | y), and a carry from the lower place happened. If such a carry
    // happens, the top bit will be 1 + 0 + 1 = 0 (&^ sum).
    uint64_t carryOut = (((x & y) | ((x | y) & ~sum)) >> 63);
    return (so_R_u64_u64){.val = sum, .val2 = carryOut};
}

// --- Subtract with borrow ---
// Sub returns the difference of x, y and borrow: diff = x - y - borrow.
// The borrow input must be 0 or 1; otherwise the behavior is undefined.
// The borrowOut output is guaranteed to be 0 or 1.
//
// This function's execution time does not depend on the inputs.
so_R_uint_uint bits_Sub(so_uint x, so_uint y, so_uint borrow) {
    if (bits_UintSize == 32) {
        so_R_u32_u32 _res1 = bits_Sub32((uint32_t)(x), (uint32_t)(y), (uint32_t)(borrow));
        uint32_t d32 = _res1.val;
        uint32_t b32 = _res1.val2;
        return (so_R_uint_uint){.val = (so_uint)(d32), .val2 = (so_uint)(b32)};
    }
    so_R_u64_u64 _res2 = bits_Sub64((uint64_t)(x), (uint64_t)(y), (uint64_t)(borrow));
    uint64_t d64 = _res2.val;
    uint64_t b64 = _res2.val2;
    return (so_R_uint_uint){.val = (so_uint)(d64), .val2 = (so_uint)(b64)};
}

// Sub32 returns the difference of x, y and borrow, diff = x - y - borrow.
// The borrow input must be 0 or 1; otherwise the behavior is undefined.
// The borrowOut output is guaranteed to be 0 or 1.
//
// This function's execution time does not depend on the inputs.
so_R_u32_u32 bits_Sub32(uint32_t x, uint32_t y, uint32_t borrow) {
    uint32_t diff = x - y - borrow;
    // The difference will underflow if the top bit of x is not set and the top
    // bit of y is set (^x & y) or if they are the same (^(x ^ y)) and a borrow
    // from the lower place happens. If that borrow happens, the result will be
    // 1 - 1 - 1 = 0 - 0 - 1 = 1 (& diff).
    uint32_t borrowOut = (((~x & y) | (~(x ^ y) & diff)) >> 31);
    return (so_R_u32_u32){.val = diff, .val2 = borrowOut};
}

// Sub64 returns the difference of x, y and borrow: diff = x - y - borrow.
// The borrow input must be 0 or 1; otherwise the behavior is undefined.
// The borrowOut output is guaranteed to be 0 or 1.
//
// This function's execution time does not depend on the inputs.
so_R_u64_u64 bits_Sub64(uint64_t x, uint64_t y, uint64_t borrow) {
    uint64_t diff = x - y - borrow;
    // See Sub32 for the bit logic.
    uint64_t borrowOut = (((~x & y) | (~(x ^ y) & diff)) >> 63);
    return (so_R_u64_u64){.val = diff, .val2 = borrowOut};
}

// --- Full-width multiply ---
// Mul returns the full-width product of x and y: (hi, lo) = x * y
// with the product bits' upper half returned in hi and the lower
// half returned in lo.
//
// This function's execution time does not depend on the inputs.
so_R_uint_uint bits_Mul(so_uint x, so_uint y) {
    if (bits_UintSize == 32) {
        so_R_u32_u32 _res1 = bits_Mul32((uint32_t)(x), (uint32_t)(y));
        uint32_t h = _res1.val;
        uint32_t l = _res1.val2;
        return (so_R_uint_uint){.val = (so_uint)(h), .val2 = (so_uint)(l)};
    }
    so_R_u64_u64 _res2 = bits_Mul64((uint64_t)(x), (uint64_t)(y));
    uint64_t h = _res2.val;
    uint64_t l = _res2.val2;
    return (so_R_uint_uint){.val = (so_uint)(h), .val2 = (so_uint)(l)};
}

// Mul32 returns the 64-bit product of x and y: (hi, lo) = x * y
// with the product bits' upper half returned in hi and the lower
// half returned in lo.
//
// This function's execution time does not depend on the inputs.
so_R_u32_u32 bits_Mul32(uint32_t x, uint32_t y) {
    uint64_t tmp = (uint64_t)(x) * (uint64_t)(y);
    uint32_t hi = (uint32_t)(tmp >> 32), lo = (uint32_t)(tmp);
    return (so_R_u32_u32){.val = hi, .val2 = lo};
}

// Mul64 returns the 128-bit product of x and y: (hi, lo) = x * y
// with the product bits' upper half returned in hi and the lower
// half returned in lo.
//
// This function's execution time does not depend on the inputs.
so_R_u64_u64 bits_Mul64(uint64_t x, uint64_t y) {
    const int64_t mask32 = ((int64_t)1 << 32) - 1;
    uint64_t x0 = (x & mask32);
    uint64_t x1 = (x >> 32);
    uint64_t y0 = (y & mask32);
    uint64_t y1 = (y >> 32);
    uint64_t w0 = x0 * y0;
    uint64_t t = x1 * y0 + (w0 >> 32);
    uint64_t w1 = (t & mask32);
    uint64_t w2 = (t >> 32);
    w1 += x0 * y1;
    uint64_t hi = x1 * y1 + w2 + (w1 >> 32);
    uint64_t lo = x * y;
    return (so_R_u64_u64){.val = hi, .val2 = lo};
}

// --- Full-width divide ---
// Div returns the quotient and remainder of (hi, lo) divided by y:
// quo = (hi, lo)/y, rem = (hi, lo)%y with the dividend bits' upper
// half in parameter hi and the lower half in parameter lo.
// Div panics for y == 0 (division by zero) or y <= hi (quotient overflow).
so_R_uint_uint bits_Div(so_uint hi, so_uint lo, so_uint y) {
    if (bits_UintSize == 32) {
        so_R_u32_u32 _res1 = bits_Div32((uint32_t)(hi), (uint32_t)(lo), (uint32_t)(y));
        uint32_t q = _res1.val;
        uint32_t r = _res1.val2;
        return (so_R_uint_uint){.val = (so_uint)(q), .val2 = (so_uint)(r)};
    }
    so_R_u64_u64 _res2 = bits_Div64((uint64_t)(hi), (uint64_t)(lo), (uint64_t)(y));
    uint64_t q = _res2.val;
    uint64_t r = _res2.val2;
    return (so_R_uint_uint){.val = (so_uint)(q), .val2 = (so_uint)(r)};
}

// Div32 returns the quotient and remainder of (hi, lo) divided by y:
// quo = (hi, lo)/y, rem = (hi, lo)%y with the dividend bits' upper
// half in parameter hi and the lower half in parameter lo.
// Div32 panics for y == 0 (division by zero) or y <= hi (quotient overflow).
so_R_u32_u32 bits_Div32(uint32_t hi, uint32_t lo, uint32_t y) {
    if (y != 0 && y <= hi) {
        so_panic("runtime error: integer overflow");
    }
    uint64_t z = (((uint64_t)(hi) << 32) | (uint64_t)(lo));
    uint32_t quo = (uint32_t)(z / (uint64_t)(y)), rem = (uint32_t)(z % (uint64_t)(y));
    return (so_R_u32_u32){.val = quo, .val2 = rem};
}

// Div64 returns the quotient and remainder of (hi, lo) divided by y:
// quo = (hi, lo)/y, rem = (hi, lo)%y with the dividend bits' upper
// half in parameter hi and the lower half in parameter lo.
// Div64 panics for y == 0 (division by zero) or y <= hi (quotient overflow).
so_R_u64_u64 bits_Div64(uint64_t hi, uint64_t lo, uint64_t y) {
    if (y == 0) {
        so_panic("runtime error: integer divide by zero");
    }
    if (y <= hi) {
        so_panic("runtime error: integer overflow");
    }
    // If high part is zero, we can directly return the results.
    if (hi == 0) {
        return (so_R_u64_u64){.val = lo / y, .val2 = lo % y};
    }
    so_uint s = (so_uint)(bits_LeadingZeros64(y));
    y <<= s;
    const int64_t two32 = ((int64_t)1 << 32);
    const int64_t mask32 = two32 - 1;
    uint64_t yn1 = (y >> 32);
    uint64_t yn0 = (y & mask32);
    uint64_t un32 = ((hi << s) | (lo >> (64 - s)));
    uint64_t un10 = (lo << s);
    uint64_t un1 = (un10 >> 32);
    uint64_t un0 = (un10 & mask32);
    uint64_t q1 = un32 / yn1;
    uint64_t rhat = un32 - q1 * yn1;
    for (; q1 >= two32 || q1 * yn0 > two32 * rhat + un1;) {
        q1--;
        rhat += yn1;
        if (rhat >= two32) {
            break;
        }
    }
    uint64_t un21 = un32 * two32 + un1 - q1 * y;
    uint64_t q0 = un21 / yn1;
    rhat = un21 - q0 * yn1;
    for (; q0 >= two32 || q0 * yn0 > two32 * rhat + un0;) {
        q0--;
        rhat += yn1;
        if (rhat >= two32) {
            break;
        }
    }
    return (so_R_u64_u64){.val = q1 * two32 + q0, .val2 = ((un21 * two32 + un0 - q0 * y) >> s)};
}

// Rem returns the remainder of (hi, lo) divided by y. Rem panics for
// y == 0 (division by zero) but, unlike Div, it doesn't panic on a
// quotient overflow.
so_uint bits_Rem(so_uint hi, so_uint lo, so_uint y) {
    if (bits_UintSize == 32) {
        return (so_uint)(bits_Rem32((uint32_t)(hi), (uint32_t)(lo), (uint32_t)(y)));
    }
    return (so_uint)(bits_Rem64((uint64_t)(hi), (uint64_t)(lo), (uint64_t)(y)));
}

// Rem32 returns the remainder of (hi, lo) divided by y. Rem32 panics
// for y == 0 (division by zero) but, unlike [Div32], it doesn't panic
// on a quotient overflow.
uint32_t bits_Rem32(uint32_t hi, uint32_t lo, uint32_t y) {
    return (uint32_t)((((uint64_t)(hi) << 32) | (uint64_t)(lo)) % (uint64_t)(y));
}

// Rem64 returns the remainder of (hi, lo) divided by y. Rem64 panics
// for y == 0 (division by zero) but, unlike [Div64], it doesn't panic
// on a quotient overflow.
uint64_t bits_Rem64(uint64_t hi, uint64_t lo, uint64_t y) {
    // We scale down hi so that hi < y, then use Div64 to compute the
    // rem with the guarantee that it won't panic on quotient overflow.
    // Given that
    //   hi ≡ hi%y    (mod y)
    // we have
    //   hi<<64 + lo ≡ (hi%y)<<64 + lo    (mod y)
    so_R_u64_u64 _res1 = bits_Div64(hi % y, lo, y);
    uint64_t rem = _res1.val2;
    return rem;
}
