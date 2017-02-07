#include "gui/gui.h"
#include "plugin/plugin.h"
#include "log.h"

void dummy_init(void) {
	log_debug("dummy: started!\n");
	if (gui_ask_yes_no("Test question:")) {
		log_debug("dummy: Yes!\n");
	} else {
		log_debug("dummy: No!\n");
	}
}

void dummy_shutdown(void) {
}

PLUGIN_REGISTER(dummy_init, dummy_shutdown)
