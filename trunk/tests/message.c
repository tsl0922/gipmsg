#include <common.h>

void
print_message(Message *msg) {
	DEBUG_INFO("message detail:");
	printf("\tversion: %s\n\tcommandNo: %lu\n\tcommandOpts: %lu\n\t"
		"userName: %s\n\thostName: %s\n\tpacketNo: %ld\n\t"
		"attach:%s\n\tmessage: %s\n" ,
		msg->version, msg->commandNo, msg->commandOpts, 
		msg->userName, msg->hostName, msg->packetNo,
		msg->attach, msg->message);
}
int main(void) {
	bool bRetVal;
	Message msg;
	int len;
	packet_no_t packet_no;
	char *bpacket;
	
#ifdef WIN32
		WSADATA 	   hWSAData;
		memset(&hWSAData, 0, sizeof(hWSAData));
		WSAStartup(MAKEWORD(2, 0), &hWSAData);
#endif

	bpacket = build_packet(IPMSG_SENDMSG, "hello",
		"1234", &len, &packet_no);
	DEBUG_INFO("build packet:");
	printf("\t%s\n", bpacket);

	bRetVal = parse_message(0,&msg,(const char *)bpacket,len);
	DEBUG_INFO("parse packet:");
	if(bRetVal) {
		printf("\tsuccess\n");
	}
	else {
		printf("\tfail\n");
	}
	print_message(&msg);
	
	exit(EXIT_SUCCESS);
}
