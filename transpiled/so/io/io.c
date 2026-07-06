#include "io.h"

// -- Types --

typedef struct eofReader eofReader;

typedef struct eofReader {
} eofReader;

// -- Forward declarations --
static so_R_i64_err copyBuffer(io_Writer dst, io_Reader src, so_Slice buf);
static so_R_int_err eofReader_Read(void* self, so_Slice b);
static so_R_i64_err io_MultiReader_writeToWithBuffer(void* self, io_Writer w, so_Slice buf);

// -- Variables and constants --
static const int64_t maxint64 = (int64_t)(~(uint64_t)(0) >> 1);

// Discard is a [Writer] on which all Write calls
// succeed without doing anything.
io_Writer io_Discard = (io_Writer){.self = &(io_DiscardWriter){}, .Write = io_DiscardWriter_Write};

// 8KB
static const int64_t defaultBufSize = 8 * 1024;

// EOF is the error returned by Read when no more input is available.
// (Read must return EOF itself, not an error wrapping EOF,
// because callers will test for EOF using ==.)
// Functions should return EOF only to signal a graceful end of input.
// If the EOF occurs unexpectedly in a structured data stream,
// the appropriate error is either [ErrUnexpectedEOF] or some other error
// giving more detail.
so_Error io_EOF = errors_New("EOF");

// ErrInvalidWrite means that a write returned an impossible count.
so_Error io_ErrInvalidWrite = errors_New("io: Write returned impossible count");

// ErrNegativeRead means that a read returned a negative count.
so_Error io_ErrNegativeRead = errors_New("io: Read returned negative count");

// ErrNoProgress is returned by some clients of a [Reader] when
// many calls to Read have failed to return any data or error,
// usually the sign of a broken [Reader] implementation.
so_Error io_ErrNoProgress = errors_New("io: multiple Read calls return no data or error");

// ErrOffset is returned by seek functions when the offset argument is invalid.
so_Error io_ErrOffset = errors_New("io: invalid offset");

// ErrShortBuffer means that a read required a longer buffer than was provided.
so_Error io_ErrShortBuffer = errors_New("io: short buffer");

// ErrShortWrite means that a write accepted fewer bytes than requested
// but failed to return an explicit error.
so_Error io_ErrShortWrite = errors_New("io: short write");

// ErrUnexpectedEOF means that EOF was encountered in the
// middle of reading a fixed-size block or data structure.
so_Error io_ErrUnexpectedEOF = errors_New("io: unexpected EOF");

// ErrUnread is returned by unread operations when they can't perform for some reason.
so_Error io_ErrUnread = errors_New("io: cannot unread previous read operation");

// ErrWhence is returned by seek functions when the whence argument is invalid.
so_Error io_ErrWhence = errors_New("io: invalid whence");

// ErrClosedPipe is the error used for read or write operations on a closed pipe.
so_Error io_ErrClosedPipe = errors_New("io: read/write on closed pipe");

// -- faces.go --

// -- impls.go --

so_R_int_err io_DiscardWriter_Write(void* self, so_Slice p) {
    (void)self;
    return (so_R_int_err){.val = so_len(p), .err = (so_Error){0}};
}

so_R_int_err io_DiscardWriter_WriteString(void* self, so_String s) {
    (void)self;
    return (so_R_int_err){.val = so_len(s), .err = (so_Error){0}};
}

// LimitReader returns a LimitedReader that reads from r
// but stops with EOF after n bytes.
io_LimitedReader io_LimitReader(io_Reader r, int64_t n) {
    return (io_LimitedReader){r, n};
}

so_R_int_err io_LimitedReader_Read(void* self, so_Slice p) {
    io_LimitedReader* l = self;
    if (l->N <= 0) {
        return (so_R_int_err){.val = 0, .err = io_EOF};
    }
    if ((int64_t)(so_len(p)) > l->N) {
        p = so_slice(so_byte, p, 0, l->N);
    }
    so_R_int_err _res1 = l->R.Read(l->R.self, p);
    so_int n = _res1.val;
    so_Error err = _res1.err;
    l->N -= (int64_t)(n);
    return (so_R_int_err){.val = n, .err = err};
}

// NewNopCloser returns a [NopCloser] wrapping r.
io_NopCloser io_NewNopCloser(io_Reader r) {
    return (io_NopCloser){r};
}

