/* Gipmsg - Local networt message communication software
 *  
 * Copyright (C) 2012 tsl0922<tsl0922@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "common.h"

SOCKET udp_sock;
SOCKET tcp_sock;
bool server = true;

static void *udp_server_thread(void *data);
static void *tcp_server_thread(void *data);
static void ipmsg_dispatch_message(SOCKET sock, Message * msg);

void ipmsg_core_init()
{
	pthread_t udp_thread;
	pthread_t tcp_thread;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	
	pthread_create(&udp_thread, &attr, udp_server_thread, NULL);
	pthread_create(&tcp_thread, &attr, tcp_server_thread, NULL);
	
	pthread_attr_destroy(&attr);

	//tell everyone I am online.
	ipmsg_send_br_entry();
}

/*
 * create and bind tcp/udp port using the given IP address and port
 * @param pAddr
 * @param nPort
 */
bool socket_init(const char *pAddr, int nPort)
{
	struct sockaddr_in sockaddr;
	struct hostent *pHostent;
	int opt = 1;
#ifdef WIN32
	ULONG inAddr;
	WSADATA hWSAData;
	memset(&hWSAData, 0, sizeof(hWSAData));
	WSAStartup(MAKEWORD(2, 0), &hWSAData);
#else
	in_addr_t inAddr;
#endif

	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	if ((pAddr == NULL) || (!strlen((const char *)pAddr))) {
		sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		if ((inAddr = inet_addr((const char *)pAddr)) != INADDR_NONE) {
			sockaddr.sin_addr.s_addr = inAddr;
		}
	}
	if (nPort < 1000)
		nPort = IPMSG_DEFAULT_PORT;
	sockaddr.sin_port = htons(nPort);

	udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
	tcp_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (udp_sock == -1 || tcp_sock == -1) {
		DEBUG_ERROR("can not create socket!\n");
		return false;
	}
	SETSOCKOPT(udp_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
	SETSOCKOPT(udp_sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(int));
	SETSOCKOPT(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

	if (BIND(tcp_sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1
	    || BIND(udp_sock, (struct sockaddr *)&sockaddr,
		    sizeof(sockaddr)) == -1) {
		DEBUG_ERROR("bind error!");
		CLOSE(udp_sock);
		CLOSE(tcp_sock);
		return false;
	}

	return true;
}

static char *check_encode(char *buf, size_t buf_len, 
	char *fencode, char **encode_p, size_t *len_p)
{
#define NULL_OBJECT 0x02
	char *ptr;
	char *outstr;
	char *encode;
	size_t size;

	/* replace NULL terminator('\0') with ASCII character */
	ptr = buf + strlen(buf) + 1;
	while ((size_t)(ptr - buf) <= buf_len) {
		*(ptr - 1) = NULL_OBJECT;
		ptr += strlen(ptr) + 1;
	}
	
	if(fencode && !strcasecmp("utf-8", fencode)) {
		encode =  strdup("utf-8");
		outstr = strdup(buf);
	}
	else if(fencode && (outstr = convert_encode("utf-8", 
			fencode, buf, buf_len))) {
		encode = strdup(fencode);
	}
	else {
		outstr = string_validate(buf, buf_len, 
			(const char *)config_get_candidate_encode(), &encode);
	}
	
	size = strlen(outstr);

	/* restore ASCII character  to NULL terminator('\0') */
	ptr = outstr;
	while ((ptr = (char *)memchr(ptr, NULL_OBJECT, outstr + size - ptr))) {
		*ptr = '\0';
		ptr++;
	}
	*len_p = size;
	*encode_p = encode;

	return outstr;
}

static void *udp_server_thread(void *data)
{
	char buffer[MAX_UDPBUF];
	struct sockaddr_in addr;
	socklen_t addr_len;
	int bytesRecieved;

	DEBUG_INFO("Initialize udp server on port %d...", config_get_port());
	addr_len = sizeof(struct sockaddr_in);
	while (server) {
		Message msg;
		char *tstring;
		size_t len;

		if ((bytesRecieved = RECVFROM(udp_sock, buffer, MAX_UDPBUF,
					      0, (struct sockaddr *)&addr,
					      &addr_len)) <= 0)
			continue;
		DEBUG_INFO("recieved packet:%s", buffer);
		tstring = check_encode(buffer, bytesRecieved, 
			config_get_default_encode(), &msg.encode, &len);
		
		if (parse_message
		    (addr.sin_addr.s_addr, &msg, (const char *)tstring, len)) {
			ipmsg_dispatch_message(udp_sock, &msg);
			free_message_data(&msg);
		}
		FREE_WITH_CHECK(tstring);
	}
	pthread_exit(NULL);
}

static void *tcp_server_thread(void *data)
{
	struct sockaddr_in addr;
	socklen_t addr_len;
	SOCKET client_sock = -1;

	DEBUG_INFO("Initialize tcp server...");
	addr_len = sizeof(struct sockaddr_in);
	if (listen(tcp_sock, 5)) {
		DEBUG_INFO("listen error!");
		pthread_exit(NULL);
	}
	while (server) {
		if ((client_sock =
		     ACCEPT(tcp_sock, (struct sockaddr *)&addr, &addr_len)) < 0)
			continue;
		DEBUG_INFO("new client!sockfd: %d", client_sock);
		tcp_request_entry(client_sock, addr.sin_addr.s_addr);
	}
	pthread_exit(NULL);
}

static void ipmsg_proc_br_entry(SOCKET sock, Message * msg)
{
	if (!is_sys_host_addr(msg->fromAddr)) {
		ipmsg_send_ansentry(msg->fromAddr);
	}
	if(!find_user(msg->fromAddr)) {
		User *user = add_user(msg);
		gdk_threads_enter();
		user_tree_add_user(user);
		gdk_threads_leave();
	}
	print_user_list();
}

static void ipmsg_proc_br_exit(SOCKET sock, Message * msg)
{
	if(find_user(msg->fromAddr)) {
		gdk_threads_enter();
		user_tree_del_user(msg->fromAddr);
		gdk_threads_leave();
		del_user(msg);
	}
	print_user_list();
}

static void ipmsg_proc_ans_entry(SOCKET sock, Message * msg)
{
	if(!find_user(msg->fromAddr)) {
		User *user = add_user(msg);
		gdk_threads_enter();
		user_tree_add_user(user);
		gdk_threads_leave();
	}
	print_user_list();
}

static void ipmsg_proc_br_absence(SOCKET sock, Message * msg)
{
	User *user = NULL;
	
	user = update_user(msg);
	if(user) {
		gdk_threads_enter();
		user_tree_update_user(user);
		gdk_threads_leave();
	}
	print_user_list();
}

static void ipmsg_proc_br_isgetlist(SOCKET sock, Message * msg)
{
}

static void ipmsg_proc_getlist(SOCKET sock, Message * msg)
{
}

static void ipmsg_proc_okgetlist(SOCKET sock, Message * msg)
{
}

static void ipmsg_proc_anslist(SOCKET sock, Message * msg)
{
}

static void ipmsg_proc_send_msg(SOCKET sock, Message * msg)
{
	if (msg->commandOpts & IPMSG_SENDCHECKOPT) {
		ipmsg_send_ansrecvmsg(msg->fromAddr, msg->packetNo);
	}
	gdk_threads_enter();
	notify_sendmsg(msg);
	gdk_threads_leave();
}

static void ipmsg_proc_recv_msg(SOCKET sock, Message * msg)
{
	packet_no_t packet_no;

	packet_no = strtoll(msg->message, (char **)NULL, 10);
	gdk_threads_enter();
	notify_recvmsg(msg->fromAddr, packet_no);
	gdk_threads_leave();
}

static void ipmsg_proc_read_msg(SOCKET sock, Message * msg)
{
}

static void ipmsg_proc_get_info(SOCKET sock, Message * msg)
{
}

static void ipmsg_proc_send_info(SOCKET sock, Message * msg)
{
}

static void ipmsg_proc_get_absence_info(SOCKET sock, Message * msg)
{
}

static void ipmsg_proc_send_absence_info(SOCKET sock, Message * msg)
{
}

static void ipmsg_proc_release_files_msg(SOCKET sock, Message * msg)
{
}

static void ipmsg_proc_get_public_key(SOCKET sock, Message * msg)
{
}

static void ipmsg_proc_anspubkey(SOCKET sock, Message * msg)
{
}

static void ipmsg_dispatch_message(SOCKET sock, Message * msg)
{
	if (!msg)
		return;

	switch (msg->commandNo) {
	case IPMSG_NOOPERATION:
		break;
	case IPMSG_BR_ENTRY:
		DEBUG_INFO("Dispatch br_entry\n");
		ipmsg_proc_br_entry(sock, msg);
		break;
	case IPMSG_BR_EXIT:
		DEBUG_INFO("Dispatch br_exit\n");
		ipmsg_proc_br_exit(sock, msg);
		break;
	case IPMSG_ANSENTRY:
		DEBUG_INFO("Dispatch ans_entry\n");
		ipmsg_proc_ans_entry(sock, msg);
		break;
	case IPMSG_BR_ABSENCE:
		DEBUG_INFO("Dispatch br_absense\n");
		ipmsg_proc_br_absence(sock, msg);
		break;
	case IPMSG_BR_ISGETLIST:
		DEBUG_INFO("Dispatch isget_list\n");
		ipmsg_proc_br_isgetlist(sock, msg);
		break;
	case IPMSG_OKGETLIST:
		DEBUG_INFO("Dispatch okget_list\n");
		ipmsg_proc_okgetlist(sock, msg);
		break;
	case IPMSG_GETLIST:
		DEBUG_INFO("Dispatch get_list\n");
		ipmsg_proc_getlist(sock, msg);
		break;
	case IPMSG_ANSLIST:
		DEBUG_INFO("Dispatch ans_list\n");
		ipmsg_proc_anslist(sock, msg);
		break;
	case IPMSG_SENDMSG:
		DEBUG_INFO("Dispatch send_message\n");
		ipmsg_proc_send_msg(sock, msg);
		break;
	case IPMSG_RECVMSG:
		DEBUG_INFO("Dispatch recv_message\n");
		ipmsg_proc_recv_msg(sock, msg);
		break;
	case IPMSG_READMSG:
		DEBUG_INFO("Dispatch read_message\n");
		ipmsg_proc_read_msg(sock, msg);
		break;
	case IPMSG_DELMSG:
		DEBUG_INFO("Dispatch delete_message\n");
		break;
	case IPMSG_ANSREADMSG:
		DEBUG_INFO("Dispatch ans_read_message\n");
		break;
	case IPMSG_GETINFO:
		DEBUG_INFO("Dispatch get_info\n");
		ipmsg_proc_get_info(sock, msg);
		break;
	case IPMSG_SENDINFO:
		DEBUG_INFO("Dispatch send_info\n");
		ipmsg_proc_send_info(sock, msg);
		break;
	case IPMSG_GETABSENCEINFO:
		DEBUG_INFO("Dispatch get_absence_info\n");
		ipmsg_proc_get_absence_info(sock, msg);
		break;
	case IPMSG_SENDABSENCEINFO:
		DEBUG_INFO("Dispatch send_absence_info\n");
		ipmsg_proc_send_absence_info(sock, msg);
		break;
	case IPMSG_RELEASEFILES:
		DEBUG_INFO("Dispatch release files\n");
		ipmsg_proc_release_files_msg(sock, msg);
		break;
	case IPMSG_GETPUBKEY:
		DEBUG_INFO("Dispatch get public key\n");
		ipmsg_proc_get_public_key(sock, msg);
		break;
	case IPMSG_ANSPUBKEY:
		DEBUG_INFO("Dispatch ans public key\n");
		ipmsg_proc_anspubkey(sock, msg);
		break;
	}
}

void ipmsg_send_br_entry()
{
	char packet[MAX_UDPBUF];
	size_t packet_len;
	packet_no_t packet_no;
	AddrInfo *info;
	int info_len, i;

	build_packet(packet, IPMSG_BR_ENTRY | IPMSG_ABSENCEOPT,
		     config_get_nickname(), config_get_groupname(), &packet_len,
		     &packet_no);
	info = get_sys_addr_info(&info_len);
	if (info) {
		for (i = 0; i < info_len; i++) {
			struct sockaddr_in sockaddr;
			if (info[i].br_addr == 0 && info_len > 1)
				continue;
			setup_sockaddr(&sockaddr, info[i].br_addr,
				       config_get_port());
			SENDTO(udp_sock, packet, packet_len, 0,
			       (const struct sockaddr *)&sockaddr,
			       sizeof(sockaddr));
		}
	}
	FREE_WITH_CHECK(info);

}

void ipmsg_send_br_exit()
{
	char packet[MAX_UDPBUF];
	size_t packet_len;
	packet_no_t packet_no;
	AddrInfo *info;
	int info_len, i;

	build_packet(packet, IPMSG_BR_EXIT, NULL, NULL, 
		&packet_len, &packet_no);
	info = get_sys_addr_info(&info_len);
	if (info) {
		for (i = 0; i < info_len; i++) {
			struct sockaddr_in sockaddr;
			if (info[i].br_addr == 0 && info_len > 1)
				continue;
			setup_sockaddr(&sockaddr, info[i].br_addr,
				       config_get_port());
			SENDTO(udp_sock, packet, packet_len, 0,
			       (const struct sockaddr *)&sockaddr,
			       sizeof(sockaddr));
		}
	}
	FREE_WITH_CHECK(info);
}

void ipmsg_send_ansentry(ulong toAddr)
{
	char packet[MAX_UDPBUF];
	size_t packet_len;
	packet_no_t packet_no;
	struct sockaddr_in sockaddr;

	build_packet(packet, IPMSG_ANSENTRY | IPMSG_ABSENCEOPT,
		     config_get_nickname(), config_get_groupname(), &packet_len,
		     &packet_no);
	setup_sockaddr(&sockaddr, toAddr, config_get_port());
	SENDTO(udp_sock, packet, packet_len, 0,
	       (const struct sockaddr *)&sockaddr, sizeof(sockaddr));
}

void ipmsg_send_ansrecvmsg(ulong toAddr, packet_no_t packet_no)
{
	char packet[MAX_UDPBUF];
	size_t packet_len;
	packet_no_t pkt_no;
	struct sockaddr_in sockaddr;
	char buf[MAX_NAMEBUF];

	sprintf(buf, "%ld", packet_no);
	build_packet(packet, IPMSG_RECVMSG, buf, NULL, &packet_len, &pkt_no);
	setup_sockaddr(&sockaddr, toAddr, config_get_port());
	SENDTO(udp_sock, packet, packet_len, 0,
	       (const struct sockaddr *)&sockaddr, sizeof(sockaddr));
}

int ipmsg_send_packet(ulong toAddr, const char *packet, size_t packet_len)
{
	struct sockaddr_in sockaddr;

	setup_sockaddr(&sockaddr, toAddr, config_get_port());
	return SENDTO(udp_sock, packet, packet_len, 0,
		      (const struct sockaddr *)&sockaddr, sizeof(sockaddr));
}

int
ipmsg_send_message(ulong toAddr, command_no_t flags, const char *message,
		   const char *attach)
{
	char packet[MAX_UDPBUF];
	size_t packet_len;
	packet_no_t packet_no;
	struct sockaddr_in sockaddr;

	build_packet(packet, flags, message, attach, &packet_len, &packet_no);
	setup_sockaddr(&sockaddr, toAddr, config_get_port());
	return SENDTO(udp_sock, packet, packet_len, 0,
		      (const struct sockaddr *)&sockaddr, sizeof(sockaddr));
}
