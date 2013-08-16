/* libunwind - a platform-independent unwind library
   Copyright (C) 2003-2004 Hewlett-Packard Co
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

#include "unwind-internal.h"

PROTECTED _Unwind_Reason_Code
_Unwind_Backtrace (_Unwind_Trace_Fn trace, void *trace_parameter)
{
  struct _Unwind_Context context;
  unw_context_t uc;
  int ret;

  if (_Unwind_InitContext (&context, &uc) < 0)
    return _URC_FATAL_PHASE1_ERROR;

  /* Phase 1 (search phase) */

  while (1)
    {
      if ((ret = unw_step (&context.cursor)) <= 0)
	{
	  if (ret == 0)
	    return _URC_END_OF_STACK;
	  else
	    return _URC_FATAL_PHASE1_ERROR;
	}

      if ((*trace) (&context, trace_parameter) != _URC_NO_REASON)
	return _URC_FATAL_PHASE1_ERROR;
    }
}

_Unwind_Reason_Code __libunwind_Unwind_Backtrace (_Unwind_Trace_Fn, void *)
     ALIAS (_Unwind_Backtrace);


PROTECTED void
_Unwind_DeleteException (struct _Unwind_Exception *exception_object)
{
  _Unwind_Exception_Cleanup_Fn cleanup = exception_object->exception_cleanup;
  
  if (cleanup)
    (*cleanup) (_URC_FOREIGN_EXCEPTION_CAUGHT, exception_object);
}

void __libunwind_Unwind_DeleteException (struct _Unwind_Exception *)
ALIAS (_Unwind_DeleteException);

PROTECTED void *
_Unwind_FindEnclosingFunction (void *ip)
{
  unw_proc_info_t pi;
  
  if (unw_get_proc_info_by_ip (unw_local_addr_space,
                               (unw_word_t) (uintptr_t) ip, &pi, 0)
      < 0)
    return NULL;
  
  return (void *) (uintptr_t) pi.start_ip;
}

void *__libunwind_Unwind_FindEnclosingFunction (void *)
ALIAS (_Unwind_FindEnclosingFunction);



PROTECTED _Unwind_Reason_Code
_Unwind_ForcedUnwind (struct _Unwind_Exception *exception_object,
                      _Unwind_Stop_Fn stop, void *stop_parameter)
{
  struct _Unwind_Context context;
  unw_context_t uc;
  
  /* We check "stop" here to tell the compiler's inliner that
   exception_object->private_1 isn't NULL when calling
   _Unwind_Phase2().  */
  if (!stop)
    return _URC_FATAL_PHASE2_ERROR;
  
  if (_Unwind_InitContext (&context, &uc) < 0)
    return _URC_FATAL_PHASE2_ERROR;
  
  exception_object->private_1 = (unsigned long) stop;
  exception_object->private_2 = (unsigned long) stop_parameter;
  
  return _Unwind_Phase2 (exception_object, &context);
}

_Unwind_Reason_Code __libunwind_Unwind_ForcedUnwind (struct _Unwind_Exception*,
                                                     _Unwind_Stop_Fn, void *)
ALIAS (_Unwind_ForcedUnwind);


PROTECTED unsigned long
_Unwind_GetBSP (struct _Unwind_Context *context)
{
#ifdef UNW_TARGET_IA64
  unw_word_t val;
  
  unw_get_reg (&context->cursor, UNW_IA64_BSP, &val);
  return val;
#else
  return 0;
#endif
}

unsigned long __libunwind_Unwind_GetBSP (struct _Unwind_Context *)
ALIAS (_Unwind_GetBSP);

PROTECTED unsigned long
_Unwind_GetCFA (struct _Unwind_Context *context)
{
  unw_word_t val;
  
  unw_get_reg (&context->cursor, UNW_REG_SP, &val);
  return val;
}

unsigned long __libunwind_Unwind_GetCFA (struct _Unwind_Context *)
ALIAS (_Unwind_GetCFA);


