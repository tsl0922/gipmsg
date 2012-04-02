#include <common.h>

static packet_no_t packet_no = 0;

bool
parse_message(ulong fromAddr, Message *msg, const char *msg_buf, size_t buf_len)
{
	bool bRetVal = true;
	char *buffer;
	char *sp = NULL;
	char *ep = NULL;
	int remain;
	packet_no_t packet_no;
	command_no_t command_no;
	char *tmp_str;

	if(!msg || !msg_buf || buf_len < 0)
		return false;
	
	memset(msg, 0, sizeof(Message));
	buffer = malloc(buf_len);
	remain = buf_len;

	GET_CLOCK_COUNT(&msg->tv);
	msg->fromAddr = fromAddr;

	memcpy(buffer, msg_buf, buf_len);
	sp = buffer;

	/* version */
	ep = memchr(sp, ':', remain);
	if(!ep) {
		bRetVal = false;
		goto error_out;
	}
	*ep = '\0';
	remain = buf_len - (ep - buffer);
	if(remain <= 0) {
		bRetVal = false;
		goto error_out;
	}
	++ep;
	msg->version = strdup(sp);
	sp = ep;

	/* packet number */
	ep = memchr(sp, ':', remain);
	if(!ep) {
		bRetVal = false;
		goto error_out;
	}
	*ep = '\0';
	remain = buf_len - (ep - buffer);
	if(remain <= 0) {
		bRetVal = false;
		goto error_out;
	}
	++ep;
	packet_no = strtoll(sp, (char **)NULL, 10);
	msg->packetNo= packet_no;
	sp = ep;

	/* sender name */
	ep = memchr(sp, ':', remain);
	if(!ep) {
		bRetVal = false;
		goto error_out;
	}
	*ep = '\0';
	remain = buf_len - (ep - buffer);
	if(remain <= 0) {
		bRetVal = false;
		goto error_out;
	}
	++ep;
	msg->userName = strdup(sp);
	sp = ep;

	/* host name */
	ep = memchr(sp, ':', remain);
	if(!ep) {
		bRetVal = false;
		goto error_out;
	}
	*ep = '\0';
	remain = buf_len - (ep - buffer);
	if(remain <= 0) {
		bRetVal = false;
		goto error_out;
	}
	++ep;
	msg->hostName = strdup(sp);
	sp = ep;

	/* command */
	ep = memchr(sp, ':', remain);
	if(!ep) {
		bRetVal = false;
		goto error_out;
	}
	*ep = '\0';
	++ep;
	command_no = strtol(sp, (char **)NULL, 10);
	msg->commandNo = ipmsg_flags_get_command(command_no);
	msg->commandOpts = ipmsg_flags_get_opt(command_no);
	sp = ep;

	/* message */
	ep = memchr(sp, IPMSG_MSG_EXT_DELIM, remain);
	if(!ep) {
		bRetVal = false;
		goto error_out;
	}
	remain = buf_len - (sp - buffer);
	msg->message = strdup(sp);
	if(!msg->message)
		goto error_out;

	/* extra data */
	if(ep - buffer < buf_len) {
		++ep;
		sp = ep;
		msg->attach = strdup(sp);
	}

error_out:
	free(buffer);
	return bRetVal;
}

void
build_packet(char *packet_p, const command_no_t command, const char *message, 
	const char *attach, size_t *len_p, packet_no_t *packet_no_p)
{
	packet_no_t this_packet_no;
	char *sender_name;
	char *sender_host;
	char *common;
	size_t len;

	if(!packet_p || !len_p || !packet_no_p)
		return;

	common = g_malloc(MAX_UDPBUF);

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

	if(message) {
		len += (strlen(message) + 1);
	}
	if(attach) {
		len += (strlen(attach) + 1);
	}

	memset(packet_p, 0, len);
	*len_p = snprintf(packet_p, len, IPMSG_ALLMSG_PACKET_FMT, 
		common, 
		(message != NULL)?message:"",
		IPMSG_MSG_EXT_DELIM,
		(attach != NULL)?attach:"");
	
	*len_p = len;
	*packet_no_p = this_packet_no;

	FREE_WITH_CHECK(sender_name);
	FREE_WITH_CHECK(sender_host);
	FREE_WITH_CHECK(common);
}

void 
free_message_data(Message *msg)
{
	if(msg != NULL) {
		FREE_WITH_CHECK(msg->version);
		FREE_WITH_CHECK(msg->userName);
		FREE_WITH_CHECK(msg->hostName);
		FREE_WITH_CHECK(msg->message);
		FREE_WITH_CHECK(msg->attach);
	}
}

char *
msg_get_version(const Message *msg)
{
	if(!msg)
		return IPMSG_UNKNOWN_NAME;
	return (msg->version != NULL && strlen(msg->version))?msg->version:IPMSG_UNKNOWN_NAME;
}

char *
msg_get_userName(const Message *msg)
{
	if(!msg)
		return IPMSG_UNKNOWN_NAME;
	return (msg->userName != NULL && strlen(msg->userName))?msg->userName:IPMSG_UNKNOWN_NAME;
}

char *
msg_get_hostName(const Message *msg)
{
	if(!msg)
		return IPMSG_UNKNOWN_NAME;
	return (msg->hostName != NULL && strlen(msg->hostName))?msg->hostName:IPMSG_UNKNOWN_NAME;
}

char *
msg_get_groupName(const Message *msg)
{
	if(!msg)
		return IPMSG_UNKNOWN_NAME;
	return (msg->attach != NULL && strlen(msg->attach))?msg->attach:IPMSG_UNKNOWN_NAME;
}

char *
msg_get_nickName(const Message *msg)
{
	if(!msg)
		return IPMSG_UNKNOWN_NAME;
	return (msg->message != NULL && strlen(msg->message))?msg->message:msg_get_userName(msg);
}

char *
msg_get_encode(const Message *msg)
{
	if(msg->commandOpts & IPMSG_UTF8OPT) {
		return "utf-8";
	}
	return IPMSG_UNKNOWN_NAME;
}

