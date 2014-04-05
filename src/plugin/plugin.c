#include "plugin.h"
#include "log.h"
#include "config/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

extern Config * conf;

static int plugins_num;
static Plugin *plugins;

void plugin_init(void) {
	Plugin plugin;
	char *plugin_name;
	char *filename;

	plugins_num = 0;
	plugins = NULL;

	plugin_name = strtok(conf->plugins, ",");
	do {
		log_debug_format("Loading plugin: %s\n", plugin_name);

#pragma GCC diagnostic ignored "-Wunused-result"
		if (conf->pluginpath != NULL) {
			asprintf(&filename, "%s/%s.so", conf->pluginpath, plugin_name);
		} else {
			asprintf(&filename, "%s.so", plugin_name);
		}
#pragma GCC diagnostic pop

		plugin.handle = dlopen(filename, RTLD_NOW);
		free(filename);
		if (plugin.handle == NULL) {
			log_debug_format("Cant load plugin: error: %s.\n", dlerror());
			continue;
		}
		
		plugin.init = dlsym(plugin.handle, "_plugin_init");
		if (plugin.init == NULL) {
			log_debug_format("Cant find symbol _plugin_init: error: %s\n", dlerror());
			dlclose(plugin.handle);
			continue;
		}
		
		plugin.shutdown = dlsym(plugin.handle, "_plugin_shutdown");
		if (plugin.shutdown == NULL) {
			log_debug_format("Cant find symbol _plugin_shutdown: error: %s\n", dlerror());
			dlclose(plugin.handle);
			continue;
		}
		
		// init plugin
		(*plugin.init)();

		// keep plugin struct
		plugins = (Plugin *)realloc(plugins, sizeof (Plugin) * (plugins_num + 1));
		plugins[plugins_num] = plugin;
		plugins_num++;
	} while ((plugin_name = strtok(NULL, ",")) != NULL);
}

void plugin_shutdown(void) {
	int i;
	
	for (i = 0; i < plugins_num; i++) {
		// shutdown plugin
		(*plugins[i].shutdown)();
		
		dlclose(plugins[i].handle);
	}
	free(plugins);
}
