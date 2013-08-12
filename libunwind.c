#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include "include/libunwind.h"

static void show_backtrace(void){
	unw_cursor_t cursor;
	unw_context_t uc;
	unw_word_t ip, sp;

	unw_get_context(&uc);
	unw_init_local(&cursor, &uc);
	while (unw_step(&cursos) > 0) {
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		unw_get_reg(&cursor, UNW_REG_SP, &sp);
		uprintf("ip = %lx, sp = %lx\n", (long)ip, (long)sp);
	}
}


static int event_handler(struct module *module, int event, void *arg)
{
	int e = 0;
	switch (event) {
	case MOD_LOAD:
		uprintf("Libunwind loaded.\n");
		show_backtrace();
		break;
	case MOD_UNLOAD:
		uprintf("Libunwind unloaded.\n");
		break;
	default:
		e = EOPNOTSUPP;
		break;
	}
	return (e);
}


static moduledata_t libunwind_conf = {
	"libunwind", /* name */
	event_handler,
	NULL
};

DECLARE_MODULE(libunwind, libunwind_conf, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
