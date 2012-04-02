#ifndef __GIPMSG_TYPES_H__
#define __GIPMSG_TYPES_H__


enum {
	EXPANDER_CLOSED,
	EXPANDER_OPEN,
	INFO_COLUMN,
	DATA_COLUMN,
	TYPE_COLUMN,
	N_COLUMNS
};

enum {
	COL_TYPE_GROUP,
	COL_TYPE_USER
};

typedef struct _MainWindow MainWindow;
typedef struct _SendDlg SendDlg;

struct _MainWindow{
	GList *sendDlgList;
	SendDlg *dlg;
	User *user;
	
	GtkWidget *window;
	GtkWidget *eventbox;
	GtkStatusIcon *trayIcon;
	GtkWidget *table;

	GtkWidget *info_label;
	GtkWidget *notebook;
	GtkWidget *users_scrolledwindow;
	GtkWidget *users_treeview;
	GtkWidget *groups_scrolledwindow;
	GtkWidget *groups_treeview;
};

typedef enum {
	ST_GETCRYPT=0,
	ST_MAKECRYPTMSG,
	ST_MAKEMSG,
	ST_SENDMSG,
	ST_DONE 
} SendStatus;

typedef struct {
	User *user;
	SendStatus status;
	command_no_t command;
	char packet[MAX_UDPBUF];
	size_t packet_len;
	packet_no_t packet_no;
} SendEntry;

struct _SendDlg {
	MainWindow *main_win;
	User **users;
	size_t len;

	SendEntry *sendEntry;
	gint sendEntryNum;
	gchar *msgBuf;
	gint timerId;
	gint retryCount;
	gchar *fontName;
	GtkWidget *emotionDlg;
	
	/* main widget begin */
	GtkWidget *dialog;
	GtkWidget *headbox;
	GdkPixbuf *headpix;
	GtkWidget *headimage;
	GtkWidget *name_box;
	GtkWidget *input_label;
	GtkWidget *name_label;
	GtkWidget *info_label; 
	GtkWidget *recv_text;
	GtkWidget *send_text;
	GtkWidget *recv_scroll;
	GtkWidget *send_scroll;
	GtkTextMark *mark;
	GtkTextBuffer *send_buffer;
	GtkTextBuffer *recv_buffer;
	GtkTextIter send_iter;
	GtkTextIter recv_iter;
	GtkTextChildAnchor *recv_anchor;

	GtkWidget *user_tree;
	GtkWidget *file_tree;
	GtkWidget *user_tree_scroll;
	GtkWidget *file_tree_scroll;

	/* toolbar begin */
	GtkWidget *toolbar;
	GtkWidget *change_font;
	GtkWidget *send_emotion;
	GtkWidget *send_image;
	GtkWidget *send_file;
	GtkWidget *send_folder;
};

typedef struct {
	MainWindow *main_win;
	User **users;
	size_t len;
} DialogArgs;

#endif

