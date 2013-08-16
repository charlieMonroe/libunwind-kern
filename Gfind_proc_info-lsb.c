/* libunwind - a platform-independent unwind library
 Copyright (c) 2003-2005 Hewlett-Packard Development Company, L.P.
 Contributed by David Mosberger-Tang <davidm@hpl.hp.com>
 
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

/* Locate an FDE via the ELF data-structures defined by LSB v1.3
 (http://www.linuxbase.org/spec/).  */

#include <sys/param.h>
#include <sys/stddef.h>
#include <sys/limits.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/linker.h>
#include <sys/malloc.h>

#include <machine/elf.h>
#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_object.h>
#include <vm/vm_kern.h>
#include <vm/vm_extern.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>


#include "unwind-internal.h"
#include "include/dwarf_i.h"
#include "include/dwarf-eh.h"
#include "include/libunwind_i.h"
#include "elf64.h"

struct table_entry
{
  int32_t start_ip_offset;
  int32_t fde_offset;
};

typedef struct {
	void		*addr;
	Elf_Off		size;
	int		flags;
	int		sec;	/* Original section */
	char		*name;
} Elf_progent;

typedef struct {
	Elf_Rel		*rel;
	int		nrel;
	int		sec;
} Elf_relent;

typedef struct {
	Elf_Rela	*rela;
	int		nrela;
	int		sec;
} Elf_relaent;

typedef struct elf_file {
	struct linker_file lf;		/* Common fields */
  
	int		preloaded;
	caddr_t		address;	/* Relocation address */
	vm_object_t	object;		/* VM object to hold file pages */
	Elf_Shdr	*e_shdr;
  
	Elf_progent	*progtab;
	int		nprogtab;
  
	Elf_relaent	*relatab;
	int		nrelatab;
  
	Elf_relent	*reltab;
	int		nreltab;
  
	Elf_Sym		*ddbsymtab;	/* The symbol table we are using */
	long		ddbsymcnt;	/* Number of symbols */
	caddr_t		ddbstrtab;	/* String table */
	long		ddbstrcnt;	/* number of bytes in string table */
  
	caddr_t		shstrtab;	/* Section name string table */
	long		shstrcnt;	/* number of bytes in string table */
  
	caddr_t		ctftab;		/* CTF table */
	long		ctfcnt;		/* number of bytes in CTF table */
	caddr_t		ctfoff;		/* CTF offset table */
	caddr_t		typoff;		/* Type offset table */
	long		typlen;		/* Number of type entries. */
  
} *elf_file_t;

MALLOC_DECLARE(M_LIBUNWIND_FILE);
MALLOC_DEFINE(M_LIBUNWIND_FILE, "file", "file");

static int
linear_search (unw_addr_space_t as, unw_word_t ip,
               unw_word_t eh_frame_start, unw_word_t eh_frame_end,
               unw_word_t fde_count,
               unw_proc_info_t *pi, int need_unwind_info, void *arg)
{
  unw_accessors_t *a = unw_get_accessors (unw_local_addr_space);
  unw_word_t i = 0, fde_addr, addr = eh_frame_start;
  int ret;
  
  while (i++ < fde_count && addr < eh_frame_end)
  {
    fde_addr = addr;
    if ((ret = dwarf_extract_proc_info_from_fde (as, a, &addr, pi, 0, 0, arg))
        < 0)
      return ret;
    
    if (ip >= pi->start_ip && ip < pi->end_ip)
    {
      if (!need_unwind_info)
        return 1;
      addr = fde_addr;
      if ((ret = dwarf_extract_proc_info_from_fde (as, a, &addr, pi,
                                                   need_unwind_info, 0,
                                                   arg))
          < 0)
        return ret;
      return 1;
    }
  }
  return -UNW_ENOINFO;
}


static inline int
strings_equal(const char *str1, const char *str2)
{
  unsigned int index;
  
  if (str1 ==  str2){
    return 1;
  }
  
  if (str1 == NULL || str2 == NULL){
    /* Just one of them NULL */
    return 0;
  }
  
  index = 0;
  while (1) {
    if (str1[index] == str2[index]){
      if (str1[index] == '\0'){
        /* Equal */
        return 1;
      }
      ++index;
      continue;
    }
    return 0;
  }
  
  return 0;
}

static Elf_progent *find_eh_frame_section(linker_file_t file){
  /* It is a bad idea to try to look into the kernel file. 
   * Actually it would probably be a good idea to check that the
   * file ends with .ko, since those files actually get to be loaded
   * the way we need (elf_link_obj vs elf_link).
   */
  if (strings_equal("kernel", file->filename)){
    return NULL;
  }
  
  elf_file_t efile = (elf_file_t)file;
  Elf_progent *eh_frame_progent = NULL;
  Elf_progent *progent = NULL;
  for (int i = 0; i < efile->nprogtab; ++i) {
    progent = &efile->progtab[i];
    
    if (strings_equal(progent->name, ".eh_frame")){
      eh_frame_progent = progent;
      break;
    }
	}
  
  if (eh_frame_progent != NULL){
    Debug(-1, "Returning pointer to .eh_frame section %p\n", eh_frame_progent->addr);
    return eh_frame_progent;
  }
  return NULL;
}


