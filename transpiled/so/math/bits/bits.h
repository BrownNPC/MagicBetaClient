#pragma once
#include "so/builtin/builtin.h"

// -- Variables and constants --

// UintSize is the size of a uint in bits.
// 32 or 64
static const int64_t bits_UintSize = ((int64_t)32 << ((uint64_t)(~(so_uint)(0)) >> 63));

// -- Functions and methods --

// --- LeadingZeros ---
// LeadingZeros returns the number of leading zero bits in x; the result is [UintSize] for x == 0.
so_int bits_LeadingZeros(so_uint x);

// LeadingZeros8 returns the number of leading zero bits in x; the result is 8 for x == 0.
so_int bits_LeadingZeros8(uint8_t x);

// LeadingZeros16 returns the number of leading zero bits in x; the result is 16 for x == 0.
so_int bits_LeadingZeros16(uint16_t x);

// LeadingZeros32 returns the number of leading zero bits in x; the result is 32 for x == 0.
so_int bits_LeadingZeros32(uint32_t x);

// LeadingZeros64 returns the number of leading zero bits in x; the result is 64 for x == 0.
so_int bits_LeadingZeros64(uint64_t x);

// TrailingZeros returns the number of trailing zero bits in x; the result is [UintSize] for x == 0.
so_int bits_TrailingZeros(so_uint x);

// TrailingZeros8 returns the number of trailing zero bits in x; the result is 8 for x == 0.
so_int bits_TrailingZeros8(uint8_t x);

// TrailingZeros16 returns the number of trailing zero bits in x; the result is 16 for x == 0.
so_int bits_TrailingZeros16(uint16_t x);

// TrailingZeros32 returns the number of trailing zero bits in x; the result is 32 for x == 0.
so_int bits_TrailingZeros32(uint32_t x);

// TrailingZeros64 returns the number of trailing zero bits in x; the result is 64 for x == 0.
so_int bits_TrailingZeros64(uint64_t x);

// OnesCount returns the number of one bits ("population count") in x.
so_int bits_OnesCount(so_uint x);

// OnesCount8 returns the number of one bits ("population count") in x.
so_int bits_OnesCount8(uint8_t x);

// OnesCount16 returns the number of one bits ("population count") in x.
so_int bits_OnesCount16(uint16_t x);

// OnesCount32 returns the number of one bits ("population count") in x.
so_int bits_OnesCount32(uint32_t x);

// OnesCount64 returns the number of one bits ("population count") in x.
so_int bits_OnesCount64(uint64_t x);

// --- RotateLeft ---
// RotateLeft returns the value of x rotated left by (k mod [UintSize]) bits.
// To rotate x right by k bits, call RotateLeft(x, -k).
//
// This function's execution time does not depend on the inputs.
so_uint bits_RotateLeft(so_uint x, so_int k);

// RotateLeft8 returns the value of x rotated left by (k mod 8) bits.
// To rotate x right by k bits, call RotateLeft8(x, -k).
//
// This function's execution time does not depend on the inputs.
uint8_t bits_RotateLeft8(uint8_t x, so_int k);

// RotateLeft16 returns the value of x rotated left by (k mod 16) bits.
// To rotate x right by k bits, call RotateLeft16(x, -k).
//
// This function's execution time does not depend on the inputs.
uint16_t bits_RotateLeft16(uint16_t x, so_int k);

// RotateLeft32 returns the value of x rotated left by (k mod 32) bits.
// To rotate x right by k bits, call RotateLeft32(x, -k).
//
// This function's execution time does not depend on the inputs.
uint32_t bits_RotateLeft32(uint32_t x, so_int k);

// RotateLeft64 returns the value of x rotated left by (k mod 64) bits.
// To rotate x right by k bits, call RotateLeft64(x, -k).
//
// This function's execution time does not depend on the inputs.
uint64_t bits_RotateLeft64(uint64_t x, so_int k);

// --- Reverse ---
// Reverse returns the value of x with its bits in reversed order.
so_uint bits_Reverse(so_uint x);

// Reverse8 returns the value of x with its bits in reversed order.
uint8_t bits_Reverse8(uint8_t x);

// Reverse16 returns the value of x with its bits in reversed order.
uint16_t bits_Reverse16(uint16_t x);

// Reverse32 returns the value of x with its bits in reversed order.
uint32_t bits_Reverse32(uint32_t x);

// Reverse64 returns the value of x with its bits in reversed order.
uint64_t bits_Reverse64(uint64_t x);

// --- ReverseBytes ---
// ReverseBytes returns the value of x with its bytes in reversed order.
//
// This function's execution time does not depend on the inputs.
so_uint bits_ReverseBytes(so_uint x);

// ReverseBytes16 returns the value of x with its bytes in reversed order.
//
// This function's execution time does not depend on the inputs.
uint16_t bits_ReverseBytes16(uint16_t x);

// ReverseBytes32 returns the value of x with its bytes in reversed order.
//
// This function's execution time does not depend on the inputs.
uint32_t bits_ReverseBytes32(uint32_t x);

// ReverseBytes64 returns the value of x with its bytes in reversed order.
//
// This function's execution time does not depend on the inputs.
uint64_t bits_ReverseBytes64(uint64_t x);

// --- Len ---
// Len returns the minimum number of bits required to represent x; the result is 0 for x == 0.
so_int bits_Len(so_uint x);

// Len8 returns the minimum number of bits required to represent x; the result is 0 for x == 0.
so_int bits_Len8(uint8_t x);

