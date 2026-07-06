#include "errors.h"

// -- Variables and constants --

// ErrUnsupported indicates that a requested operation cannot be performed,
// because it is unsupported. For example, a call to [os.Link] when using a
// file system that does not support hard links.
so_Error errors_ErrUnsupported = errors_New("unsupported operation");

// -- Implementation --
