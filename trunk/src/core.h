#ifndef __CORE_H__
#define __CORE_H__

void ipmsg_core_init(const char *pAddr, int nPort);
void ipmsg_send_br_entry();
void ipmsg_send_ansentry(ulong toAddr);
void ipmsg_send_ansrecvmsg(ulong toAddr, packet_no_t packet_no);
void ipmsg_send_file_info(ulong toAddr, command_no_t optflags, const char *attach);
int ipmsg_send_packet(ulong toAddr, const char *packet, size_t packet_len);
int ipmsg_send_message(ulong toAddr, command_no_t flags, const char *message, const char *attach);


extern SOCKET udp_sock;
extern SOCKET tcp_sock;

#endif
