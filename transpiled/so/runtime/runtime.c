#include "runtime.h"

// -- Implementation --

// Version returns the So tree's version string.
// It is either the commit hash and date at the time of the build or,
// when possible, a release tag like "v0.1.0".
so_String runtime_Version(void) {
    return runtime_buildVersion;
}
