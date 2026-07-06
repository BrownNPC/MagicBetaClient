#include "mem.h"

// -- Variables and constants --

// ErrOutOfMemory is returned when a memory allocation
// fails due to insufficient memory.
so_Error mem_ErrOutOfMemory = errors_New("out of memory");
so_Error mem_ErrNoAlloc = errors_New("mem: allocation not allowed");

// NoAlloc is an instance of an Allocator that returns an error
// on any allocation. Free is a no-op.
// It's meant for cases when allocations are strictly forbidden.
mem_Allocator mem_NoAlloc = (mem_Allocator){.self = &(mem_NoAllocator){}, .Alloc = mem_NoAllocator_Alloc, .Free = mem_NoAllocator_Free, .Realloc = mem_NoAllocator_Realloc};

// System is an instance of a memory allocator that uses
// the system's malloc, realloc, and free functions.
mem_Allocator mem_System = (mem_Allocator){.self = &(mem_SystemAllocator){}, .Alloc = mem_SystemAllocator_Alloc, .Free = mem_SystemAllocator_Free, .Realloc = mem_SystemAllocator_Realloc};

// -- allocator.go --

// -- arena.go --

// NewArena creates an arena allocator backed by the given buffer.
mem_Arena mem_NewArena(so_Slice buf) {
    return (mem_Arena){.buf = buf};
}

so_R_ptr_err mem_Arena_Alloc(void* self, so_int size, so_int align) {
    mem_Arena* a = self;
    if (size <= 0) {
        so_panic("mem: invalid allocation size");
    }
    if (align <= 0 || (align & (align - 1)) != 0) {
        so_panic("mem: invalid alignment");
    }
    so_int aligned = ((a->offset + align - 1) & ~(align - 1));
    if (aligned + size > so_len(a->buf)) {
        return (so_R_ptr_err){.val = NULL, .err = mem_ErrOutOfMemory};
    }
    so_clear(so_byte, so_slice(so_byte, a->buf, aligned, aligned + size));
    a->lastStart = aligned;
    a->offset = aligned + size;
    return (so_R_ptr_err){.val = &so_at(so_byte, a->buf, aligned), .err = (so_Error){0}};
}

so_R_ptr_err mem_Arena_Realloc(void* self, void* ptr, so_int oldSize, so_int newSize, so_int align) {
    mem_Arena* a = self;
    if (oldSize <= 0 || newSize <= 0) {
        so_panic("mem: invalid allocation size");
    }
    if (align <= 0 || (align & (align - 1)) != 0) {
        so_panic("mem: invalid alignment");
    }
    // Last allocation: resize in place.
    if (ptr == &so_at(so_byte, a->buf, a->lastStart) && a->lastStart + oldSize == a->offset) {
        so_int newEnd = a->lastStart + newSize;
        if (newEnd > so_len(a->buf)) {
            return (so_R_ptr_err){.val = NULL, .err = mem_ErrOutOfMemory};
        }
        if (newSize > oldSize) {
            so_clear(so_byte, so_slice(so_byte, a->buf, a->offset, newEnd));
        }
        a->offset = newEnd;
        return (so_R_ptr_err){.val = ptr, .err = (so_Error){0}};
    }
    // Not the last allocation, shrinking: return same pointer.
    if (newSize <= oldSize) {
        return (so_R_ptr_err){.val = ptr, .err = (so_Error){0}};
    }
    // Not the last allocation, growing: allocate new and copy.
    so_R_ptr_err _res1 = mem_Arena_Alloc(a, newSize, align);
    void* newPtr = _res1.val;
    so_Error err = _res1.err;
    if (err.self != NULL) {
        return (so_R_ptr_err){.val = NULL, .err = err};
    }
    memmove(newPtr, ptr, (uintptr_t)(oldSize));
    return (so_R_ptr_err){.val = newPtr, .err = (so_Error){0}};
}

void mem_Arena_Free(void* self, void* ptr, so_int size, so_int align) {
    mem_Arena* a = self;
    (void)align;
    // Last allocation: reclaim the space.
    if (ptr == &so_at(so_byte, a->buf, a->lastStart) && a->lastStart + size == a->offset) {
        a->offset = a->lastStart;
    }
}

// Reset reclaims all allocated memory, allowing the arena to be reused.
void mem_Arena_Reset(void* self) {
    mem_Arena* a = self;
    a->offset = 0;
    a->lastStart = 0;
}

// -- malloc.go --

// -- mem.go --