so_R_int_err io_NopCloser_Read(void* self, so_Slice p) {
    io_NopCloser* n = self;
    return n->r.Read(n->r.self, p);
}

so_Error io_NopCloser_Close(void* self) {
    (void)self;
    return (so_Error){0};
}

// NewSectionReader returns a [SectionReader] that reads from r
// starting at offset off and stops with EOF after n bytes.
io_SectionReader io_NewSectionReader(io_ReaderAt r, int64_t off, int64_t n) {
    int64_t remaining = 0;
    if (off <= maxint64 - n) {
        remaining = n + off;
    } else {
        // Overflow, with no way to return error.
        // Assume we can read up to an offset of 1<<63 - 1.
        remaining = maxint64;
    }
    return (io_SectionReader){r, off, off, remaining, n};
}

so_R_int_err io_SectionReader_Read(void* self, so_Slice p) {
    io_SectionReader* s = self;
    if (s->off >= s->limit) {
        return (so_R_int_err){.val = 0, .err = io_EOF};
    }
    {
        int64_t max = s->limit - s->off;
        if ((int64_t)(so_len(p)) > max) {
            p = so_slice(so_byte, p, 0, max);
        }
    }
    so_R_int_err _res1 = s->r.ReadAt(s->r.self, p, s->off);
    so_int n = _res1.val;
    so_Error err = _res1.err;
    s->off += (int64_t)(n);
    return (so_R_int_err){.val = n, .err = err};
}

so_R_i64_err io_SectionReader_Seek(void* self, int64_t offset, so_int whence) {
    io_SectionReader* s = self;
    if (whence == io_SeekStart) {
        offset += s->base;
    } else if (whence == io_SeekCurrent) {
        offset += s->off;
    } else if (whence == io_SeekEnd) {
        offset += s->limit;
    } else {
        return (so_R_i64_err){.val = 0, .err = io_ErrWhence};
    }
    if (offset < s->base) {
        return (so_R_i64_err){.val = 0, .err = io_ErrOffset};
    }
    s->off = offset;
    return (so_R_i64_err){.val = offset - s->base, .err = (so_Error){0}};
}

so_R_int_err io_SectionReader_ReadAt(void* self, so_Slice p, int64_t off) {
    io_SectionReader* s = self;
    if (off < 0 || off >= io_SectionReader_Size(s)) {
        return (so_R_int_err){.val = 0, .err = io_EOF};
    }
    off += s->base;
    {
        int64_t max = s->limit - off;
        if ((int64_t)(so_len(p)) > max) {
            p = so_slice(so_byte, p, 0, max);
            so_R_int_err _res1 = s->r.ReadAt(s->r.self, p, off);
            so_int n = _res1.val;
            so_Error err = _res1.err;
            if (err.self == NULL) {
                err = io_EOF;
            }
            return (so_R_int_err){.val = n, .err = err};
        }
    }
    return s->r.ReadAt(s->r.self, p, off);
}

// Size returns the size of the section in bytes.
int64_t io_SectionReader_Size(void* self) {
    io_SectionReader* s = self;
    return s->limit - s->base;
}

// Outer returns the underlying [ReaderAt] and offsets for the section.
//
// The returned values are the same that were passed to [NewSectionReader]
// when the [SectionReader] was created.
io_ReaderAtOffset io_SectionReader_Outer(void* self) {
    io_SectionReader* s = self;
    return (io_ReaderAtOffset){s->r, s->base, s->n};
}

// -- io.go --

// Copy copies from src to dst until either EOF is reached
// on src or an error occurs. It returns the number of bytes
// copied and the first error encountered while copying, if any.
//
// A successful Copy returns err == nil, not err == EOF.
// Because Copy is defined to read from src until EOF, it does
// not treat an EOF from Read as an error to be reported.
//
// Copy allocates a buffer on the stack to hold data during the copy.
so_R_i64_err io_Copy(io_Writer dst, io_Reader src) {
    so_int size = defaultBufSize;
    bool ok = (src.Read == io_LimitedReader_Read);
    if (ok) {
        io_LimitedReader* l = (io_LimitedReader*)src.self;
        if ((int64_t)(size) > l->N) {
            if (l->N < 1) {
                size = 1;
            } else {
                size = (so_int)(l->N);
            }
        }
    }
    so_Slice buf = so_make_slice(so_byte, size, size);
    return copyBuffer(dst, src, buf);
}

