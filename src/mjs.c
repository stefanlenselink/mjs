#include "defs.h"
#include "mjs.h"
#include "controller/controller.h"
#include "plugin/http_controller/http_controller.h"
#include "gui/gui.h"
#include "songdata/songdata.h"
#include "engine/engine.h"
#include "config/config.h"
#include "log.h"
#include "utils.h"
//#include "plugin/plugin.h"

#include <string.h>
#include <signal.h>
#include <errno.h>

#include <time.h>
#include <locale.h>


int main(int argc, char *argv[]) {
	srand(time(NULL));

	utils_init_sig_handlers();

	//Prevent UTF-8 BUGS
	setlocale(LC_ALL,"");

	//Make sure COLS and LINES are set
	// TODO move to gui_init
	if (!initscr())
		bailout(1);

	/**
	 * Do ALL the inits here
	 */
	config_init();
	log_init();
	engine_init( );
	songdata_init( );
	controller_init( );
	http_controller_init();
	gui_init();	
	//plugin_init();
	
	//engine_jump_to("/home/hidde/Music/intro.mp3");
	log_debug("MJS started!!\n");
	gui_loop();
	
	bailout(-1);

	return 0;
}


