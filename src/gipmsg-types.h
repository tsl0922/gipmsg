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

#ifndef __GIPMSG_TYPES_H__
#define __GIPMSG_TYPES_H__

typedef struct _MainWindow MainWindow;
typedef struct _SendDlg SendDlg;

struct _MainWindow {
	GList *sendDlgList;
	SendDlg *dlg;
	User *user;

	GtkWidget *window;
	GtkWidget *eventbox;
	GtkStatusIcon *trayIcon;
	GtkWidget *table;

	GtkWidget *info_label;
	GtkWidget *user_tree_scroll;
	GtkWidget *user_tree;
};

typedef enum {
	ST_GETCRYPT = 0,
	ST_MAKECRYPTMSG,
	ST_MAKEMSG,
	ST_SENDMSG,
	ST_DONE
} SendStatus;

typedef struct {
	User *user;
	SendStatus status;
	command_no_t command;
	char *msg;
	char packet[MAX_UDPBUF];
	size_t packet_len;
	packet_no_t packet_no;
	int retryCount;
} SendEntry;

struct _SendDlg {
	User *user;/* current user (must own a copy of user data from user list) */

	GList *send_list;		/* SendEntry list */
	GList *file_list;		/* shared file list */
	GStaticMutex mutex;
	gint timerId;
	Message *msg;			/* the last recieved msg(must own a copy of recieved messge data) */
	gchar *text;
	gchar *fontName;

	GtkWidget *rvbox;
	GtkWidget *info_area_box;
	GtkWidget *recv_file_box;
	GtkWidget *recv_action_vbox;
	GtkWidget *recv_action_hbox;
	gboolean is_show_recv_file;
	
	GtkWidget *dialog;
	GtkWidget *head_icon;	/* head icon */
	GtkWidget *photo_image;	/* photo image */
	GtkWidget *state_label;	/* user state */
	GtkWidget *name_label;	/* nick name */
	GtkWidget *sign_label;	/* personal sign */
	GtkWidget *info_label;	/* user info */
	GtkWidget *recv_text;	/* message area */
	GtkWidget *send_text;	/* input area */
	GtkTextMark *mark;
	GtkTextBuffer *send_buffer;
	GtkTextBuffer *recv_buffer;
	GtkTextIter send_iter;
	GtkTextIter recv_iter;

	GtkWidget *recv_file_tree;	/* file to recieve */
	GtkWidget *recv_progress_bar;
	ProgressInfo recv_progress;
	GtkWidget *local_file_tree;
	GtkWidget *send_file_tree;	/* file to send */
	GtkWidget *send_progress_bar;
	ProgressInfo send_progress;
	gboolean is_show_recv_progress;
	gboolean is_show_send_progress;

	GtkWidget *emotionDlg;
};

typedef struct {
	MainWindow *main_win;
	User **users;
	size_t len;
} DialogArgs;

#endif
