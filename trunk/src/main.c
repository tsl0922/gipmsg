#include "common.h"

int
main(int argc, char *argv[]) {	
	DEBUG_INFO("Start");
#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	bindtextdomain(GETTEXT_PACKAGE , NULL);
	textdomain(GETTEXT_PACKAGE);
#endif

	if(!g_thread_supported())
		g_thread_init(NULL);
	gdk_threads_init();

	gtk_init(&argc, &argv);
	ipmsg_ui_init();
	ipmsg_core_init(NULL, 0);
	gtk_main();
	DEBUG_INFO("End");

	exit(EXIT_SUCCESS);
}

