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

#ifndef __UTIL_H__
#define __UTIL_H__

#define FREE_WITH_CHECK(x) \
	if(x != NULL) free(x); x=NULL;
char *get_sys_user_name();
char *get_sys_host_name();
char *get_addr_str(ulong addr_data);
AddrInfo *get_sys_addr_info(int *len_p);
bool is_sys_host_addr(ulong ipaddr);
void build_path(char *path, const char *base, const char *name);
struct tm *get_currenttime();
void setup_sockaddr(struct sockaddr_in *sockaddr, ulong ipaddr, int port);
char *seprate_token(const char *buf, char seprator, char **offset_p);

ssize_t xsend(SOCKET sock, const void *buf, size_t count, int flags);
ssize_t xrecv(SOCKET sock, const void *buf, size_t count, int flags);
ssize_t xread(int fd, const void *buf, size_t count);
ssize_t xwrite(int fd, const void *buf, size_t count);

void format_filesize(char *buf, ulong filesize);

char *get_exe_dir();

#ifndef HAVE_STRNDUP
char *strndup(const char *src, size_t n);
#endif

#endif
