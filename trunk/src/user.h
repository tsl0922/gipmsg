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