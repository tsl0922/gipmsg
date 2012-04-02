#include <common.h>

void
test_get_addr_info() 
{
	AddrInfo *info;
	int len, i;
	
#ifdef WIN32
		WSADATA 	   hWSAData;
		memset(&hWSAData, 0, sizeof(hWSAData));
		WSAStartup(MAKEWORD(2, 0), &hWSAData);
#endif

	info = get_sys_addr_info(&len);
	if(info) {
		for(i = 0;i < len; i++) {
			printf("Interface[%d]:\n", i);
			printf("\tipaddr:%s\n", get_addr_str(info[i].ipaddr));
			printf("\tnetmask:%s\n", get_addr_str(info[i].netmask));
			printf("\tbroadcast:%s\n", get_addr_str(info[i].br_addr));
		}
	}
}

void 
test_format_filesize()
{
	char buf[1000];
	format_filesize(buf,1024013);
	printf("%s\n", buf);
}

void test_calcuate_file_size() 
{
	char buf[MAX_NAMEBUF];
	long size;
	
	size = calcuate_file_size("/home/tsl0922/workspace");
	format_filesize(buf, size);
	printf("size: %s\n", buf);
}

void
test_setup_file_info(char *path) 
{
	FileInfo info;
	setup_file_info(&info, 1, path);
	printf("%s\n", info.path);
	printf("%s\n", info.name);
	printf("%ld\n", info.attr);
	printf("%ld\n", info.size);
	printf("%ld\n", info.mtime);
	printf("%ld\n", info.ctime);
	printf("%ld\n", info.atime);
}

int main(void)
{
//		test_get_addr_info();
//		test_format_filesize();
//		test_calcuate_file_size();
	test_setup_file_info("/home/tsl0922/NVIDIA-Linux-x86-290.10.run");
	test_setup_file_info("/home/tsl0922/eglibc_2.11.1.orig.tar.gz");
	test_setup_file_info("/home/tsl0922/workspace");
	return 0;
}
