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
#include <common.h>

char *
get_sys_user_name() {
	char *username = NULL;
#ifdef _LINUX
	struct passwd *pwd;
	pwd = getpwuid(getuid());
	if(!pwd)
	{
		printf("Failed to get username.\n");
	}
	username = strdup(pwd->pw_name);
#endif

#ifdef WIN32
	DWORD size = 127;
	username = (char *)malloc(size);
	if(!GetUserName(username, &size)) {
		free(username);
		username = NULL;
	}
#endif
	return username;
}

char *
get_sys_host_name() {
	char *hostname = NULL;

	hostname = (char *)malloc(128);
	memset(hostname, 0, 128);
	if(GETHOSTNAME(hostname, 128) == -1)
	{
		printf("Failed to get hostname.\n");
	}
	hostname[127] = '\0';
	
	return hostname;
}

char *
get_addr_str(ulong addr_data) {
	char *addrstr = NULL;
#ifdef _LINUX
	addrstr = (char *)malloc(INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &addr_data, addrstr, INET_ADDRSTRLEN);
#endif

#ifdef WIN32
	IN_ADDR IPAddr;

	IPAddr.S_un.S_addr = addr_data;
	addrstr = (char *)inet_ntoa(IPAddr);
#endif
	return addrstr;
}

AddrInfo *
get_sys_addr_info(int *len_p) {
	AddrInfo *retAddr = NULL;
	int retLen = 0;
	int sum, i;

#ifdef _LINUX
	int sock;
	struct ifconf ifc;
	struct ifreq *ifr;
	struct sockaddr_in *addr;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1) goto error_out;
	ifc.ifc_len = 5 * sizeof(struct ifreq);//max support 5 ipaddr
	ifc.ifc_buf = (caddr_t)malloc(ifc.ifc_len);
	if(ioctl(sock, SIOCGIFCONF, &ifc) == -1)
	{
		free(ifc.ifc_buf);
		goto error_out;
	}

	sum = ifc.ifc_len / sizeof(struct ifreq);
	retAddr = (AddrInfo *)malloc(sum * sizeof(AddrInfo));
	for(i = 0;i < sum;i++)
	{
		ifr = ifc.ifc_req + i;
		if(ioctl(sock, SIOCGIFFLAGS, ifr) == -1)
			continue;
		if((ifr->ifr_flags & IFF_UP)
			&& ioctl(sock, SIOCGIFADDR, ifr) != -1) {
			addr = (struct sockaddr_in *)&ifr->ifr_addr;
			retAddr[retLen].ipaddr = (ulong)addr->sin_addr.s_addr;
		}
		if((ifr->ifr_flags & IFF_BROADCAST)
			&& ioctl(sock, SIOCGIFBRDADDR, ifr) != -1) {
			addr = (struct sockaddr_in *)&ifr->ifr_broadaddr;
			retAddr[retLen].br_addr = (ulong)addr->sin_addr.s_addr;
		}
		if(ioctl(sock, SIOCGIFNETMASK, ifr) != -1) {
			addr = (struct sockaddr_in *)&ifr->ifr_netmask;
			retAddr[retLen].netmask = (ulong)addr->sin_addr.s_addr;
		}
		retLen++;
	}
	close(sock);
#endif

#ifdef WIN32
	PMIB_IPADDRTABLE pIPAddrTable;
    DWORD dwSize = 0;
	
    pIPAddrTable = (MIB_IPADDRTABLE *)malloc(sizeof (MIB_IPADDRTABLE));
    if (pIPAddrTable) {
        if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
            pIPAddrTable = (MIB_IPADDRTABLE *) malloc(dwSize);
        }
		if (pIPAddrTable == NULL) {
            goto error_out;
        }
    }
    if (GetIpAddrTable( pIPAddrTable, &dwSize, 0) == NO_ERROR ) { 
		sum = pIPAddrTable->dwNumEntries;
        retAddr = (AddrInfo *)malloc(sum * sizeof(AddrInfo));
	    for (i=0; i < sum; i++) {
			retAddr[retLen].ipaddr    = pIPAddrTable->table[i].dwAddr;	        
			retAddr[retLen].netmask    = pIPAddrTable->table[i].dwMask;	        
			retAddr[retLen].br_addr = (pIPAddrTable->table[i].dwAddr & pIPAddrTable->table[i].dwMask) | ~pIPAddrTable->table[i].dwMask;
			retLen++;
	    }
    }
    if (pIPAddrTable) {
        free(pIPAddrTable);
        pIPAddrTable = NULL;
    }
#endif

error_out:
	*len_p = retLen;

	return retAddr;
}

