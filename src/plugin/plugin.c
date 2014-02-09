#include "mjs.h"
#include "plugin.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

static int plugins_num;
static Plugin *plugins;

void plugin_init(void) {
	int i;
	char *filename;
	
	plugins_num = conf->plugins_num;
	plugins = calloc(plugins_num, sizeof(Plugin));
	
	for (i = 0; i < plugins_num; i++) {
		log_debug_format("Loading plugin: %s\n", conf->plugins[i]);
		
		if (conf->plugin_dir != NULL) {
			asprintf(&filename, "%s/%s.so", conf->plugin_dir, conf->plugins[i]);
		} else {
			asprintf(&filename, "%s.so", conf->plugins[i]);
		}
		plugins[i].handle = dlopen(filename, RTLD_NOW);
		free(filename);
		if (plugins[i].handle == NULL) {
			log_debug_format("Cant load plugin: error: %s.\n", dlerror());
			continue;
		}
		
		plugins[i].init = dlsym(plugins[i].handle, "_plugin_init");
		if (plugins[i].init == NULL) {
			log_debug_format("Cant find symbol _plugin_init: error: %s\n", dlerror());
			dlclose(plugins[i].handle);
			plugins[i].handle = NULL;
			continue;
		}
		
		plugins[i].shutdown = dlsym(plugins[i].handle, "_plugin_shutdown");
		if (plugins[i].init == NULL) {
			log_debug_format("Cant find symbol _plugin_shutdown: error: %s\n", dlerror());
			dlclose(plugins[i].handle);
			plugins[i].handle = NULL;
			continue;
		}
		
		// init plugin
		(*plugins[i].init)();
	}
}

void plugin_shutdown(void) {
	int i;
	
	for (i = 0; i < plugins_num; i++) {
		if (plugins[i].handle == NULL) {
			continue;
		}
		
		// shutdown plugin
		(*plugins[i].shutdown)();
		
		dlclose(plugins[i].handle);
	}
	free(plugins);
}