// CopyBuffer is identical to Copy except that it stages through the
// provided buffer rather than allocating a temporary one.
// If buf is nil or has zero length, CopyBuffer panics.
so_R_i64_err io_CopyBuffer(io_Writer dst, io_Reader src, so_Slice buf) {
    if (so_len(buf) == 0) {
        so_panic("io.CopyBuffer: empty buffer");
    }
    return copyBuffer(dst, src, buf);
}

// CopyN copies n bytes (or until an error) from src to dst.
// It returns the number of bytes copied and the earliest
// error encountered while copying.
// On return, written == n if and only if err == nil.
//
// Allocates a buffer on the stack to hold data during the copy.
so_R_i64_err io_CopyN(io_Writer dst, io_Reader src, int64_t n) {
    io_LimitedReader* r = &(io_LimitedReader){src, n};
    so_R_i64_err _res1 = io_Copy(dst, (io_Reader){.self = r, .Read = io_LimitedReader_Read});
    int64_t written = _res1.val;
    so_Error err = _res1.err;
    if (written == n) {
        return (so_R_i64_err){.val = n, .err = (so_Error){0}};
    }
    if (written < n && err.self == NULL) {
        // src stopped early; must have been EOF.
        err = io_EOF;
    }
    return (so_R_i64_err){.val = written, .err = err};
}

// ReadAll reads from r until an error or EOF and returns the data it read.
// A successful call returns err == nil, not err == EOF. Because ReadAll is
// defined to read from src until EOF, it does not treat an EOF from Read
// as an error to be reported.
//
// If the allocator is nil, uses the system allocator.
// The returned slice is allocated; the caller owns it.
so_R_slice_err io_ReadAll(mem_Allocator a, io_Reader r) {
    // Build slices of exponentially growing size,
    // then copy into a perfectly-sized slice at the end.
    so_Slice buf = mem_AllocSlice(so_byte, (a), (0), (512));
    // Starting with next equal to 256 (instead of say 512 or 1024)
    // allows less memory usage for small inputs that finish in the
    // early growth stages, but we grow the read sizes quickly such that
    // it does not materially impact medium or large inputs.
    so_int next = 256;
    so_Slice chunks = mem_AllocSlice(so_Slice, (a), (0), (4));
    // Invariant: finalSize = sum(len(c) for c in chunks)
    so_int finalSize = 0;
    for (;;) {
        so_R_int_err _res1 = r.Read(r.self, so_slice(so_byte, buf, so_len(buf), so_cap(buf)));
        so_int n = _res1.val;
        so_Error err = _res1.err;
        buf = so_slice(so_byte, buf, 0, so_len(buf) + n);
        if (err.self != NULL) {
            if (err.self == io_EOF.self) {
                err = (so_Error){0};
            }
            if (so_len(chunks) == 0) {
                mem_FreeSlice(so_Slice, (a), (chunks));
                return (so_R_slice_err){.val = buf, .err = err};
            }
            // Build our final right-sized slice.
            finalSize += so_len(buf);
            so_Slice final = mem_AllocSlice(so_byte, (a), (0), (finalSize));
            for (so_int _ = 0; _ < so_len(chunks); _++) {
                so_Slice chunk = so_at(so_Slice, chunks, _);
                final = so_extend(so_byte, final, (chunk));
            }
            final = so_extend(so_byte, final, (buf));
            // Free the intermediate slices.
            for (so_int _ = 0; _ < so_len(chunks); _++) {
                so_Slice chunk = so_at(so_Slice, chunks, _);
                mem_FreeSlice(so_byte, (a), (chunk));
            }
            mem_FreeSlice(so_Slice, (a), (chunks));
            mem_FreeSlice(so_byte, (a), (buf));
            return (so_R_slice_err){.val = final, .err = err};
        }
        if (so_cap(buf) - so_len(buf) < so_cap(buf) / 16) {
            // Move to the next intermediate slice.
            chunks = slices_Append(so_Slice, (a), (chunks), (buf));
            finalSize += so_len(buf);
            buf = mem_AllocSlice(so_byte, (a), (0), (next));
            next += next / 2;
        }
    }
}

