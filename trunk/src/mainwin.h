#ifndef __GIPMSG_MAINWIN_H__
#define __GIPMSG_MAINWIN_H__

void ipmsg_ui_init();
void users_tree_create_default_group();
void users_tree_add_user(User *user);
void users_tree_del_user(ulong ipaddr);
void users_tree_update_user(User *user);
void clear_users_tree();
void notify_recvmsg(ulong ipaddr, packet_no_t packet_no);
void notify_sendmsg(ulong ipaddr, char *msg);

#endif
