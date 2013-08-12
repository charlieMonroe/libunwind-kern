#define UNW_LOCAL_ONLY
#include "include/libunwind.h"
#include "include/libunwind-common.h"
#if defined(UNW_LOCAL_ONLY) && !defined(UNW_REMOTE_ONLY)
#include "Gstash_frame.c"
#endif