// ReadFull reads exactly len(buf) bytes from r into buf.
// It returns the number of bytes copied and an error if fewer bytes were read.
// The error is EOF only if no bytes were read.
// If an EOF happens after reading some but not all the bytes,
// ReadFull returns [ErrUnexpectedEOF].
// On return, n == len(buf) if and only if err == nil.
// If r returns an error having read at least len(buf) bytes, the error is dropped.
so_R_int_err io_ReadFull(io_Reader r, so_Slice buf) {
    so_int n = 0;
    so_Error err = {0};
    for (; n < so_len(buf) && err.self == NULL;) {
        so_int nn = 0;
        so_R_int_err _res1 = r.Read(r.self, so_slice(so_byte, buf, n, buf.len));
        nn = _res1.val;
        err = _res1.err;
        n += nn;
    }
    if (n >= so_len(buf)) {
        err = (so_Error){0};
    } else if (n > 0 && err.self == io_EOF.self) {
        err = io_ErrUnexpectedEOF;
    }
    return (so_R_int_err){.val = n, .err = err};
}

// WriteString writes the contents of the string s to w, which accepts a slice of bytes.
// [Writer.Write] is called exactly once.
so_R_int_err io_WriteString(io_Writer w, so_String s) {
    return w.Write(w.self, so_string_bytes(s));
}

// copyBuffer is the actual implementation of Copy and CopyN,
// with a buffer provided by the caller.
static so_R_i64_err copyBuffer(io_Writer dst, io_Reader src, so_Slice buf) {
    int64_t written = 0;
    so_Error err = {0};
    for (;;) {
        so_R_int_err _res1 = src.Read(src.self, buf);
        so_int nr = _res1.val;
        so_Error er = _res1.err;
        if (nr > 0) {
            so_R_int_err _res2 = dst.Write(dst.self, so_slice(so_byte, buf, 0, nr));
            so_int nw = _res2.val;
            so_Error ew = _res2.err;
            if (nw < 0 || nr < nw) {
                nw = 0;
                if (ew.self == NULL) {
                    ew = io_ErrInvalidWrite;
                }
            }
            written += (int64_t)(nw);
            if (ew.self != NULL) {
                err = ew;
                break;
            }
            if (nr != nw) {
                err = io_ErrShortWrite;
                break;
            }
        }
        if (er.self != NULL) {
            if (er.self != io_EOF.self) {
                err = er;
            }
            break;
        }
    }
    return (so_R_i64_err){.val = written, .err = err};
}

// -- multi.go --

static so_R_int_err eofReader_Read(void* self, so_Slice b) {
    (void)self;
    (void)b;
    return (so_R_int_err){.val = 0, .err = io_EOF};
}

so_R_int_err io_MultiReader_Read(void* self, so_Slice p) {
    io_MultiReader* mr = self;
    so_int n = 0;
    so_Error err = {0};
    for (; so_len(mr->readers) > 0;) {
        // Optimization to flatten nested multiReaders (Issue 13558).
        if (so_len(mr->readers) == 1) {
            io_Reader r0 = so_at(io_Reader, mr->readers, 0);
            {
                bool ok = (r0.Read == io_MultiReader_Read);
                if (ok) {
                    io_MultiReader* mr0 = (io_MultiReader*)r0.self;
                    mr->readers = mr0->readers;
                    continue;
                }
            }
        }
        so_R_int_err _res1 = so_at(io_Reader, mr->readers, 0).Read(so_at(io_Reader, mr->readers, 0).self, p);
        n = _res1.val;
        err = _res1.err;
        if (err.self == io_EOF.self) {
            // Use eofReader instead of nil to avoid nil panic
            // after performing flatten (Issue 18232).
            // permit earlier GC
            so_at(io_Reader, mr->readers, 0) = (io_Reader){.self = &(eofReader){}, .Read = eofReader_Read};
            mr->readers = so_slice(io_Reader, mr->readers, 1, mr->readers.len);
        }
        if (n > 0 || err.self != io_EOF.self) {
            if (err.self == io_EOF.self && so_len(mr->readers) > 0) {
                // Don't return EOF yet. More readers remain.
                err = (so_Error){0};
            }
            return (so_R_int_err){.val = n, .err = err};
        }
    }
    return (so_R_int_err){.val = 0, .err = io_EOF};
}

so_R_i64_err io_MultiReader_WriteTo(void* self, io_Writer w) {
    io_MultiReader* mr = self;
    return io_MultiReader_writeToWithBuffer(mr, w, so_make_slice(so_byte, defaultBufSize, defaultBufSize));
}

