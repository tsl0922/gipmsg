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

#include <common.h>

static packet_no_t packet_no = 0;

bool
parse_message(ulong fromAddr, Message * msg, const char *msg_buf,
	      size_t buf_len)
{
	char *tok = NULL;
	char *ptr = NULL;
	command_no_t command_no = 0;
	char *tmp_str;

	if (!msg || !msg_buf || buf_len < 0)
		return false;

	GET_CLOCK_COUNT(&msg->tv);
	msg->fromAddr = fromAddr;

	/* version */
	tok = seprate_token(msg_buf, ':', &ptr);
	msg->version = strdup(tok);
	if (!ptr)
		return false;

	/* packet number */
	tok = seprate_token(msg_buf, ':', &ptr);
	msg->packetNo = strtoll(tok, (char **)NULL, 10);
	if (!ptr)
		return false;

	/* sender name */
	tok = seprate_token(msg_buf, ':', &ptr);
	msg->userName = strdup(tok);
	if (!ptr)
		return false;

	/* host name */
	tok = seprate_token(msg_buf, ':', &ptr);
	msg->hostName = strdup(tok);
	if (!ptr)
		return false;

	/* command */
	tok = seprate_token(msg_buf, ':', &ptr);
	command_no = strtol(tok, (char **)NULL, 10);
	msg->commandNo = ipmsg_flags_get_command(command_no);
	msg->commandOpts = ipmsg_flags_get_opt(command_no);
	if (!ptr)
		return false;

	/* message */
	tok = seprate_token(msg_buf, IPMSG_MSG_EXT_DELIM, &ptr);
	msg->message = strdup(tok);

	/* extra data */
	if (ptr && (ptr - msg_buf < buf_len)) {
		if (*ptr == ':')
			++ptr;
		msg->attach = strdup(ptr);
	}

	return true;
}

/* 
 * ipmsg packet format:
 *
 *	Version(1):PacketNo:SenderName:SenderHost:CommandNo:AdditionalSection
 *
 */

void
build_packet(char *packet_p, const command_no_t command, const char *message,
	     const char *attach, size_t * len_p, packet_no_t * packet_no_p)
{
	packet_no_t this_packet_no;
	char *sender_name;
	char *sender_host;
	char *common;
	size_t len;

	if (!packet_p || !len_p || !packet_no_p)
		return;

	common = (char *)malloc(MAX_UDPBUF);

	/* generate packet no */
	++packet_no;
	this_packet_no = time(NULL) % 1000 + packet_no;

	/* sendername: current username */
	sender_name = get_sys_user_name();

	/* senderhost: hostname */
	sender_host = get_sys_host_name();

	len = 0;

	len += snprintf(common, MAX_UDPBUF, IPMSG_COMMON_MSG_FMT,
			this_packet_no, sender_name, sender_host, command);

	if (message) {
		len += (strlen(message) + 1);
	}
	if (attach) {
		len += (strlen(attach) + 1);
	}

	memset(packet_p, 0, len);
	*len_p = snprintf(packet_p, len, IPMSG_ALLMSG_PACKET_FMT,
			  common,
			  (message != NULL) ? message : "",
			  IPMSG_MSG_EXT_DELIM, (attach != NULL) ? attach : "");

	*len_p = len;
	*packet_no_p = this_packet_no;

	FREE_WITH_CHECK(sender_name);
	FREE_WITH_CHECK(sender_host);
	FREE_WITH_CHECK(common);
}

void free_message_data(Message * msg)
{
	if (msg != NULL) {
		FREE_WITH_CHECK(msg->version);
		FREE_WITH_CHECK(msg->userName);
		FREE_WITH_CHECK(msg->hostName);
		FREE_WITH_CHECK(msg->message);
		FREE_WITH_CHECK(msg->attach);
	}
}

Message *dup_message(Message * src)
{
	Message *new_msg = (Message *) malloc(sizeof(Message));
	memset(new_msg, 0, sizeof(Message));
	memcpy(new_msg, src, sizeof(Message));
	STRDUP_WITH_CHECK(new_msg->version, src->version);
	STRDUP_WITH_CHECK(new_msg->userName, src->userName);
	STRDUP_WITH_CHECK(new_msg->hostName, src->hostName);
	STRDUP_WITH_CHECK(new_msg->message, src->message);
	STRDUP_WITH_CHECK(new_msg->attach, src->attach);

	return new_msg;
}

char *msg_get_version(const Message * msg)
{
	if (!msg)
		return IPMSG_UNKNOWN_NAME;
	return (msg->version != NULL
		&& strlen(msg->version)) ? msg->version : IPMSG_UNKNOWN_NAME;
}

char *msg_get_userName(const Message * msg)
{
	if (!msg)
		return IPMSG_UNKNOWN_NAME;
	return (msg->userName != NULL
		&& strlen(msg->userName)) ? msg->userName : IPMSG_UNKNOWN_NAME;
}

char *msg_get_hostName(const Message * msg)
{
	if (!msg)
		return IPMSG_UNKNOWN_NAME;
	return (msg->hostName != NULL
		&& strlen(msg->hostName)) ? msg->hostName : IPMSG_UNKNOWN_NAME;
}

char *msg_get_groupName(const Message * msg)
{
	if (!msg)
		return IPMSG_UNKNOWN_NAME;
	return (msg->attach != NULL
		&& strlen(msg->attach)) ? msg->attach : IPMSG_UNKNOWN_NAME;
}

char *msg_get_nickName(const Message * msg)
{
	if (!msg)
		return IPMSG_UNKNOWN_NAME;
	return (msg->message != NULL
		&& strlen(msg->message)) ? msg->message : msg_get_userName(msg);
}

char *msg_get_encode(const Message * msg)
{
	if (msg->commandOpts & IPMSG_UTF8OPT) {
		return "utf-8";
	}
	return IPMSG_UNKNOWN_NAME;
}
