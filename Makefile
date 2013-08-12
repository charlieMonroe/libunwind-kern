KMOD	= libunwind
SRCS	= libunwind.c \
	  Unwind.c

.include <bsd.kmod.mk>
