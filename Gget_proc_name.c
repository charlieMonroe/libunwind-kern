/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2005 Hewlett-Packard Co
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

#include "include/libunwind_i.h"
#include <sys/linker.h>

struct proc_name_search_result {
  caddr_t ip;
  char *buf;
  size_t buf_len;
  long *offset;
};

static int linker_foreach(linker_file_t file, void *ctx) {
  struct proc_name_search_result *result = (struct proc_name_search_result*)ctx;
  if (linker_search_symbol_name(result->ip, result->buf, result->buf_len, result->offset) == 0){
    // Found it -> return 1 so that the foreach stops
    return 1;
  }
  return 0;
}

static inline int
get_proc_name (unw_addr_space_t as, unw_word_t ip,
	       char *buf, size_t buf_len, unw_word_t *offp, void *arg)
{
  struct proc_name_search_result result;
  result.ip = (caddr_t)ip;
  result.buf = buf;
  result.buf_len = buf_len;
  result.offset = offp;
  
  return linker_file_foreach(linker_foreach, &result);
}

PROTECTED int
unw_get_proc_name (unw_cursor_t *cursor, char *buf, size_t buf_len,
		   unw_word_t *offp)
{
  struct cursor *c = (struct cursor *) cursor;

  return get_proc_name (tdep_get_as (c), tdep_get_ip (c), buf, buf_len, offp,
			tdep_get_as_arg (c));
}
