/* libunwind - a platform-independent unwind library
   Copyright (C) 2007 Google, Inc
	Contributed by Arun Sharma <arun.sharma@google.com>
   Copyright (C) 2010 Konstantin Belousov <kib@freebsd.org>

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

#include "ucontext_i.h"
#if defined __linux__
#include <asm/unistd.h>
#define	SIG_SETMASK   2
#define	SIGSET_BYTE_SIZE   (64/8)
#elif defined __FreeBSD__
#include <sys/syscall.h>
#endif

/*  int _Ux86_64_setcontext (const ucontext_t *ucp)

  Restores the machine context provided.
  Unlike the libc implementation, doesn't clobber %rax
  
*/
	.global _Ux86_64_setcontext
	.type _Ux86_64_setcontext, @function

_Ux86_64_setcontext:

	/* restore the rest of the state */
	movq    UC_MCONTEXT_GREGS_R8(%rdi),%r8
	movq    UC_MCONTEXT_GREGS_R9(%rdi),%r9
	movq    UC_MCONTEXT_GREGS_RBX(%rdi),%rbx
	movq    UC_MCONTEXT_GREGS_RBP(%rdi),%rbp
	movq    UC_MCONTEXT_GREGS_R12(%rdi),%r12
	movq    UC_MCONTEXT_GREGS_R13(%rdi),%r13
	movq    UC_MCONTEXT_GREGS_R14(%rdi),%r14
	movq    UC_MCONTEXT_GREGS_R15(%rdi),%r15
	movq    UC_MCONTEXT_GREGS_RSI(%rdi),%rsi
	movq    UC_MCONTEXT_GREGS_RDX(%rdi),%rdx
	movq    UC_MCONTEXT_GREGS_RAX(%rdi),%rax
	movq    UC_MCONTEXT_GREGS_RCX(%rdi),%rcx
	movq    UC_MCONTEXT_GREGS_RSP(%rdi),%rsp

        /* push the return address on the stack */
	movq    UC_MCONTEXT_GREGS_RIP(%rdi),%rcx
	pushq   %rcx

	movq    UC_MCONTEXT_GREGS_RCX(%rdi),%rcx
	movq    UC_MCONTEXT_GREGS_RDI(%rdi),%rdi
	retq

	.size _Ux86_64_setcontext, . - _Ux86_64_setcontext

      /* We do not need executable stack.  */
      .section        .note.GNU-stack,"",@progbits
