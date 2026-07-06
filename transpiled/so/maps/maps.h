#pragma once
#include "so/builtin/builtin.h"
#include "so/c/c.h"
#include "so/math/bits/bits.h"
#include "so/mem/mem.h"
#include "so/runtime/runtime.h"

// -- Embeds --

// wymum performs 128-bit multiply-and-mix.
// Uses hardware 128-bit multiply on 64-bit, software fallback on 32-bit.
#if so_int_bits == 64
static inline uint64_t maps_wymum(uint64_t a, uint64_t b) {
    __uint128_t r = (__uint128_t)a * b;
    return (uint64_t)(r >> 64) ^ (uint64_t)r;
}
#else
static inline uint64_t maps_wymum(uint64_t a, uint64_t b) {
    so_R_u64_u64 r = bits_Mul64(a, b);
    return r.val ^ r.val2;
}
#endif

// keyHash hashes a key, dispatching to string or inline hash.
#define maps_keyHash(K, key_ptr, seed) _Generic((K){0}, \
    so_String: maps_hashString(key_ptr, seed),          \
    default: maps_hash(key_ptr, sizeof(K), seed))

// equal compares two typed key pointers for equality.
#define maps_keyEqual(K, a, b)                                       \
    _Generic((K){0},                                                 \
        so_String: so_string_eq(*(so_String*)(a), *(so_String*)(b)), \
        default: memcmp((a), (b), sizeof(K)) == 0)

// -- Types --

typedef struct maps_ByteMap maps_ByteMap;
typedef struct maps_Iter maps_Iter;
typedef struct maps_Map maps_Map;

// ByteMap is a Robin Hood hashmap operating on raw byte keys and values.
// Most users will want to use the generic [Map] wrapper type instead.
typedef struct maps_ByteMap {
    mem_Allocator a;
    so_Slice hdib;
    so_Slice keys;
    so_Slice vals;
    so_int ksize;
    so_int vsize;
    uint64_t seed;
    so_int len;
    so_int mask;
    so_int growAt;
} maps_ByteMap;

// Iter is an iterator over a Map's key-value pairs.
typedef struct maps_Iter {
    so_Slice hdib;
    so_Slice keys;
    so_Slice vals;
    so_int i;
} maps_Iter;

// Map is a generic hashmap similar to Go's built-in map[K]V.
// It automatically grows as needed, but does not shrink.
typedef struct maps_Map {
    maps_ByteMap bm;
} maps_Map;

// -- Functions and methods --

// NewByteMap creates a new ByteMap with the given initial capacity,
// key size, and value size, using the provided allocator (or the
// default allocator if nil). The map automatically grows as needed.
//
// If the allocator is nil, uses the system allocator.
// The caller is responsible for freeing map resources
// with [ByteMap.Free] when done using it.
maps_ByteMap maps_NewByteMap(mem_Allocator a, so_int size, so_int ksize, so_int vsize);

// Len returns the number of key-value pairs in the map.
so_int maps_ByteMap_Len(void* self);

// Clear removes all key-value pairs from the map, resetting
// it to an empty state. Does not free map resources;
// the map can be reused after Clear.
void maps_ByteMap_Clear(void* self);

// Free frees internal resources used by the map.
// If the map is already freed, does nothing.
// The map must not be used after Free.
void maps_ByteMap_Free(void* self);

// Resize grows or reallocates the map to hold at least size entries.
void maps_ByteMap_Resize(void* self, so_int size);

// wyr8 reads 8 bytes as a little-endian uint64.
//
static inline uint64_t wyr8(void* p) {
    uint64_t v = 0;
    mem_Copy(&v, p, 8);
    return v;
}

// wyr4 reads 4 bytes as a little-endian uint32.
//
static inline uint64_t wyr4(void* p) {
    uint32_t v = 0;
    mem_Copy(&v, p, 4);
    return (uint64_t)(v);
}

// hash computes wyhash with a per-map seed.
//
static inline so_int maps_hash(void* key, so_int length, uint64_t seed) {
    so_byte* p = (so_byte*)key;
    uint64_t wyp0 = (uint64_t)(0xa0761d6478bd642f);
    uint64_t wyp1 = (uint64_t)(0xe7037ed1a0b428db);
    seed = maps_wymum((seed ^ wyp0), wyp1);
    uint64_t a = 0, b = 0;
    if (length > 16) {
        for (so_int i = 0; i + 16 <= length; i += 16) {
            seed = maps_wymum((wyr8(c_PtrAdd(so_byte, (p), (i))) ^ wyp1), (wyr8(c_PtrAdd(so_byte, (p), (i + 8))) ^ seed));
        }
        a = wyr8(c_PtrAdd(so_byte, (p), (length - 16)));
        b = wyr8(c_PtrAdd(so_byte, (p), (length - 8)));
    } else if (length >= 4) {
        a = ((wyr4(p) << 32) | wyr4(c_PtrAdd(so_byte, (p), (((length >> 3) << 2)))));
        b = ((wyr4(c_PtrAdd(so_byte, (p), (length - 4))) << 32) | wyr4(c_PtrAdd(so_byte, (p), (length - 4 - ((length >> 3) << 2)))));
    } else if (length > 0) {
        a = ((((uint64_t)(*p) << 16) | ((uint64_t)(*c_PtrAdd(so_byte, (p), ((length >> 1)))) << 8)) | (uint64_t)(*c_PtrAdd(so_byte, (p), (length - 1))));
    }
    uint64_t r = maps_wymum((wyp1 ^ (uint64_t)(length)), maps_wymum((a ^ wyp1), (b ^ seed)));
    // upper 48 bits is the hash value
    return (so_int)(r >> 16);
}

