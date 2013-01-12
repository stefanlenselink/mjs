#ifndef _plugin_h
#define _plugin_h

#define PLUGIN_REGISTER(init, shutdown) \
	void _plugin_init(void) { init(); } \
	void _plugin_shutdown(void) { shutdown(); }


typedef struct _Plugin {
	void *handle;
	void (*init)(void);
	void (*shutdown)(void);
} Plugin;

void plugin_init(void);
void plugin_shutdown(void);

#endif /* _plugin_h */
