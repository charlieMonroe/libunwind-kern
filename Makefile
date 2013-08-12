KMOD	= libunwind
SRCS	= libunwind.c \
		Unwind.c \
		Gcreate_addr_space.c \
		getcontext.S \
		Gget_proc_info.c \
		Gget_save_loc.c \
		Gglobal.c \
		Ginit_local.c \
		Ginit.c \
		Gos-freebsd.c \
		Gregs.c \
		Gresume.c \
		Gstash_frame.c \
		Gstep.c \
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
		siglongjmp.S

.include <bsd.kmod.mk>
