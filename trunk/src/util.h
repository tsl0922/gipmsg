#ifndef __UTIL_H__
#define __UTIL_H__

#define FREE_WITH_CHECK(x) \
	if(x != NULL) free(x); x=NULL;

char *get_sys_user_name();
char *get_sys_host_name();
char *get_addr_str(ulong addr_data);
AddrInfo *get_sys_addr_info(int *len_p);
bool is_sys_host_addr(ulong ipaddr);
struct tm *get_currenttime();
void setup_sockaddr(struct sockaddr_in *sockaddr, ulong ipaddr, int port);
void format_filesize(char *buf, long filesize);
long calcuate_file_size(const char *path);
bool setup_file_info(FileInfo *info, int id, const char *path);
char *get_exe_dir();

#ifndef HAVE_STRNDUP
char *strndup(const char *src, size_t n);
#endif

#endif
