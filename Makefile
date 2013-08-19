CFLAGS  += -O0
KMOD	= libunwind
SRCS	= libunwind.c \
		Unwind.c \
		getcontext.S \
		is_fpreg.c \
		Lcreate_addr_space.c \
		Lget_proc_info.c \
		Lget_save_loc.c \
		Lglobal.c \
		Linit_local.c \
		Linit.c \
		longjmp.S \
		Los-freebsd.c \
		Lregs.c \
		Lresume.c \
		Lstash_frame.c \
		Lstep.c \
		regname.c \
		setcontext.S \
		siglongjmp.S \
		_ReadSLEB.c \
		_ReadULEB.c \
		backtrace.c \
		dyn-cancel.c \
		dyn-register.c \
		flush_cache.c \
		init.c \
		Ldestroy_addr_space.c \
		Ldyn-extract.c \
		Lfind_dynamic_proc_info.c \
		Lget_accessors.c \
		Lget_fpreg.c \
		Lget_proc_info_by_ip.c \
		Lget_proc_name.c \
		Lget_reg.c \
		Lput_dynamic_unwind_info.c \
		Lset_caching_policy.c \
		Lset_fpreg.c \
		Lset_reg.c \
		mempool.c \
		strerror.c \
		malloc_types.c \
		dwarf_Lstep.c \
		global.c \
		Ltrace.c \
		Lexpr.c \
		Lfde.c \
		Lfind_proc_info-lsb.c \
		Lfind_unwind_table.c \
		Lparser.c \
		Lpe.c

.include <bsd.kmod.mk>
