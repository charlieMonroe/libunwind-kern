#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

static int event_handler(struct module *module, int event, void *arg)
{
	int e = 0;
	switch (event) {
	case MOD_LOAD:
		uprintf("Libunwind loaded.\n");
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
