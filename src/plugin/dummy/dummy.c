#include "gui/gui.h"
#include "plugin/plugin.h"
#include "log.h"

void dummy_init(void) {
	log_debug("MJS dummmy plugin started!\n");
	if (gui_ask_yes_no("Test question:")) {
		log_debug("Yes!\n");
	} else {
		log_debug("No!\n");
	}
}

void dummy_shutdown(void) {
}

PLUGIN_REGISTER(dummy_init, dummy_shutdown)
