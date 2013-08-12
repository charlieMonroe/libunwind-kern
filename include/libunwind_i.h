/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2005 Hewlett-Packard Co
   Copyright (C) 2007 David Mosberger-Tang
	Contributed by David Mosberger-Tang <dmosberger@gmail.com>

This file is part of libunwind.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

/* This files contains libunwind-internal definitions which are
   subject to frequent change and are not to be exposed to
   libunwind-users.  */

#ifndef libunwind_i_h
#define libunwind_i_h

#include "compiler.h"

/* Platform-independent libunwind-internal declarations.  */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/sx.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/osd.h>
# include <sys/endian.h>

#include "libunwind.h"

#ifdef DEBUG
# define UNW_DEBUG	1
#else
# define UNW_DEBUG	0
#endif

#define assert(cond, format...)			\
	if (!(cond)) { printf(format); panic(""); }


#if UNW_DEBUG
#define unwi_debug_level		UNWI_ARCH_OBJ(debug_level)
extern long unwi_debug_level;

# define Debug(level,format...)						\
do {									\
  if (unwi_debug_level >= level)					\
    {									\
      int _n = level;							\
      if (_n > 16)							\
	_n = 16;							\
      uprintf ("%*c>%s: ", _n, ' ', __FUNCTION__);		\
      uprintf (format);						\
    }									\
} while (0)
# define Dprintf(format...) 	    printf (format)
# ifdef __GNUC__
#  undef inline
#  define inline	UNUSED
# endif
#else
# define Debug(level,format...)
# define Dprintf(format...)
#endif

#include "dwarf_i.h"

#define mutex_init(l, name)							\
	sx_init_flags(l, name, SX_RECURSE)
#define mutex_lock(l)							\
	sx_xlock(l)
#define mutex_unlock(l)							\
	sx_unlock(l)


static inline int
cmpxchg_ptr (void *addr, void *old, void *new)
{
  union
    {
      void *vp;
      long *vlp;
    }
  u;

  u.vp = addr;
  return __sync_bool_compare_and_swap(u.vlp, (long) old, (long) new);
}
# define fetch_and_add1(_ptr)		__sync_fetch_and_add(_ptr, 1)
# define fetch_and_add(_ptr, value)	__sync_fetch_and_add(_ptr, value)
# define HAVE_CMPXCHG
# define HAVE_FETCH_AND_ADD
#define atomic_read(ptr)	(*(ptr))

#define UNWI_OBJ(fn)	  UNW_PASTE(UNW_PREFIX,UNW_PASTE(I,fn))
#define UNWI_ARCH_OBJ(fn) UNW_PASTE(UNW_PASTE(UNW_PASTE(_UI,UNW_TARGET),_), fn)

#define unwi_full_mask    UNWI_ARCH_OBJ(full_mask)

/* Silence compiler warnings about variables which are used only if libunwind
   is configured in a certain way */
static inline void mark_as_used(void *v UNUSED) {
}

# define SIGPROCMASK(how, new_mask, old_mask) mark_as_used(old_mask)

#define define_lock(name) \
  struct sx name;
#define lock_init(l, name)		mutex_init (l, name)
#define lock_acquire(l,m)				\
  mutex_lock (l);
#define lock_release(l,m)			\
  mutex_unlock (l);

#define SOS_MEMORY_SIZE 16384	/* see src/mi/mempool.c */

#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS MAP_ANON
#endif
#define GET_MEMORY(mem, size, type)				    		    \
do {									    \
  mem = malloc(size, type, M_WAITOK)					\
} while (0)

#define unwi_find_dynamic_proc_info	UNWI_OBJ(find_dynamic_proc_info)
#define unwi_extract_dynamic_proc_info	UNWI_OBJ(extract_dynamic_proc_info)
#define unwi_put_dynamic_unwind_info	UNWI_OBJ(put_dynamic_unwind_info)
#define unwi_dyn_remote_find_proc_info	UNWI_OBJ(dyn_remote_find_proc_info)
#define unwi_dyn_remote_put_unwind_info	UNWI_OBJ(dyn_remote_put_unwind_info)
#define unwi_dyn_validate_cache		UNWI_OBJ(dyn_validate_cache)

extern int unwi_find_dynamic_proc_info (unw_addr_space_t as,
					unw_word_t ip,
					unw_proc_info_t *pi,
					int need_unwind_info, void *arg);
extern int unwi_extract_dynamic_proc_info (unw_addr_space_t as,
					   unw_word_t ip,
					   unw_proc_info_t *pi,
					   unw_dyn_info_t *di,
					   int need_unwind_info,
					   void *arg);
extern void unwi_put_dynamic_unwind_info (unw_addr_space_t as,
					  unw_proc_info_t *pi, void *arg);

/* These handle the remote (cross-address-space) case of accessing
   dynamic unwind info. */

extern int unwi_dyn_remote_find_proc_info (unw_addr_space_t as,
					   unw_word_t ip,
					   unw_proc_info_t *pi,
					   int need_unwind_info,
					   void *arg);
extern void unwi_dyn_remote_put_unwind_info (unw_addr_space_t as,
					     unw_proc_info_t *pi,
					     void *arg);
extern int unwi_dyn_validate_cache (unw_addr_space_t as, void *arg);

extern unw_dyn_info_list_t _U_dyn_info_list;
extern struct sx _U_dyn_info_list_lock;

static ALWAYS_INLINE int
print_error (const char *string)
{
  return uprintf ("%s\n", string);
}

#define mi_init		UNWI_ARCH_OBJ(mi_init)

extern void mi_init (void);	/* machine-independent initializations */
extern unw_word_t _U_dyn_info_list_addr (void);

/* This is needed/used by ELF targets only.  */

struct elf_image
  {
    void *image;		/* pointer to mmap'd image */
    size_t size;		/* (file-) size of the image */
  };

struct elf_dyn_info
  {
    struct elf_image ei;
    unw_dyn_info_t di_cache;
    unw_dyn_info_t di_debug;    /* additional table info for .debug_frame */
  };

static inline void invalidate_edi (struct elf_dyn_info *edi)
{
// TODO
	/*if (edi->ei.image)
    munmap (edi->ei.image, edi->ei.size);*/
  memset (edi, 0, sizeof (*edi));
  edi->di_cache.format = -1;
  edi->di_debug.format = -1;
}


/* Provide a place holder for architecture to override for fast access
   to memory when known not to need to validate and know the access
   will be local to the process. A suitable override will improve
   unw_tdep_trace() performance in particular. */
#define ACCESS_MEM_FAST(ret,validate,cur,addr,to) \
  do { (ret) = dwarf_get ((cur), DWARF_MEM_LOC ((cur), (addr)), &(to)); } \
  while (0)

/* Define GNU and processor specific values for the Phdr p_type field in case
   they aren't defined by <elf.h>.  */
#ifndef PT_GNU_EH_FRAME
# define PT_GNU_EH_FRAME	0x6474e550
#endif /* !PT_GNU_EH_FRAME */
#ifndef PT_ARM_EXIDX
# define PT_ARM_EXIDX		0x70000001	/* ARM unwind segment */
#endif /* !PT_ARM_EXIDX */

#include "tdep/libunwind_i.h"

#ifndef tdep_get_func_addr
# define tdep_get_func_addr(as,addr,v)		(*(v) = addr, 0)
#endif

#define UNW_ALIGN(x,a) (((x)+(a)-1UL)&~((a)-1UL))

#endif /* libunwind_i_h */
