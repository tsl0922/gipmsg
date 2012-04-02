#ifndef __MESSAGE_H__
#define __MESSAGE_H__

bool parse_message(ulong fromAddr, Message *msg, const char *msg_buf, size_t buf_len);

void build_packet(char *packet_p, const command_no_t command, const char *message, 
	const char *attach, size_t *len_p, packet_no_t *packet_no_p);

char *msg_get_version(const Message *msg);
char *msg_get_userName(const Message *msg);
char *msg_get_hostName(const Message *msg);
char *msg_get_groupName(const Message *msg);
char *msg_get_nickName(const Message *msg);
char *msg_get_encode(const Message *msg);

#endif