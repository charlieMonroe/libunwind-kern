#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/types.h>
#include <sys/systm.h>


#include "include/libunwind.h"

static void show_backtrace(void){
	unw_cursor_t cursor;
	unw_context_t uc;
	unw_word_t ip, sp, offset;

	unw_getcontext(&uc);
	printf("Got context\n");
	unw_init_local(&cursor, &uc);
	printf("Inited local\n");
	while (unw_step(&cursor) > 0) {
		char fname[64];
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		unw_get_reg(&cursor, UNW_REG_SP, &sp);
		fname[0] = '\0';
		unw_get_proc_name(&cursor, fname, sizeof(fname), &offset);
		printf("ip = %lx [%s+0x%lx], sp = %lx\n", (long)ip, fname, (long)offset, (long)sp);
	}
}


static int event_handler(struct module *module, int event, void *arg)
{
	int e = 0;
	switch (event) {
	case MOD_LOAD:
		printf("Libunwind loaded.\n");
		show_backtrace();
		break;
	case MOD_UNLOAD:
		printf("Libunwind unloaded.\n");
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
MODULE_VERSION(libunwind, 0);