// FreeString frees a heap-allocated string.
// If the allocator is nil, uses the system allocator.
void mem_FreeString(mem_Allocator a, so_String s) {
    if (so_len(s) == 0) {
        return;
    }
    mem_Free(so_byte, (a), (unsafe_StringData(s)));
}

// -- noalloc.go --

so_R_ptr_err mem_NoAllocator_Alloc(void* self, so_int size, so_int align) {
    (void)self;
    (void)size;
    (void)align;
    return (so_R_ptr_err){.val = NULL, .err = mem_ErrNoAlloc};
}

so_R_ptr_err mem_NoAllocator_Realloc(void* self, void* ptr, so_int oldSize, so_int newSize, so_int align) {
    (void)self;
    (void)ptr;
    (void)oldSize;
    (void)newSize;
    (void)align;
    return (so_R_ptr_err){.val = NULL, .err = mem_ErrNoAlloc};
}

void mem_NoAllocator_Free(void* self, void* ptr, so_int size, so_int align) {
    (void)self;
    (void)ptr;
    (void)size;
    (void)align;
}

// -- system.go --

so_R_ptr_err mem_SystemAllocator_Alloc(void* self, so_int size, so_int align) {
    (void)self;
    if (size <= 0) {
        so_panic("mem: invalid allocation size");
    }
    if (align <= 0 || (align & (align - 1)) != 0) {
        so_panic("mem: invalid alignment");
    }
    void* ptr = calloc(1, (uintptr_t)(size));
    if (ptr == NULL) {
        return (so_R_ptr_err){.val = NULL, .err = mem_ErrOutOfMemory};
    }
    return (so_R_ptr_err){.val = ptr, .err = (so_Error){0}};
}

so_R_ptr_err mem_SystemAllocator_Realloc(void* self, void* ptr, so_int oldSize, so_int newSize, so_int align) {
    (void)self;
    if (oldSize <= 0 || newSize <= 0) {
        so_panic("mem: invalid allocation size");
    }
    if (align <= 0 || (align & (align - 1)) != 0) {
        so_panic("mem: invalid alignment");
    }
    void* newPtr = realloc(ptr, (uintptr_t)(newSize));
    if (newPtr == NULL) {
        return (so_R_ptr_err){.val = NULL, .err = mem_ErrOutOfMemory};
    }
    if (newSize > oldSize) {
        // Zero new memory beyond the old size.
        so_byte* p = c_PtrAdd(so_byte, ((so_byte*)newPtr), (oldSize));
        mem_Clear(p, newSize - oldSize);
    }
    return (so_R_ptr_err){.val = newPtr, .err = (so_Error){0}};
}

void mem_SystemAllocator_Free(void* self, void* ptr, so_int size, so_int align) {
    (void)self;
    (void)size;
    (void)align;
    free(ptr);
}

// -- tracker.go --

so_R_ptr_err mem_Tracker_Alloc(void* self, so_int size, so_int align) {
    mem_Tracker* t = self;
    so_R_ptr_err _res1 = t->Allocator.Alloc(t->Allocator.self, size, align);
    void* ptr = _res1.val;
    so_Error err = _res1.err;
    if (err.self != NULL) {
        return (so_R_ptr_err){.val = NULL, .err = err};
    }
    t->Stats.Alloc += (uint64_t)(size);
    t->Stats.TotalAlloc += (uint64_t)(size);
    t->Stats.Mallocs++;
    return (so_R_ptr_err){.val = ptr, .err = (so_Error){0}};
}

so_R_ptr_err mem_Tracker_Realloc(void* self, void* ptr, so_int oldSize, so_int newSize, so_int align) {
    mem_Tracker* t = self;
    so_R_ptr_err _res1 = t->Allocator.Realloc(t->Allocator.self, ptr, oldSize, newSize, align);
    void* newPtr = _res1.val;
    so_Error err = _res1.err;
    if (err.self != NULL) {
        return (so_R_ptr_err){.val = NULL, .err = err};
    }
    if (newSize > oldSize) {
        t->Stats.Alloc += (uint64_t)(newSize - oldSize);
        t->Stats.TotalAlloc += (uint64_t)(newSize - oldSize);
    } else {
        t->Stats.Alloc -= (uint64_t)(oldSize - newSize);
    }
    t->Stats.Mallocs++;
    t->Stats.Frees++;
    return (so_R_ptr_err){.val = newPtr, .err = (so_Error){0}};
}

void mem_Tracker_Free(void* self, void* ptr, so_int size, so_int align) {
    mem_Tracker* t = self;
    t->Allocator.Free(t->Allocator.self, ptr, size, align);
    t->Stats.Alloc -= (uint64_t)(size);
    t->Stats.Frees++;
}