PROTECTED unsigned long
_Unwind_GetDataRelBase (struct _Unwind_Context *context)
{
  unw_proc_info_t pi;
  
  pi.gp = 0;
  unw_get_proc_info (&context->cursor, &pi);
  return pi.gp;
}

unsigned long __libunwind_Unwind_GetDataRelBase (struct _Unwind_Context *)
ALIAS (_Unwind_GetDataRelBase);

PROTECTED unsigned long
_Unwind_GetGR (struct _Unwind_Context *context, int index)
{
  unw_word_t val;
  
  if (index == UNW_REG_SP && context->end_of_stack)
  /* _Unwind_ForcedUnwind() requires us to return a NULL
   stack-pointer after reaching the end of the stack.  */
    return 0;
  
  unw_get_reg (&context->cursor, index, &val);
  return val;
}

unsigned long __libunwind_Unwind_GetGR (struct _Unwind_Context *, int)
ALIAS (_Unwind_GetGR);

PROTECTED unsigned long
_Unwind_GetIP (struct _Unwind_Context *context)
{
  unw_word_t val;
  
  unw_get_reg (&context->cursor, UNW_REG_IP, &val);
  return val;
}

unsigned long __libunwind_Unwind_GetIP (struct _Unwind_Context *)
ALIAS (_Unwind_GetIP);

PROTECTED unsigned long
_Unwind_GetIPInfo (struct _Unwind_Context *context, int *ip_before_insn)
{
  unw_word_t val;
  
  unw_get_reg (&context->cursor, UNW_REG_IP, &val);
  *ip_before_insn = unw_is_signal_frame (&context->cursor);
  return val;
}

unsigned long __libunwind_Unwind_GetIPInfo (struct _Unwind_Context *, int *)
ALIAS (_Unwind_GetIPInfo);

PROTECTED unsigned long
_Unwind_GetLanguageSpecificData (struct _Unwind_Context *context)
{
  unw_proc_info_t pi;
  
  pi.lsda = 0;
  unw_get_proc_info (&context->cursor, &pi);
  return pi.lsda;
}

unsigned long
__libunwind_Unwind_GetLanguageSpecificData (struct _Unwind_Context *)
ALIAS (_Unwind_GetLanguageSpecificData);

PROTECTED unsigned long
_Unwind_GetRegionStart (struct _Unwind_Context *context)
{
  unw_proc_info_t pi;
  
  pi.start_ip = 0;
  unw_get_proc_info (&context->cursor, &pi);
  return pi.start_ip;
}

unsigned long __libunwind_Unwind_GetRegionStart (struct _Unwind_Context *)
ALIAS (_Unwind_GetRegionStart);

PROTECTED unsigned long
_Unwind_GetTextRelBase (struct _Unwind_Context *context)
{
  return 0;
}

unsigned long __libunwind_Unwind_GetTextRelBase (struct _Unwind_Context *)
ALIAS (_Unwind_GetTextRelBase);

_Unwind_Personality_Fn __libunwind_default_personality;

