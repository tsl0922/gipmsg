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

#ifndef __GIPMSG_SENDFILE_H__
#define __GIPMSG_SENDFILE_H__

bool setup_file_info(FileInfo * info, const char *path);
void add_file(FileInfo * info);
bool del_file(const char *path);
void send_file_info(ulong toAddr, GSList * files);
GSList *parse_file_info(char *attach, packet_no_t packet_no);
void tcp_request_entry(SOCKET client_sock, ulong ipaddr);
void recv_file_entry(FileInfo * info, const char *path, SendDlg *dlg);

#endif /*__GIPMSG_SENDFILE_H__*/
