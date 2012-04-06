/* Gipmsg - Local networt message communication software
 *  
 * Copyright (C) 2012 tsl0922<tsl0922@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __MESSAGE_H__
#define __MESSAGE_H__

bool parse_message(ulong fromAddr, Message * msg, const char *msg_buf,
		   size_t buf_len);

void build_packet(char *packet_p, const command_no_t command,
		  const char *message, const char *attach, size_t * len_p,
		  packet_no_t * packet_no_p);
Message *dup_message(Message * msg);
char *msg_get_version(const Message * msg);
char *msg_get_userName(const Message * msg);
char *msg_get_hostName(const Message * msg);
char *msg_get_groupName(const Message * msg);
char *msg_get_nickName(const Message * msg);
char *msg_get_encode(const Message * msg);

#endif