// hashString hashes a string key by its content.
//
static inline so_int maps_hashString(void* keyPtr, uint64_t seed) {
    so_String s = *(so_String*)keyPtr;
    return maps_hash(unsafe_StringData(s), so_len(s), seed);
}

// Next advances the iterator to the next key-value pair, which will
// then be available through the [Iter.Key] and [Iter.Value] methods.
// It returns false if there are no more pairs to iterate over.
//
#define maps_Iter_Next(K, V, it_) ({ \
    bool _found = false; \
    uint64_t* _hdib = unsafe_SliceData(it_->hdib); \
    so_int _n = so_len(it_->hdib); \
    for (; it_->i < _n;) { \
        if ((*c_PtrAt(uint64_t, (_hdib), (it_->i)) & 0xFFFF) != 0) { \
            it_->i++; \
            _found = true; \
            break; \
        } \
        it_->i++; \
    } \
    _found; \
})

// Key returns the key of the current key-value pair.
//
#define maps_Iter_Key(K, V, it_) ({ \
    c_Assert(it_->i > 0, "maps: Iter.Key called before Next"); \
    K* _keys = c_PtrAs(K, (unsafe_SliceData(it_->keys))); \
    *c_PtrAt(K, (_keys), (it_->i - 1)); \
})

// Value returns the value of the current key-value pair.
//
#define maps_Iter_Value(K, V, it_) ({ \
    c_Assert(it_->i > 0, "maps: Iter.Value called before Next"); \
    V* _vals = c_PtrAs(V, (unsafe_SliceData(it_->vals))); \
    *c_PtrAt(V, (_vals), (it_->i - 1)); \
})

// New creates a new Map with the given minimal capacity.
//
#define maps_New(K, V, a_, size_) ({ \
    maps_ByteMap bm = maps_NewByteMap(a_, size_, c_Sizeof(K), c_Sizeof(V)); \
    (maps_Map){.bm = bm}; \
})

// Has returns true if the given key is in the map.
//
#define maps_Map_Has(K, V, m_, key_) ({ \
    K _key = key_; \
    bool _found = false; \
    maps_ByteMap _m = m_->bm; \
    if (so_len(_m.hdib) > 0) { \
        so_int _hash = maps_keyHash(K, (&_key), (_m.seed)); \
        so_int _i = (_hash & _m.mask); \
        uint64_t* _hdib = unsafe_SliceData(_m.hdib); \
        K* _keys = c_PtrAs(K, (unsafe_SliceData(_m.keys))); \
        so_int _dist = 1; \
        for (;;) { \
            uint64_t _ehdib = *c_PtrAt(uint64_t, (_hdib), (_i)); \
            if ((so_int)(_ehdib & 0xFFFF) < _dist) { \
                break; \
            } \
            if ((so_int)(_ehdib >> 16) == _hash && maps_keyEqual(K, (&_key), (c_PtrAt(K, (_keys), (_i))))) { \
                _found = true; \
                break; \
            } \
            _i = ((_i + 1) & _m.mask); \
            _dist++; \
        } \
    } \
    _found; \
})

// Get returns the value for the given key,
// or the zero value if the key is not in the map.
//
#define maps_Map_Get(K, V, m_, key_) ({ \
    K _key = key_; \
    V _val = c_Zero(V); \
    maps_ByteMap _m = m_->bm; \
    if (so_len(_m.hdib) > 0) { \
        so_int _hash = maps_keyHash(K, (&_key), (_m.seed)); \
        so_int _i = (_hash & _m.mask); \
        uint64_t* _hdib = unsafe_SliceData(_m.hdib); \
        K* _keys = c_PtrAs(K, (unsafe_SliceData(_m.keys))); \
        V* _vals = c_PtrAs(V, (unsafe_SliceData(_m.vals))); \
        so_int _dist = 1; \
        for (;;) { \
            uint64_t _ehdib = *c_PtrAt(uint64_t, (_hdib), (_i)); \
            if ((so_int)(_ehdib & 0xFFFF) < _dist) { \
                break; \
            } \
            if ((so_int)(_ehdib >> 16) == _hash && maps_keyEqual(K, (&_key), (c_PtrAt(K, (_keys), (_i))))) { \
                _val = *c_PtrAt(V, (_vals), (_i)); \
                break; \
            } \
            _i = ((_i + 1) & _m.mask); \
            _dist++; \
        } \
    } \
    _val; \
})

