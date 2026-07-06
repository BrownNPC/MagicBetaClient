#include "maps.h"

// -- Forward declarations --
static void rehash(maps_ByteMap* dst, maps_ByteMap* src);
static void insert(maps_ByteMap* m, so_int h, void* key, void* val);

// -- Variables and constants --

// must be above 50%
static const double loadFactor = 0.85;

// -- bytemap.go --

// NewByteMap creates a new ByteMap with the given initial capacity,
// key size, and value size, using the provided allocator (or the
// default allocator if nil). The map automatically grows as needed.
//
// If the allocator is nil, uses the system allocator.
// The caller is responsible for freeing map resources
// with [ByteMap.Free] when done using it.
maps_ByteMap maps_NewByteMap(mem_Allocator a, so_int size, so_int ksize, so_int vsize) {
    maps_ByteMap m = (maps_ByteMap){.a = a, .ksize = ksize, .vsize = vsize, .seed = runtime_Seed()};
    so_int sz = 8;
    // The map must be large enough to hold size entries without resizing.
    for (; (so_int)((double)(sz) * loadFactor) < size;) {
        sz *= 2;
    }
    m.hdib = mem_AllocSlice(uint64_t, (m.a), (sz), (sz));
    m.keys = mem_AllocSlice(so_byte, (m.a), (sz * ksize), (sz * ksize));
    m.vals = mem_AllocSlice(so_byte, (m.a), (sz * vsize), (sz * vsize));
    m.mask = sz - 1;
    m.growAt = (so_int)((double)(sz) * loadFactor);
    return m;
}

// Len returns the number of key-value pairs in the map.
so_int maps_ByteMap_Len(void* self) {
    maps_ByteMap* m = self;
    return m->len;
}

// Clear removes all key-value pairs from the map, resetting
// it to an empty state. Does not free map resources;
// the map can be reused after Clear.
void maps_ByteMap_Clear(void* self) {
    maps_ByteMap* m = self;
    so_clear(uint64_t, m->hdib);
    m->len = 0;
}

// Free frees internal resources used by the map.
// If the map is already freed, does nothing.
// The map must not be used after Free.
void maps_ByteMap_Free(void* self) {
    maps_ByteMap* m = self;
    if (so_len(m->hdib) == 0) {
        return;
    }
    mem_FreeSlice(uint64_t, (m->a), (m->hdib));
    mem_FreeSlice(so_byte, (m->a), (m->keys));
    mem_FreeSlice(so_byte, (m->a), (m->vals));
    m->hdib = (so_Slice){0};
    m->keys = (so_Slice){0};
    m->vals = (so_Slice){0};
    m->len = 0;
}

// Resize grows or reallocates the map to hold at least size entries.
void maps_ByteMap_Resize(void* self, so_int size) {
    maps_ByteMap* m = self;
    maps_ByteMap nmap = maps_NewByteMap(m->a, size, m->ksize, m->vsize);
    // preserve seed so stored hashes remain valid
    nmap.seed = m->seed;
    rehash(&nmap, m);
    maps_ByteMap_Free(m);
    *m = nmap;
}

// rehash moves all entries from src into dst.
static void rehash(maps_ByteMap* dst, maps_ByteMap* src) {
    uint64_t* hdib = unsafe_SliceData(src->hdib);
    so_byte* keys = unsafe_SliceData(src->keys);
    so_byte* vals = unsafe_SliceData(src->vals);
    so_int ksize = src->ksize;
    so_int vsize = src->vsize;
    so_int n = so_len(src->hdib);
    for (so_int i = 0; i < n; i++) {
        uint64_t* hdI = c_PtrAt(uint64_t, (hdib), (i));
        // for gcc analyzer
        c_Assert(hdI != NULL, "maps: nil hdib pointer");
        if ((*hdI & 0xFFFF) > 0) {
            insert(dst, (so_int)(*hdI >> 16), c_PtrAdd(so_byte, (keys), (i * ksize)), c_PtrAdd(so_byte, (vals), (i * vsize)));
        }
    }
}

// insert does byte-level Robin Hood insertion into a map.
// Used during rehash only - skips equality check since keys are unique.
static void insert(maps_ByteMap* m, so_int h, void* key, void* val) {
    uint64_t ehdib = (((uint64_t)(h) << 16) | 1);
    so_int ksize = m->ksize;
    so_int vsize = m->vsize;
    uint64_t* hdib = unsafe_SliceData(m->hdib);
    so_byte* keys = unsafe_SliceData(m->keys);
    so_byte* vals = unsafe_SliceData(m->vals);
    so_byte* ekey = c_Alloca(so_byte, (ksize));
    so_byte* eval = c_Alloca(so_byte, (vsize));
    mem_Copy(ekey, key, ksize);
    mem_Copy(eval, val, vsize);
    so_int i = (h & m->mask);
    for (;;) {
        uint64_t* hdI = c_PtrAt(uint64_t, (hdib), (i));
        if ((*hdI & 0xFFFF) == 0) {
            *hdI = ehdib;
            mem_Copy(c_PtrAdd(so_byte, (keys), (i * ksize)), ekey, ksize);
            mem_Copy(c_PtrAdd(so_byte, (vals), (i * vsize)), eval, vsize);
            m->len++;
            return;
        }
        if ((*hdI & 0xFFFF) < (ehdib & 0xFFFF)) {
            mem_Swap(uint64_t, (hdI), (&ehdib));
            mem_SwapByte(c_PtrAdd(so_byte, (keys), (i * ksize)), ekey, ksize);
            mem_SwapByte(c_PtrAdd(so_byte, (vals), (i * vsize)), eval, vsize);
        }
        i = ((i + 1) & m->mask);
        ehdib++;
    }
}

// -- hash.go --

// -- iter.go --

// -- maps.go --
