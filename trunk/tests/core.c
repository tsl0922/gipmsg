#include "common.h"

void test1()
{
	SOCKET sock;
	struct sockaddr_in sockaddr;
	int opt  = 1;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1) {
		exit(-1);
	}
	
	setup_sockaddr(&sockaddr, INADDR_ANY, 2425);
	SETSOCKOPT(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
	SETSOCKOPT(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(int));
	if(BIND(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1) {
		CLOSE(sock);
		exit(-1);
	}

	char *packet;
	size_t packet_len;
	packet_no_t packet_no;
	AddrInfo *info;
	int info_len, i;

	packet = build_packet(IPMSG_BR_ENTRY | IPMSG_ABSENCEOPT, 
		config_get_nickname(), config_get_groupname(), &packet_len, &packet_no);
	printf("packet: %s\n", packet);
	info = get_sys_addr_info(&info_len);
	if(info) {
		for(i = 0;i < info_len;i++) {
			struct sockaddr_in addr;
			setup_sockaddr(&addr, info[i].br_addr, 2425);
			SENDTO(sock, packet, packet_len, 0, (const struct sockaddr *)&addr, sizeof(addr));
		}
	}
	FREE_WITH_CHECK(info);
}

void test2()
{
	int sock;
	int opt  = 1;
	struct sockaddr_in addr;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1) {
		exit(-1);
	}
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(int));
	setup_sockaddr(&addr, inet_addr("10.14.7.255"), 2425);
	sendto(sock, "hello", 5, 0, (const struct sockaddr *)&addr, sizeof(addr));
}

int main(void)
{
	test1();
//		test2();
	return 0;
}