static so_R_i64_err io_MultiReader_writeToWithBuffer(void* self, io_Writer w, so_Slice buf) {
    io_MultiReader* mr = self;
    int64_t sum = 0;
    so_Error err = {0};
    for (so_int i = 0; i < so_len(mr->readers); i++) {
        io_Reader r = so_at(io_Reader, mr->readers, i);
        int64_t n = 0;
        {
            bool ok = (r.Read == io_MultiReader_Read);
            if (ok) {
                // reuse buffer with nested multiReaders
                io_MultiReader* subMr = (io_MultiReader*)r.self;
                so_R_i64_err _res1 = io_MultiReader_writeToWithBuffer(subMr, w, buf);
                n = _res1.val;
                err = _res1.err;
            } else {
                so_R_i64_err _res2 = copyBuffer(w, r, buf);
                n = _res2.val;
                err = _res2.err;
            }
        }
        sum += n;
        if (err.self != NULL) {
            // permit resume / retry after error
            mr->readers = so_slice(io_Reader, mr->readers, i, mr->readers.len);
            return (so_R_i64_err){.val = sum, .err = err};
        }
        // permit early GC
        so_at(io_Reader, mr->readers, i) = (io_Reader){0};
    }
    mr->readers = (so_Slice){0};
    return (so_R_i64_err){.val = sum, .err = (so_Error){0}};
}

// NewMultiReader returns a Reader that's the logical concatenation of
// the provided input readers. They're read sequentially. Once all
// inputs have returned EOF, Read will return EOF.  If any of the readers
// return a non-nil, non-EOF error, Read will return that error.
io_MultiReader io_NewMultiReader(so_Slice readers) {
    so_Slice r = so_make_slice(io_Reader, so_len(readers), so_len(readers));
    so_copy(io_Reader, r, readers);
    return (io_MultiReader){r};
}

so_R_int_err io_MultiWriter_Write(void* self, so_Slice p) {
    io_MultiWriter* t = self;
    so_int n = 0;
    so_Error err = {0};
    for (so_int _ = 0; _ < so_len(t->writers); _++) {
        io_Writer w = so_at(io_Writer, t->writers, _);
        so_R_int_err _res1 = w.Write(w.self, p);
        n = _res1.val;
        err = _res1.err;
        if (err.self != NULL) {
            return (so_R_int_err){.val = n, .err = err};
        }
        if (n != so_len(p)) {
            err = io_ErrShortWrite;
            return (so_R_int_err){.val = n, .err = err};
        }
    }
    return (so_R_int_err){.val = so_len(p), .err = (so_Error){0}};
}

so_R_int_err io_MultiWriter_WriteString(void* self, so_String s) {
    io_MultiWriter* t = self;
    so_int n = 0;
    so_Error err = {0};
    // lazily initialized if/when needed
    so_Slice p = {0};
    for (so_int _ = 0; _ < so_len(t->writers); _++) {
        io_Writer w = so_at(io_Writer, t->writers, _);
        if (p.ptr == NULL) {
            p = so_string_bytes(s);
        }
        so_R_int_err _res1 = w.Write(w.self, p);
        n = _res1.val;
        err = _res1.err;
        if (err.self != NULL) {
            return (so_R_int_err){.val = n, .err = err};
        }
        if (n != so_len(s)) {
            return (so_R_int_err){.val = n, .err = io_ErrShortWrite};
        }
    }
    return (so_R_int_err){.val = so_len(s), .err = (so_Error){0}};
}

// NewMultiWriter creates a writer that duplicates its writes to all the
// provided writers, similar to the Unix tee(1) command.
//
// Each write is written to each listed writer, one at a time.
// If a listed writer returns an error, that overall write operation
// stops and returns the error; it does not continue down the list.
io_MultiWriter io_NewMultiWriter(so_Slice writers) {
    so_Slice allWriters = so_make_slice(io_Writer, 0, so_len(writers));
    for (so_int _ = 0; _ < so_len(writers); _++) {
        io_Writer w = so_at(io_Writer, writers, _);
        {
            bool ok = (w.Write == io_MultiWriter_Write);
            if (ok) {
                io_MultiWriter* mw = (io_MultiWriter*)w.self;
                allWriters = so_extend(io_Writer, allWriters, (mw->writers));
            } else {
                allWriters = so_append(io_Writer, allWriters, w);
            }
        }
    }
    return (io_MultiWriter){allWriters};
}

// -- pipe.go --