// Set sets the value for the given key,
// overwriting any existing value.
//
#define maps_Map_Set(K, V, m_, key_, value_) do { \
    K _key = key_; \
    V _val = value_; \
    maps_ByteMap* _m = &m_->bm; \
    if (_m->len >= _m->growAt) { \
        maps_ByteMap_Resize(_m, so_len(_m->hdib) * 2); \
    } \
    so_int _hash = maps_keyHash(K, (&_key), (_m->seed)); \
    uint64_t _ehdib = (((uint64_t)(_hash) << 16) | 1); \
    so_int _i = (_hash & _m->mask); \
    uint64_t* _hdib = unsafe_SliceData(_m->hdib); \
    K* _keys = c_PtrAs(K, (unsafe_SliceData(_m->keys))); \
    V* _vals = c_PtrAs(V, (unsafe_SliceData(_m->vals))); \
    K _ekey = _key; \
    V _eval = _val; \
    for (;;) { \
        uint64_t* _hdi = c_PtrAt(uint64_t, (_hdib), (_i)); \
        if ((*_hdi & 0xFFFF) == 0) { \
            *_hdi = _ehdib; \
            *c_PtrAt(K, (_keys), (_i)) = _ekey; \
            *c_PtrAt(V, (_vals), (_i)) = _eval; \
            _m->len++; \
            break; \
        } \
        if ((_ehdib >> 16) == (*_hdi >> 16) && maps_keyEqual(K, (&_ekey), (c_PtrAt(K, (_keys), (_i))))) { \
            *c_PtrAt(V, (_vals), (_i)) = _eval; \
            break; \
        } \
        if ((*_hdi & 0xFFFF) < (_ehdib & 0xFFFF)) { \
            mem_Swap(uint64_t, (_hdi), (&_ehdib)); \
            mem_Swap(K, (c_PtrAt(K, (_keys), (_i))), (&_ekey)); \
            mem_Swap(V, (c_PtrAt(V, (_vals), (_i))), (&_eval)); \
        } \
        _i = ((_i + 1) & _m->mask); \
        _ehdib++; \
    } \
} while (0)

// Delete removes the key and its value from the map.
// If the key is not in the map, does nothing.
//
#define maps_Map_Delete(K, V, m_, key_) do { \
    K _key = key_; \
    maps_ByteMap* _m = &m_->bm; \
    so_int _hash = maps_keyHash(K, (&_key), (_m->seed)); \
    so_int _i = (_hash & _m->mask); \
    uint64_t* _hdib = unsafe_SliceData(_m->hdib); \
    K* _keys = c_PtrAs(K, (unsafe_SliceData(_m->keys))); \
    V* _vals = c_PtrAs(V, (unsafe_SliceData(_m->vals))); \
    so_int _dist = 1; \
    for (; so_len(_m->hdib) > 0;) { \
        uint64_t* _hdi = c_PtrAt(uint64_t, (_hdib), (_i)); \
        if ((so_int)(*_hdi & 0xFFFF) < _dist) { \
            break; \
        } \
        if ((so_int)(*_hdi >> 16) == _hash && maps_keyEqual(K, (&_key), (c_PtrAt(K, (_keys), (_i))))) { \
            for (;;) { \
                so_int _prev = _i; \
                _i = ((_i + 1) & _m->mask); \
                if ((*c_PtrAt(uint64_t, (_hdib), (_i)) & 0xFFFF) <= 1) { \
                    *c_PtrAt(uint64_t, (_hdib), (_prev)) = 0; \
                    mem_Clear(c_PtrAt(K, (_keys), (_prev)), c_Sizeof(K)); \
                    mem_Clear(c_PtrAt(V, (_vals), (_prev)), c_Sizeof(V)); \
                    break; \
                } \
                *c_PtrAt(uint64_t, (_hdib), (_prev)) = *c_PtrAt(uint64_t, (_hdib), (_i)) - 1; \
                *c_PtrAt(K, (_keys), (_prev)) = *c_PtrAt(K, (_keys), (_i)); \
                *c_PtrAt(V, (_vals), (_prev)) = *c_PtrAt(V, (_vals), (_i)); \
            } \
            _m->len--; \
            break; \
        } \
        _i = ((_i + 1) & _m->mask); \
        _dist++; \
    } \
} while (0)

// Iter returns an iterator over the map's key-value pairs.
// The map must not be modified or freed while iterating.
//
#define maps_Map_Iter(K, V, m_) ({ \
    (maps_Iter){.hdib = m_->bm.hdib, .keys = m_->bm.keys, .vals = m_->bm.vals}; \
})

// Len returns the number of key-value pairs in the map.
//
#define maps_Map_Len(K, V, m_) ({ \
    maps_ByteMap_Len(&m_->bm); \
})

// Clear removes all key-value pairs from the map, resetting
// it to an empty state. Does not free map resources;
// the map can be reused after Clear.
//
#define maps_Map_Clear(K, V, m_) do { \
    maps_ByteMap_Clear(&m_->bm); \
} while (0)

// Free frees internal resources used by the map.
// If the map is already freed, does nothing.
// The map must not be used after calling Free.
//
#define maps_Map_Free(K, V, m_) do { \
    maps_ByteMap_Free(&m_->bm); \
} while (0)
