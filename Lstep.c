#define UNW_LOCAL_ONLY
#include "include/libunwind.h"
#include "unwind-internal.h"
#if defined(UNW_LOCAL_ONLY) && !defined(UNW_REMOTE_ONLY)
#include "Gstep.c"
#endif