bool
is_sys_host_addr(ulong ipaddr) {
	AddrInfo *info;
	int len, i;

	info = get_sys_addr_info(&len);
	if(info) {
		for(i = 0;i < len;i++) {
			if(info[i].ipaddr == ipaddr) {
				FREE_WITH_CHECK(info);
				return true;
			}
		}
	}
	FREE_WITH_CHECK(info);
	
	return false;
}

void
build_path(char *path, const char *base, const char *name)
{
	char tpath[MAX_PATH];
	char *ptr;

	if(!strcmp(name, ".")) {
		strcpy(path, base);
		return;
	}
	if(!strcmp(name ,"..")) {
		strcpy(tpath, base);
		ptr = strrchr(tpath, '/');
		*ptr = '\0';
		strcpy(path, tpath);
		return;
	}
	snprintf(tpath, MAX_PATH, "%s/%s", base, name);
	strcpy(path, tpath);
}

struct tm *
get_currenttime()
{
	time_t t;
	time(&t);
	return localtime(&t);
}

void
setup_sockaddr(struct sockaddr_in *sockaddr, ulong ipaddr, int port) {
	if(!sockaddr) return;
	
	memset(sockaddr, 0, sizeof(struct sockaddr_in));
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_addr.s_addr = ipaddr;
	sockaddr->sin_port = htons(port);
}

/**
 * devide str by set the seprator to '\0'
 * @param buf 
 * @param seprator 
 * @param handle (out)point to str after the first seprator
 * @return the str before the first seprator
 */
char *
seprate_token(const char *buf, char seprator, char **handle)
{
	char *sp, *ep;

	if(handle == NULL)
		return NULL;
	if(*handle != NULL)
		sp = *handle;
	else
		sp = buf;
	ep = strchr(sp, seprator);
	if(!ep) {
		*handle = NULL;
		return sp;
	}
	*ep = '\0';
	*handle = ++ep;

	return sp;	
}

ssize_t
xsend(SOCKET sock, const void *buf, size_t count, int flags)
{
	ssize_t size = -1;
	ssize_t n_send = 0;
	
	while(n_send != count && size != 0) {
		if((size = SEND(sock, (char *)buf + n_send, count - n_send, flags)) == -1) {
			if(errno == EINTR)
				continue;
			DEBUG_ERROR("send error!");
			return -1;
		}
		n_send += size;
	}
	return n_send;
}

ssize_t
xrecv(SOCKET sock, const void *buf, size_t count, int flags)
{
	ssize_t size = -1;
	ssize_t n_recv = 0;
	
	while(n_recv != count && size != 0) {
		if((size = RECV(sock, (char *)buf + n_recv, count - n_recv, flags)) == -1) {
			if(errno == EINTR)
				continue;
			DEBUG_ERROR("recv error!");
			return -1;
		}
		n_recv += size;
	}
	return n_recv;
}

ssize_t
xread(int fd, const void *buf, size_t count)
{
	ssize_t size = -1;
	ssize_t n_read = 0;
	
	while(n_read != count && size != 0) {
		if((size = READ(fd, (char *)buf + n_read, count - n_read)) == -1) {
			if(errno == EINTR)
				continue;
			DEBUG_ERROR("read error!");
			return -1;
		}
		n_read += size;
	}
	return n_read;
}

ssize_t
xwrite(int fd, const void *buf, size_t count)
{
	ssize_t size = -1;
	ssize_t n_write = 0;
	
	while(n_write != count && size != 0) {
		if((size = WRITE(fd, (char *)buf + n_write, count - n_write)) == -1) {
			if(errno == EINTR)
				continue;
			DEBUG_ERROR("write error!");
			return -1;
		}
		n_write += size;
	}
	return n_write;
}

void
format_filesize(char *buf, ulong filesize)
{
	double newsize = filesize;
	int i =0;
	
	while(abs(newsize) >= 1024) {
		newsize = newsize/1024;
		i++;
		if(i == 4) break;
	}

	char *array[5] = {"B","KB","MB","GB","TB"};
	sprintf(buf, "%.2f %s", newsize, array[i]);
}

char *
get_exe_dir() {
	char *dir;
#ifdef WIN32
	TCHAR buf[1024];
	DWORD len;
	if(!(len = GetModuleFileName(NULL, buf, 1024)))
		return NULL;
	buf[len] = '\0';
	dir = strdup(buf);
#endif

#ifdef _LINUX
	char buf[1024];
	ssize_t len;
	if((len = readlink("/proc/self/exe", buf, 1024)) == -1)
		return NULL;
	buf[len] = '\0';
	dir = strdup(buf);
#endif

	return dir;
}

#ifndef HAVE_STRNDUP
char *
strndup(const char *src, size_t n) {
	char *dst;
	
	if (src) {
		dst = (char *)malloc(n);
		strncpy (dst, src, n);
		dst[n] = '\0';
	}
	else
		dst = NULL;

	return dst;
}
#endif