#pragma once
#include "so/builtin/builtin.h"

// -- Variables and constants --

// ErrUnsupported indicates that a requested operation cannot be performed,
// because it is unsupported. For example, a call to [os.Link] when using a
// file system that does not support hard links.
extern so_Error errors_ErrUnsupported;
