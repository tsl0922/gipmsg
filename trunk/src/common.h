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

#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>	
#include <fcntl.h>
#include <dirent.h>

#ifdef _LINUX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <netdb.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <sys/sendfile.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <pwd.h>
#endif

#ifdef WIN32
#include <windows.h>
#include <io.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>

#ifdef ENABLE_NLS
#include <locale.h>
#include <glib/gi18n.h>
#else
#define  _(x) (x)
#define N_(x) (x)
#endif

#include "os-support.h"
#include "debug.h"
#include "ipmsg.h"
#include "ipmsg-types.h"
#include "util.h"
#include "myconfig.h"
#include "user.h"
#include "core.h"
#include "gipmsg-types.h"
#include "mainwin.h"
#include "senddlg.h"
#include "emotion.h"
#include "fileshare.h"

#endif /*_COMMON_H_*/

