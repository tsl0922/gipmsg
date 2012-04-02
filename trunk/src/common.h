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

#ifdef _LINUX
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <netdb.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <sys/sendfile.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <unistd.h>	
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <dirent.h>
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

#include "os_support.h"
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
#include "sendfile.h"

#endif /*_COMMON_H_*/


