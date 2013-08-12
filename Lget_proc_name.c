#define UNW_LOCAL_ONLY
#include "include/libunwind.h"
#if defined(UNW_LOCAL_ONLY) && !defined(UNW_REMOTE_ONLY)
#include "Gget_proc_name.c"
#endif
