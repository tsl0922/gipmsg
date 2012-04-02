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

void
format_filesize(char *buf, long filesize)
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

long
calcuate_file_size(const char *path) {//include hidden files, links
	DIR *dir = NULL;
	struct stat st;
	struct dirent *dt;
	char tpath[MAX_BUF];
	long size = 0;

	if(stat(path, &st) == -1) return 0;
	if(S_ISDIR(st.st_mode)) {
		dir = opendir(path);
		while(dir && (dt = readdir(dir))) {
			if (!strcmp(dt->d_name, ".") || !strcmp(dt->d_name, ".."))
				continue;
			//construct full path
			sprintf(tpath, "%s/%s", path, dt->d_name);
			//user recursion
			size += calcuate_file_size(tpath);
		}
		if(dir) closedir(dir);
	}
	else {
		size += st.st_size;
	}
	return size;
}

bool
setup_file_info(FileInfo *info, int id, const char *path)
{
	struct stat st;
	char *sp;

	if(lstat(path, &st) == -1) return false;
	info->id = id;
	strcpy(info->path, path);
	sp = strrchr(path, '/');
	if(sp)
		strcpy(info->name, sp + 1);
	else
		strcpy(info->name, path);
	info->mtime = st.st_mtime;
	info->atime = st.st_atime;
	info->ctime = st.st_ctime;
	info->size = S_ISDIR(st.st_mode)?0:st.st_size;
	
	info->attr = S_ISREG(st.st_mode)?IPMSG_FILE_REGULAR:0;
	info->attr |= S_ISDIR(st.st_mode)?IPMSG_FILE_DIR:0;
	info->attr |= S_ISCHR(st.st_mode)?IPMSG_FILE_CDEV:0;
	info->attr |= S_ISFIFO(st.st_mode)?IPMSG_FILE_FIFO:0;
	info->attr |= S_ISLNK(st.st_mode)?IPMSG_FILE_SYMLINK:0;

	return true;
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