/* ptr is a pointer to a dwarf_callback_data structure and, on entry,
 member ip contains the instruction-pointer we're looking
 for.  */
static int
dwarf_callback (linker_file_t file, struct dwarf_callback_data *cb_data)
{
  unw_dyn_info_t *di = &cb_data->di;
  unw_proc_info_t *pi = cb_data->pi;
  
  unw_word_t eh_frame_start;
  unw_word_t eh_frame_end;
  unw_word_t fde_count;
  unw_word_t ip = cb_data->ip;
  
  int found = 0;
  int need_unwind_info = cb_data->need_unwind_info;
  
//  pause("dwarf_callback", 100);
  
  /* First, see if the function indeed comes from this file. */
  if (ip < (unw_word_t)file->address ||
      ip > (unw_word_t)(file->address + file->size)) {
    /* Try the next file */
    Debug(-1, "The function definitely isn't in file %s\n", file->filename);
    return 0;
  }
  
  c_linker_sym_t function_symbol;
  long diff;
  if (linker_ddb_search_symbol((caddr_t)ip, &function_symbol, &diff) != 0) {
    Debug(-1, "Failed to find symbol at address %p\n", (void*)ip);
    return 0;
  }
  
  linker_symval_t values;
  if (linker_ddb_symbol_values(function_symbol, &values) != 0){
    Debug(15, "Failed to retrieve symbol values [ip=%p;fsym=%p]\n", (void*)ip,
          (void*)function_symbol);
    return 0;
  }
  
  Debug(-1, "Looking through file %s for function %s[start=%p;ip=%p]\n", file->filename,
        values.name, values.value, (void*)ip);
  
  pi->gp = di->gp = 0;
  
  Elf_progent *section = find_eh_frame_section(file);
  if (section == NULL){
    /* No section found. */
    return 0;
  }
  
  eh_frame_start = (unw_word_t)section->addr;
  eh_frame_end = (unw_word_t)((char*)section->addr + section->size);
  fde_count = (eh_frame_end - eh_frame_start) / sizeof(struct table_entry);
  
  di->format = UNW_INFO_FORMAT_TABLE;
  di->start_ip = (unw_word_t)values.value;
  di->end_ip = (unw_word_t)values.value + values.size;
  
  di->u.rti.name_ptr = (unw_word_t)values.name;
  
  cb_data->single_fde = 1;
  found = linear_search (unw_local_addr_space, ip,
                         eh_frame_start, eh_frame_end, fde_count,
                         pi, need_unwind_info, NULL);
  Debug(-1, "Linear search returned %i\n", found);
	if (found != 1){
		return 0;
	}
	
  return found;
}

HIDDEN int
dwarf_find_proc_info (unw_addr_space_t as, unw_word_t ip,
                      unw_proc_info_t *pi, int need_unwind_info, void *arg)
{
  struct dwarf_callback_data cb_data;
  intrmask_t saved_mask;
  int ret = 0;
  
  Debug (14, "looking for IP=0x%lx\n", (long) ip);
  
  memset (&cb_data, 0, sizeof (cb_data));
  cb_data.ip = ip;
  cb_data.pi = pi;
  cb_data.need_unwind_info = need_unwind_info;
  cb_data.di.format = -1;
  cb_data.di_debug.format = -1;
  
  SIGPROCMASK (SIG_SETMASK, &unwi_full_mask, &saved_mask);
  ret = linker_file_foreach((linker_predicate_t *)dwarf_callback, &cb_data);
  SIGPROCMASK (SIG_SETMASK, &saved_mask, NULL);
  
  if (ret > 0)
  {
    Debug (14, "IP=0x%lx not found\n", (long) ip);
    return -UNW_ENOINFO;
  }
  
  if (cb_data.single_fde)
  /* already got the result in *pi */
    return 0;
  
  /* search the table: */
  if (cb_data.di.format != -1)
    ret = dwarf_search_unwind_table (as, ip, &cb_data.di,
                                     pi, need_unwind_info, arg);
  else
    ret = -UNW_ENOINFO;
  
  if (ret == -UNW_ENOINFO && cb_data.di_debug.format != -1)
    ret = dwarf_search_unwind_table (as, ip, &cb_data.di_debug, pi,
                                     need_unwind_info, arg);
  return ret;
}