// Len16 returns the minimum number of bits required to represent x; the result is 0 for x == 0.
so_int bits_Len16(uint16_t x);

// Len32 returns the minimum number of bits required to represent x; the result is 0 for x == 0.
so_int bits_Len32(uint32_t x);

// Len64 returns the minimum number of bits required to represent x; the result is 0 for x == 0.
so_int bits_Len64(uint64_t x);

// --- Add with carry ---
// Add returns the sum with carry of x, y and carry: sum = x + y + carry.
// The carry input must be 0 or 1; otherwise the behavior is undefined.
// The carryOut output is guaranteed to be 0 or 1.
//
// This function's execution time does not depend on the inputs.
so_R_uint_uint bits_Add(so_uint x, so_uint y, so_uint carry);

// Add32 returns the sum with carry of x, y and carry: sum = x + y + carry.
// The carry input must be 0 or 1; otherwise the behavior is undefined.
// The carryOut output is guaranteed to be 0 or 1.
//
// This function's execution time does not depend on the inputs.
so_R_u32_u32 bits_Add32(uint32_t x, uint32_t y, uint32_t carry);

// Add64 returns the sum with carry of x, y and carry: sum = x + y + carry.
// The carry input must be 0 or 1; otherwise the behavior is undefined.
// The carryOut output is guaranteed to be 0 or 1.
//
// This function's execution time does not depend on the inputs.
so_R_u64_u64 bits_Add64(uint64_t x, uint64_t y, uint64_t carry);

// --- Subtract with borrow ---
// Sub returns the difference of x, y and borrow: diff = x - y - borrow.
// The borrow input must be 0 or 1; otherwise the behavior is undefined.
// The borrowOut output is guaranteed to be 0 or 1.
//
// This function's execution time does not depend on the inputs.
so_R_uint_uint bits_Sub(so_uint x, so_uint y, so_uint borrow);

// Sub32 returns the difference of x, y and borrow, diff = x - y - borrow.
// The borrow input must be 0 or 1; otherwise the behavior is undefined.
// The borrowOut output is guaranteed to be 0 or 1.
//
// This function's execution time does not depend on the inputs.
so_R_u32_u32 bits_Sub32(uint32_t x, uint32_t y, uint32_t borrow);

// Sub64 returns the difference of x, y and borrow: diff = x - y - borrow.
// The borrow input must be 0 or 1; otherwise the behavior is undefined.
// The borrowOut output is guaranteed to be 0 or 1.
//
// This function's execution time does not depend on the inputs.
so_R_u64_u64 bits_Sub64(uint64_t x, uint64_t y, uint64_t borrow);

// --- Full-width multiply ---
// Mul returns the full-width product of x and y: (hi, lo) = x * y
// with the product bits' upper half returned in hi and the lower
// half returned in lo.
//
// This function's execution time does not depend on the inputs.
so_R_uint_uint bits_Mul(so_uint x, so_uint y);

// Mul32 returns the 64-bit product of x and y: (hi, lo) = x * y
// with the product bits' upper half returned in hi and the lower
// half returned in lo.
//
// This function's execution time does not depend on the inputs.
so_R_u32_u32 bits_Mul32(uint32_t x, uint32_t y);

// Mul64 returns the 128-bit product of x and y: (hi, lo) = x * y
// with the product bits' upper half returned in hi and the lower
// half returned in lo.
//
// This function's execution time does not depend on the inputs.
so_R_u64_u64 bits_Mul64(uint64_t x, uint64_t y);

// --- Full-width divide ---
// Div returns the quotient and remainder of (hi, lo) divided by y:
// quo = (hi, lo)/y, rem = (hi, lo)%y with the dividend bits' upper
// half in parameter hi and the lower half in parameter lo.
// Div panics for y == 0 (division by zero) or y <= hi (quotient overflow).
so_R_uint_uint bits_Div(so_uint hi, so_uint lo, so_uint y);

// Div32 returns the quotient and remainder of (hi, lo) divided by y:
// quo = (hi, lo)/y, rem = (hi, lo)%y with the dividend bits' upper
// half in parameter hi and the lower half in parameter lo.
// Div32 panics for y == 0 (division by zero) or y <= hi (quotient overflow).
so_R_u32_u32 bits_Div32(uint32_t hi, uint32_t lo, uint32_t y);

// Div64 returns the quotient and remainder of (hi, lo) divided by y:
// quo = (hi, lo)/y, rem = (hi, lo)%y with the dividend bits' upper
// half in parameter hi and the lower half in parameter lo.
// Div64 panics for y == 0 (division by zero) or y <= hi (quotient overflow).
so_R_u64_u64 bits_Div64(uint64_t hi, uint64_t lo, uint64_t y);

// Rem returns the remainder of (hi, lo) divided by y. Rem panics for
// y == 0 (division by zero) but, unlike Div, it doesn't panic on a
// quotient overflow.
so_uint bits_Rem(so_uint hi, so_uint lo, so_uint y);

// Rem32 returns the remainder of (hi, lo) divided by y. Rem32 panics
// for y == 0 (division by zero) but, unlike [Div32], it doesn't panic
// on a quotient overflow.
uint32_t bits_Rem32(uint32_t hi, uint32_t lo, uint32_t y);

// Rem64 returns the remainder of (hi, lo) divided by y. Rem64 panics
// for y == 0 (division by zero) but, unlike [Div64], it doesn't panic
// on a quotient overflow.
uint64_t bits_Rem64(uint64_t hi, uint64_t lo, uint64_t y);
