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

#ifndef __CORE_H__
#define __CORE_H__
void ipmsg_core_init();
bool socket_init(const char *pAddr, int nPort);
void ipmsg_send_br_entry();
void ipmsg_send_ansentry(ulong toAddr);
void ipmsg_send_ansrecvmsg(ulong toAddr, packet_no_t packet_no);
int ipmsg_send_packet(ulong toAddr, const char *packet, size_t packet_len);
int ipmsg_send_message(ulong toAddr, command_no_t flags, const char *message, const char *attach);


extern SOCKET udp_sock;
extern SOCKET tcp_sock;

#endif