static inline const struct table_entry *
unw_lookup (const struct table_entry *table, size_t table_size, int32_t rel_ip)
{
  unsigned long table_len = table_size / sizeof (struct table_entry);
  const struct table_entry *e = NULL;
  unsigned long lo, hi, mid;
  
  /* do a binary search for right entry: */
  for (lo = 0, hi = table_len; lo < hi;)
  {
    mid = (lo + hi) / 2;
    e = table + mid;
    Debug (15, "e->start_ip_offset = %lx\n", (long) e->start_ip_offset);
    if (rel_ip < e->start_ip_offset)
      hi = mid;
    else
      lo = mid + 1;
  }
  if (hi <= 0)
    return NULL;
  e = table + hi - 1;
  return e;
}


PROTECTED int
dwarf_search_unwind_table (unw_addr_space_t as, unw_word_t ip,
                           unw_dyn_info_t *di, unw_proc_info_t *pi,
                           int need_unwind_info, void *arg)
{
  const struct table_entry *e = NULL, *table;
  unw_word_t segbase = 0, fde_addr;
  unw_accessors_t *a;
#ifndef UNW_LOCAL_ONLY
  struct table_entry ent;
#endif
  int ret;
  unw_word_t debug_frame_base;
  size_t table_len;
  
#ifdef UNW_REMOTE_ONLY
  assert (di->format == UNW_INFO_FORMAT_REMOTE_TABLE);
#else
  assert (di->format == UNW_INFO_FORMAT_REMOTE_TABLE
          || di->format == UNW_INFO_FORMAT_TABLE);
#endif
  assert (ip >= di->start_ip && ip < di->end_ip);
  
  if (di->format == UNW_INFO_FORMAT_REMOTE_TABLE)
  {
    table = (const struct table_entry *) (uintptr_t) di->u.rti.table_data;
    table_len = di->u.rti.table_len * sizeof (unw_word_t);
    debug_frame_base = 0;
  }
  else
  {
#ifndef UNW_REMOTE_ONLY
    struct unw_debug_frame_list *fdesc = (void *) di->u.ti.table_data;
    
    /* UNW_INFO_FORMAT_TABLE (i.e. .debug_frame) is read from local address
     space.  Both the index and the unwind tables live in local memory, but
     the address space to check for properties like the address size and
     endianness is the target one.  */
    as = unw_local_addr_space;
    table = fdesc->index;
    table_len = fdesc->index_size * sizeof (struct table_entry);
    debug_frame_base = (uintptr_t) fdesc->debug_frame;
#endif
  }
  
  a = unw_get_accessors (as);
  
#ifndef UNW_REMOTE_ONLY
  if (as == unw_local_addr_space)
  {
    segbase = di->u.rti.segbase;
    e = unw_lookup (table, table_len, ip - segbase);
  }
  else
#endif
  {
#ifndef UNW_LOCAL_ONLY
    segbase = di->u.rti.segbase;
    if ((ret = remote_lookup (as, (uintptr_t) table, table_len,
                              ip - segbase, &ent, arg)) < 0)
      return ret;
    if (ret)
      e = &ent;
    else
      e = NULL;	/* no info found */
#endif
  }
  if (!e)
  {
    Debug (1, "IP %lx inside range %lx-%lx, but no explicit unwind info found\n",
           (long) ip, (long) di->start_ip, (long) di->end_ip);
    /* IP is inside this table's range, but there is no explicit
     unwind info.  */
    return -UNW_ENOINFO;
  }
  Debug (15, "ip=0x%lx, start_ip=0x%lx\n",
         (long) ip, (long) (e->start_ip_offset));
  if (debug_frame_base)
    fde_addr = e->fde_offset + debug_frame_base;
  else
    fde_addr = e->fde_offset + segbase;
  Debug (1, "e->fde_offset = %lx, segbase = %lx, debug_frame_base = %lx, "
         "fde_addr = %lx\n", (long) e->fde_offset, (long) segbase,
         (long) debug_frame_base, (long) fde_addr);
  if ((ret = dwarf_extract_proc_info_from_fde (as, a, &fde_addr, pi,
                                               need_unwind_info,
                                               debug_frame_base, arg)) < 0)
    return ret;
  
  /* .debug_frame uses an absolute encoding that does not know about any
   shared library relocation.  */
  if (di->format == UNW_INFO_FORMAT_TABLE)
  {
    pi->start_ip += segbase;
    pi->end_ip += segbase;
    pi->flags = UNW_PI_FLAG_DEBUG_FRAME;
  }
  
  if (ip < pi->start_ip || ip >= pi->end_ip)
    return -UNW_ENOINFO;
  
  return 0;
}

HIDDEN void
dwarf_put_unwind_info (unw_addr_space_t as, unw_proc_info_t *pi, void *arg)
{
  return;	/* always a nop */
}
