#include <sys/malloc.h>
#include "include/libunwind_i.h"

MALLOC_DEFINE(M_LIBUNWIND_TYPE, "libunwind allocations", "Libunwind "
              "Allocations");