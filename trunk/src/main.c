/* Gipmsg - Local networt message communication software
 *  
 * Copyright (C) 2012 tsl0922<tsl0922@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "common.h"

init()
{
	if(!socket_init(pAddr, nPort))
		return;
	g_thread_create((GThreadFunc)udp_server_thread, NULL, FALSE, NULL);
	g_thread_create((GThreadFunc)tcp_server_thread, NULL, FALSE, NULL);
		
	//tell everyone I am online.
	ipmsg_send_br_entry();
	bool ipmsg_core_init(const char *pAddr, int nPort);
}

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

