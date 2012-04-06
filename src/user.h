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
#ifndef __GIPMSG_USER_H__
#define __GIPMSG_USER_H__

User *create_user(Message *msg);
User *copy_user(User *src_user);
void free_user(User *user);
void clear_user_list();
void print_user_list();
bool add_user(Message *msg);
User *find_user(ulong ipaddr);
bool del_user(Message *msg);
bool update_user(Message *msg);
bool send_msg_to_user(User *user, const char *msg);

#endif