PROTECTED _Unwind_Reason_Code
_Unwind_RaiseException (struct _Unwind_Exception *exception_object)
{
  uint64_t exception_class = exception_object->exception_class;
  _Unwind_Personality_Fn personality;
  struct _Unwind_Context context;
  _Unwind_Reason_Code reason;
  unw_proc_info_t pi;
  unw_context_t uc;
  unw_word_t ip;
  int ret;
  
  Debug (1, "(exception_object=%p)\n", exception_object);
  
  if (_Unwind_InitContext (&context, &uc) < 0)
    return _URC_FATAL_PHASE1_ERROR;
  
  /* Phase 1 (search phase) */
  
  while (1)
  {
    if ((ret = unw_step (&context.cursor)) <= 0)
    {
      if (ret == 0)
	    {
	      Debug (1, "no handler found\n");
	      return _URC_END_OF_STACK;
	    }
      else
        return _URC_FATAL_PHASE1_ERROR;
    }
    
    if (unw_get_proc_info (&context.cursor, &pi) < 0)
      return _URC_FATAL_PHASE1_ERROR;
    
    personality = (_Unwind_Personality_Fn) (uintptr_t) pi.handler;
    if (personality != NULL){
      reason = (*personality) (_U_VERSION, _UA_SEARCH_PHASE,
                               exception_class, exception_object,
                               &context);
      if (reason != _URC_CONTINUE_UNWIND)
	    {
	      if (reason == _URC_HANDLER_FOUND)
          break;
	      else
        {
          Debug (1, "personality returned %d\n", reason);
          return _URC_FATAL_PHASE1_ERROR;
        }
	    }
    }else{
      Debug(1, "no personality!\n");
    }
  }
  
  /* Exceptions are associated with IP-ranges.  If a given exception
   is handled at a particular IP, it will _always_ be handled at
   that IP.  If this weren't true, we'd have to track the tuple
   (IP,SP,BSP) to uniquely identify the stack frame that's handling
   the exception.  */
  if (unw_get_reg (&context.cursor, UNW_REG_IP, &ip) < 0)
    return _URC_FATAL_PHASE1_ERROR;
  exception_object->private_1 = 0;	/* clear "stop" pointer */
  exception_object->private_2 = ip;	/* save frame marker */
  
  Debug (-1, "found handler for IP=%lx; entering cleanup phase\n", (long) ip);
  
  /* Reset the cursor to the first frame: */
  if (unw_init_local (&context.cursor, &uc) < 0)
    return _URC_FATAL_PHASE1_ERROR;
  
  return _Unwind_Phase2 (exception_object, &context);
}

_Unwind_Reason_Code
__libunwind_Unwind_RaiseException (struct _Unwind_Exception *)
ALIAS (_Unwind_RaiseException);

PROTECTED _Unwind_Reason_Code
_Unwind_Resume_or_Rethrow (struct _Unwind_Exception *exception_object)
{
  struct _Unwind_Context context;
  unw_context_t uc;
  
  if (exception_object->private_1)
  {
    if (_Unwind_InitContext (&context, &uc) < 0)
      return _URC_FATAL_PHASE2_ERROR;
    
    return _Unwind_Phase2 (exception_object, &context);
  }
  else
    return _Unwind_RaiseException (exception_object);
}

_Unwind_Reason_Code
__libunwind_Unwind_Resume_or_Rethrow (struct _Unwind_Exception *)
ALIAS (_Unwind_Resume_or_Rethrow);

PROTECTED void
_Unwind_Resume (struct _Unwind_Exception *exception_object)
{
  struct _Unwind_Context context;
  unw_context_t uc;
  
  if (_Unwind_InitContext (&context, &uc) < 0)
    abort ();
  
  _Unwind_Phase2 (exception_object, &context);
  abort ();
}

void __libunwind_Unwind_Resume (struct _Unwind_Exception *)
ALIAS (_Unwind_Resume);

PROTECTED void
_Unwind_SetIP (struct _Unwind_Context *context, unsigned long new_value)
{
  unw_set_reg (&context->cursor, UNW_REG_IP, new_value);
}

void __libunwind_Unwind_SetIP (struct _Unwind_Context *, unsigned long)
ALIAS (_Unwind_SetIP);


#ifdef UNW_TARGET_X86
#include "include/dwarf_i.h"
#endif

PROTECTED void
_Unwind_SetGR (struct _Unwind_Context *context, int index,
               unsigned long new_value)
{
#ifdef UNW_TARGET_X86
  index = dwarf_to_unw_regnum(index);
#endif
  unw_set_reg (&context->cursor, index, new_value);
#ifdef UNW_TARGET_IA64
  if (index >= UNW_IA64_GR && index <= UNW_IA64_GR + 127)
  /* Clear the NaT bit. */
    unw_set_reg (&context->cursor, UNW_IA64_NAT + (index - UNW_IA64_GR), 0);
#endif
}

void __libunwind_Unwind_SetGR (struct _Unwind_Context *, int, unsigned long)
ALIAS (_Unwind_SetGR);



