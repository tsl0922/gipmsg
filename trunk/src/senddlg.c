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

#include "common.h"

#define MSG_SEND_MAX_RETRY		3
#define MSG_RETRY_INTERVAL		1500

/* send file tree */
enum {
	SF_COL_ICON,		/* file icon */
	SF_COL_NAME,		/* file name */
	SF_COL_TYPE,		/* file type(regular file, dir file) */
	SF_COL_SIZE,		/* file size */
	SF_COL_PATH,		/* save path */
	SF_COL_INFO,		/* FileInfo pointer(not displayed) */
	SF_COL_NUM
};

/* recv file tree */
enum {
	RF_COL_ICON,		/* file icon */
	RF_COL_NAME,		/* file name */
	RF_COL_TYPE,		/* file type(regular file, dir file) */
	RF_COL_SIZE,		/* file size */
	RF_COL_PATH,		/* save path */
	RF_COL_TIME,		/* recv path */
	RF_COL_INFO,		/* FileInfo pointer(not displayed) */
	RF_COL_NUM
};

extern MainWindow main_win;

typedef struct {
	SendDlg *dlg;
	void *data;
} CallbackArgs;

typedef void *(*ButtonFunc) (GtkButton * button, gpointer data);

typedef void *(*LinkButtonFunc) (GtkLinkButton * button, const gchar * link_,
				 gpointer data);

static void senddlg_ui_init(SendDlg * dlg);
static void on_send_file_clicked(GtkWidget * widget, SendDlg *dlg);
static void on_send_folder_clicked(GtkWidget * widget, SendDlg *dlg);
static void update_dlg_info(SendDlg * dlg);
static void send_msg(SendEntry * entry);
static void recv_text_add_button(SendDlg * dlg, const char *label,
				 const char *tip_text, LinkButtonFunc func,
				 gpointer data);
static gboolean retry_message_handler(gpointer data);
static void show_recv_file_box(SendDlg *dlg , gboolean show);
static gboolean update_progress_bar(SendDlg *dlg);

SendDlg *senddlg_new(User * user)
{
	SendDlg *dlg = (SendDlg *) malloc(sizeof(SendDlg));

	memset(dlg, 0, sizeof(SendDlg));

	if (!user) {
		free(dlg);
		return NULL;
	}
	dlg->timerId = 0;

	dlg->user = dup_user(user);
	dlg->fontName = strdup("Arial 10");
	dlg->emotionDlg = NULL;
	dlg->send_list = NULL;
	g_static_mutex_init(&dlg->mutex);
	dlg->msg = NULL;
	dlg->is_show_recv_file = FALSE;
	dlg->is_show_recv_progress = FALSE;
	dlg->is_show_send_progress = FALSE;
	senddlg_ui_init(dlg);

	g_timeout_add(300, (GSourceFunc)update_progress_bar, dlg);
	
	return dlg;
}

static gboolean on_close_clicked(GtkWidget * widget, SendDlg * dlg)
{
	main_win.sendDlgList = g_list_remove(main_win.sendDlgList, dlg);
	free_user_data(dlg->user);
	if(dlg->msg) free_message_data(dlg->msg);
	gtk_widget_destroy(dlg->dialog);
	FREE_WITH_CHECK(dlg);

	return TRUE;
}

static gboolean on_detete(GtkWidget * widget, GdkEvent * event, SendDlg * dlg)
{
	return on_close_clicked(NULL, dlg);
}

static void
on_retry_link_clicked(GtkLinkButton * button, const gchar * link_,
		      gpointer data)
{
	CallbackArgs *args = (CallbackArgs *)data;
	send_msg((SendEntry *) (args->data));
	args->dlg->timerId =
	    g_timeout_add(MSG_RETRY_INTERVAL, retry_message_handler, args->dlg);
	gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
	FREE_WITH_CHECK(args);
}

static gboolean retry_message_handler(gpointer data)
{
	SendDlg *dlg = (SendDlg *) data;
	GList *list = g_list_first(dlg->send_list);
	SendEntry *entry;
	bool finish = true;

	g_source_remove(dlg->timerId);
	while (list) {
		entry = (SendEntry *) list->data;
		if (entry->status != ST_DONE) {
			if (entry->retryCount++ < MSG_SEND_MAX_RETRY) {
				DEBUG_INFO("retry: %d", entry->retryCount);
				send_msg(entry);
				finish = false;
			} else {
				char buf[MAX_BUF];
				DEBUG_ERROR("Send fail!");
				snprintf(buf, MAX_BUF,
					 "Failed to send the following message to %s:\n%s",
					 entry->user->nickName, entry->msg);
				senddlg_add_info(dlg, buf);
				//create retry button
				entry->retryCount = 0;	//reset retryCount
				CallbackArgs *args =
				    (CallbackArgs *)malloc(sizeof(CallbackArgs));
				args->dlg = dlg;
				args->data = entry;
				recv_text_add_button(dlg, "Retry",
						     "Resend message.",
						     (LinkButtonFunc)
						     on_retry_link_clicked,
						     args);
			}
		} else {
			g_static_mutex_lock(&dlg->mutex);
			FREE_WITH_CHECK(entry->msg);
			FREE_WITH_CHECK(entry);
			dlg->send_list =
			    g_list_delete_link(dlg->send_list, list);
			g_static_mutex_unlock(&dlg->mutex);
			list = g_list_first(dlg->send_list);
			continue;
		}
		list = list->next;
	}
	
	if (!finish)
		dlg->timerId =
		    g_timeout_add(MSG_RETRY_INTERVAL, retry_message_handler,
				  dlg);
	return TRUE;
}

bool is_same_packet(SendDlg * dlg, Message * msg)
{
	if (dlg->msg == NULL)
		return false;

	return (dlg->msg->fromAddr == msg->fromAddr
		&& dlg->msg->packetNo == msg->packetNo);
}

static void make_packet(SendEntry * entry)
{
	packet_no_t packet_no;
	char *tmsg;

	tmsg = convert_encode(entry->user->encode, "utf-8", 
			entry->msg, strlen(entry->msg));
	if(tmsg) {
		FREE_WITH_CHECK(entry->msg);
		entry->msg = tmsg;
	}
	build_packet(entry->packet, entry->command, entry->msg,
		     NULL, &entry->packet_len, &entry->packet_no);
	entry->status = ST_SENDMSG;
}

static void send_msg(SendEntry * entry)
{
	if (entry->status == ST_MAKEMSG) {
		make_packet(entry);
	}
	if (entry->status == ST_SENDMSG) {
		ipmsg_send_packet(entry->user->ipaddr, entry->packet,
				  entry->packet_len);
	}
}

static void send_file_attach_info(SendDlg * dlg)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GSList *files = NULL;
	FileInfo *info;
	char buf[MAX_BUF];
	int file_num = 0;
	int folder_num = 0;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dlg->send_file_tree));
	if (!gtk_tree_model_get_iter_first(model, &iter))
		return;
	do {
		gtk_tree_model_get(model, &iter, SF_COL_INFO, &info, -1);
		if (info->attr & IPMSG_FILE_DIR)
			folder_num++;
		else
			file_num++;
		files = g_slist_append(files, info);
	} while (gtk_tree_model_iter_next(model, &iter));

	if (files)
		send_file_info(dlg->user, files);
	gtk_list_store_clear(GTK_LIST_STORE(model));
	snprintf(buf, MAX_BUF,
		 "You request send %d files, %d folders(total: %d) to %s.",
		 file_num, folder_num, file_num + folder_num,
		 dlg->user->nickName);
	senddlg_add_info(dlg, buf);
}

static void on_send_clicked(GtkWidget * widget, SendDlg * dlg)
{
	GtkTextIter begin, end;
	gint count, i;
	SendEntry *sendEntry;
	command_no_t command = IPMSG_SENDMSG | IPMSG_SENDCHECKOPT;

	send_file_attach_info(dlg);
	gtk_text_buffer_get_start_iter(dlg->send_buffer, &begin);
	gtk_text_buffer_get_end_iter(dlg->send_buffer, &end);
	dlg->text = gtk_text_buffer_get_text(dlg->send_buffer,
					     &begin, &end, TRUE);
	if (*dlg->text == '\0')
		return;

	senddlg_add_message(dlg, dlg->text, true);

	sendEntry = (SendEntry *) malloc(sizeof(SendEntry));
	sendEntry->command = command;
	sendEntry->user = dlg->user;
	sendEntry->status = ST_MAKEMSG;
	sendEntry->retryCount = 0;
	sendEntry->msg = strdup(dlg->text);

	send_msg(sendEntry);
	dlg->timerId =
	    g_timeout_add(MSG_RETRY_INTERVAL, retry_message_handler, dlg);
	g_static_mutex_lock(&dlg->mutex);
	dlg->send_list = g_list_append(dlg->send_list, sendEntry);
	g_static_mutex_unlock(&dlg->mutex);

	gtk_text_buffer_delete(dlg->send_buffer, &begin, &end);
}

static void on_change_font_clicked(GtkWidget * widget, SendDlg * dlg)
{
	GtkWidget *dialog;
	PangoFontDescription *fontDesc;

	dialog = gtk_font_selection_dialog_new("Change Font");
	gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG
						(dialog), dlg->fontName);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		dlg->fontName =
		    gtk_font_selection_dialog_get_font_name
		    (GTK_FONT_SELECTION_DIALOG(dialog));
		fontDesc = pango_font_description_from_string(dlg->fontName);
		gtk_widget_modify_font(dlg->send_text, fontDesc);
		gtk_widget_modify_font(dlg->recv_text, fontDesc);
	}
	gtk_widget_destroy(dialog);
}

static void on_send_emotion_clicked(GtkWidget * widget, SendDlg * dlg)
{

	int x, y, ex, ey, root_x, root_y;

	gtk_widget_translate_coordinates(widget, dlg->dialog, 0, 0, &ex, &ey);
	gtk_window_get_position(GTK_WINDOW(dlg->dialog), &root_x, &root_y);
	x = root_x + ex + 3;
	y = root_y + ey - 165;

	show_emotion_dialog(dlg, x, y);
}

static void on_send_image_clicked(GtkWidget * widget, SendDlg * dlg)
{
	GtkWidget *dialog;
	gchar *filename = NULL;
	GtkFileFilter *filter;

	dialog = gtk_file_chooser_dialog_new("Select file",
					     GTK_WINDOW(dlg->dialog),
					     GTK_FILE_CHOOSER_ACTION_OPEN,
					     GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OPEN,
					     GTK_RESPONSE_ACCEPT, NULL);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter,
				 "Image Files(*.jpg;*.jpeg;*.png;*.gif)");
	gtk_file_filter_add_pattern(filter, "*.jpg");
	gtk_file_filter_add_pattern(filter, "*.jpeg");
	gtk_file_filter_add_pattern(filter, "*.png");
	gtk_file_filter_add_pattern(filter, "*.gif");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		filename =
		    gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if (filename)
			g_print("%s\n", filename);
		//FIXME: send image
		if (filename)
			g_free(filename);
	}
	gtk_widget_destroy(dialog);
}

static GtkWidget *create_title_area(SendDlg *dlg)
{
	GtkWidget *hbox;
	GtkWidget *vbox1;
	GtkWidget *hbox1;
	GdkPixbuf *pb;

	hbox = gtk_hbox_new(FALSE, 5);
	vbox1 = gtk_vbox_new(TRUE, 0);
	
	dlg->head_icon = gtk_image_new();
	pb = gdk_pixbuf_new_from_file_at_size(ICON_PATH "icon_linux.png", 30,
			30, NULL);
	gtk_image_set_from_pixbuf(GTK_IMAGE(dlg->head_icon), pb);
	g_object_unref(pb);
	gtk_box_pack_start(GTK_BOX(hbox), dlg->head_icon, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox1, FALSE, FALSE, 0);
	
	hbox1 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0);
	dlg->name_label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox1), dlg->name_label, FALSE, FALSE, 0);
	dlg->state_label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox1), dlg->state_label, FALSE, FALSE, 10);
	
	dlg->sign_label = gtk_label_new(NULL);
	gtk_widget_set_size_request(dlg->sign_label, 450, 20);
	gtk_misc_set_alignment(GTK_MISC(dlg->sign_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), dlg->sign_label, FALSE, FALSE, 0);

	return hbox;
}

static GtkWidget *create_recv_text_view(SendDlg *dlg)
{
	GtkWidget *scroll;

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
				       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll),
					    GTK_SHADOW_ETCHED_IN);
	dlg->recv_text = gtk_text_view_new();
	gtk_container_add(GTK_CONTAINER(scroll), dlg->recv_text);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(dlg->recv_text), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(dlg->recv_text),
				    GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(dlg->recv_text), FALSE);

	dlg->recv_buffer =
	    gtk_text_view_get_buffer(GTK_TEXT_VIEW(dlg->recv_text));
	gtk_text_buffer_create_tag(dlg->recv_buffer, "blue", "foreground",
				   "#639900", NULL);
	gtk_text_buffer_create_tag(dlg->recv_buffer, "grey", "foreground",
				   "#808080", NULL);
	gtk_text_buffer_create_tag(dlg->recv_buffer, "green", "foreground",
				   "green", NULL);
	gtk_text_buffer_create_tag(dlg->recv_buffer, "red", "foreground",
				   "#0088bf", NULL);
	gtk_text_buffer_create_tag(dlg->recv_buffer, "lm10", "left_margin", 10,
				   NULL);
	gtk_text_buffer_create_tag(dlg->recv_buffer, "tm10",
				   "pixels-below-lines", 2, NULL);
	gtk_text_buffer_create_tag(dlg->recv_buffer, "small", "left_margin", 5,
				   NULL);
	gtk_text_buffer_get_end_iter(dlg->recv_buffer, &(dlg->recv_iter));
	gtk_text_buffer_create_mark(dlg->recv_buffer, "scroll",
				    &(dlg->recv_iter), FALSE);

	return scroll;
}

static GtkWidget *create_send_toolbar(SendDlg *dlg)
{
	GtkWidget *toolbar;
	GtkWidget *toolitem;
	GtkWidget *tool_icon;
	
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	tool_icon = gtk_image_new_from_stock(GTK_STOCK_SELECT_FONT,
				     GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Font",
			"change font.", NULL, tool_icon,
			G_CALLBACK(on_change_font_clicked), dlg);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	tool_icon = gtk_image_new_from_file(ICON_PATH "emotion.png");
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
			"Emotion", "send emotion.",
			NULL, tool_icon,
			G_CALLBACK(on_send_emotion_clicked), dlg);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	tool_icon = gtk_image_new_from_file(ICON_PATH "image.png");
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
			"Picture", "send picture.",
			NULL, tool_icon,
			G_CALLBACK(on_send_image_clicked), dlg);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	tool_icon = gtk_image_new_from_file(ICON_PATH "file.png");
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
			"File", "send file.",
			NULL, tool_icon,
			G_CALLBACK(on_send_file_clicked), dlg);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	tool_icon = gtk_image_new_from_file(ICON_PATH "folder.png");
	gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
			"Folder", "send folder.",
			NULL, tool_icon,
			G_CALLBACK(on_send_folder_clicked), dlg);
	gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

	return toolbar;
}

static GtkWidget *create_send_text_view(SendDlg *dlg)
{
	GtkWidget *scroll;

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
				       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll),
					    GTK_SHADOW_ETCHED_IN);
	dlg->send_text = gtk_text_view_new();
	gtk_container_add(GTK_CONTAINER(scroll), dlg->send_text);
	gtk_widget_set_size_request(dlg->send_text, 0, 100);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(dlg->send_text),
				    GTK_WRAP_WORD_CHAR);
	dlg->send_buffer =
	    gtk_text_view_get_buffer(GTK_TEXT_VIEW(dlg->send_text));
	gtk_text_buffer_get_iter_at_offset(dlg->send_buffer, &dlg->send_iter, 0);
	
	return scroll;
	
}

static GtkWidget *create_send_action_area(SendDlg *dlg)
{
	GtkWidget *halign;
	GtkWidget *hbox;
	GtkWidget *button;

	halign = gtk_alignment_new(1, 0, 0, 0);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(halign), hbox);
	
	button = gtk_button_new_with_label("Close");
	gtk_widget_set_size_request(button, 100, 30);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 2);
	g_signal_connect(button, "clicked", G_CALLBACK(on_close_clicked), dlg);

	button = gtk_button_new_with_label("Send");
	gtk_widget_set_size_request(button, 100, 30);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 2);
	g_signal_connect(button, "clicked", G_CALLBACK(on_send_clicked), dlg);

	return halign;
}

static GtkWidget *create_info_area(SendDlg * dlg)
{
	GtkWidget *vbox;
	GtkWidget *frame;
	GdkPixbuf *pb;

	vbox = gtk_vbox_new(FALSE, 0);
	dlg->info_area_box = vbox;
	frame = gtk_frame_new("User Information");
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	dlg->info_label = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(frame), dlg->info_label);
	frame = gtk_frame_new("User Photo");
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame),
				  GTK_SHADOW_ETCHED_IN);
	gtk_widget_set_size_request(frame, 180, 150);
	dlg->photo_image = gtk_image_new();
	gtk_container_add(GTK_CONTAINER(frame), dlg->photo_image);

	pb = gdk_pixbuf_new_from_file_at_size(ICON_PATH "icon.png", 140,
						     140, NULL);
	gtk_image_set_from_pixbuf(GTK_IMAGE(dlg->photo_image), pb);
	g_object_unref(pb);
	
	return vbox;
}

static GtkWidget *create_send_file_tree()
{
	GtkWidget *send_tree;
	GtkListStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	/* create model */
	store = gtk_list_store_new(SF_COL_NUM,
				   GDK_TYPE_PIXBUF, G_TYPE_STRING,
				   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				   G_TYPE_POINTER);
	/* create tree */
	send_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(send_tree), 
			GTK_TREE_VIEW_GRID_LINES_HORIZONTAL);

	/* create column */
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes("Name",
							  renderer, "pixbuf",
							  SF_COL_ICON, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "text",
					    SF_COL_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(send_tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Type",
							  renderer, "text",
							  SF_COL_TYPE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(send_tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Size",
							  renderer, "text",
							  SF_COL_SIZE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(send_tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Path",
							  renderer, "text",
							  SF_COL_PATH, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(send_tree), column);

	return send_tree;
}

static GdkPixbuf *get_file_icon(GFile * file)
{
	GFileInfo *fileinfo;
	GtkIconInfo *iconinfo;
	GdkPixbuf *pixbuf;

	fileinfo = g_file_query_info(file, "standard::*", 0, NULL, NULL);
	iconinfo = gtk_icon_theme_lookup_by_gicon(gtk_icon_theme_get_default(),
						  g_file_info_get_icon
						  (fileinfo), 24, 0);
	pixbuf = gtk_icon_info_load_icon(iconinfo, NULL);
	gtk_icon_info_free(iconinfo);

	return pixbuf;
}

static void update_send_file_tree(GFile * file, SendDlg *dlg)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkIconInfo *iconinfo;
	GdkPixbuf *pixbuf;
	FileInfo *info;
	gchar *path;
	char filesize[MAX_NAMEBUF];

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dlg->send_file_tree));
	path = g_file_get_path(file);
	DEBUG_INFO("path: %s", path);
	//check whether the file is already added
	if (gtk_tree_model_get_iter_first(model, &iter)) {
		gchar *tpath;
		do {
			gtk_tree_model_get(model, &iter, SF_COL_PATH, &tpath,
					   -1);
			if (!strcmp(tpath, path)) {
				g_free(path);
				return;
			}
		} while (gtk_tree_model_iter_next(model, &iter));
	}
	info = (FileInfo *) malloc(sizeof(FileInfo));
	if (!setup_file_info(info, path)) {
		g_free(path);
		return;
	}
	pixbuf = (GdkPixbuf *)get_file_icon(file);
	format_filesize(filesize, info->size);
	gtk_list_store_append(GTK_LIST_STORE(model), &iter);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			   SF_COL_ICON, pixbuf,
			   SF_COL_NAME, info->name,
			   SF_COL_TYPE,
			   (info->attr & IPMSG_FILE_DIR) ? "folder" : "file",
			   SF_COL_SIZE, filesize, SF_COL_PATH, info->path,
			   SF_COL_INFO, info, -1);
	dlg->file_list = g_list_append(dlg->file_list, info);
	g_object_unref(file);
	g_free(path);
}

static void on_send_file_clicked(GtkWidget * widget, SendDlg *dlg)
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Select file",
					     GTK_WINDOW(dlg->dialog),
					     GTK_FILE_CHOOSER_ACTION_OPEN,
					     GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OPEN,
					     GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *files =
		    gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dialog));
		g_slist_foreach(files, (GFunc) update_send_file_tree, dlg);
		g_slist_free(files);
	}
	gtk_widget_destroy(dialog);
}

static void on_send_folder_clicked(GtkWidget * widget, SendDlg *dlg)
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Select file",
					     GTK_WINDOW(dlg->dialog),
					     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					     GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OPEN,
					     GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *files =
		    gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dialog));
		g_slist_foreach(files, (GFunc) update_send_file_tree, dlg);
		g_slist_free(files);
	}
	gtk_widget_destroy(dialog);
}

static GtkWidget *create_button(const char *text, ButtonFunc func, gpointer data)
{
	GtkWidget *button;
	GtkWidget *label;
	PangoFontDescription *fontDesc;

	button = gtk_button_new();
	label = gtk_label_new(text);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	fontDesc = pango_font_description_from_string("Arial 8");
	gtk_widget_modify_font(label, fontDesc);
	gtk_container_add(GTK_CONTAINER(button), label);
	gtk_container_check_resize(GTK_CONTAINER(button));
	if(func != NULL) {
		g_signal_connect(button, "clicked", G_CALLBACK(func), data);
	}

	return button;
}

static void on_monitor_clicked(GtkButton *button, SendDlg *dlg)
{
	if(dlg->is_show_recv_file)
		show_recv_file_box(dlg, FALSE);
	else
		show_recv_file_box(dlg, TRUE);
}

static void on_cancel_file_clicked(GtkButton *button, SendDlg *dlg)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	GList *slist;

	slist = gtk_tree_selection_get_selected_rows(
		gtk_tree_view_get_selection(GTK_TREE_VIEW(dlg->send_file_tree)), NULL);
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dlg->send_file_tree));
	
	if(!g_list_length(slist)) {
		gtk_list_store_clear(GTK_LIST_STORE(model));
		return;
	}
	
	while (slist) {
		path = (GtkTreePath *) slist->data;
		if (gtk_tree_model_get_iter(model, &iter, path)) {
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
		}
		slist = slist->next;
	}

	g_list_foreach(slist, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(slist);
}

static GtkWidget *create_send_file_action_area(SendDlg *dlg)
{
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *label;

	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), "<span font='bold 8'>Send File</span>");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), 
		create_button("Cancel", (ButtonFunc)on_cancel_file_clicked, dlg), 
		FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), 
		create_button("File", (ButtonFunc)on_send_file_clicked, dlg), 
		FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), 
		create_button("Folder", (ButtonFunc)on_send_folder_clicked, dlg), 
		FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), 
		create_button("Monitor", (ButtonFunc)on_monitor_clicked, dlg), 
		FALSE, FALSE, 0);

	return hbox;
}

GtkWidget *create_send_file_box(SendDlg *dlg)
{
	GtkWidget *vbox;
	GtkWidget *scroll;

	vbox = gtk_vbox_new(FALSE, 0);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll),
			GTK_SHADOW_ETCHED_IN);
	gtk_widget_set_size_request(scroll, 180, 100);
	dlg->send_file_tree = create_send_file_tree();
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection
			(GTK_TREE_VIEW(dlg->send_file_tree)),
			GTK_SELECTION_MULTIPLE);
	gtk_container_add(GTK_CONTAINER(scroll), dlg->send_file_tree);
	gtk_box_pack_start(GTK_BOX(vbox), create_send_file_action_area(dlg),
			FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

	return vbox;
}

static gchar *
get_save_path(GtkWindow *parent)
{
	GtkWidget *dialog;
	gchar *save_path = NULL;
	
	dialog = gtk_file_chooser_dialog_new("Select Save Path",
					     parent,
					     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					     GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OPEN,
					     GTK_RESPONSE_ACCEPT, NULL);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
		save_path = g_file_get_path(file);
	}
	gtk_widget_destroy(dialog);
	
	return save_path;
}

static void accept_selected_items(SendDlg *dlg)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	FileInfo *info;
	gchar *save_path;
	GList *slist;

	slist = gtk_tree_selection_get_selected_rows(
		gtk_tree_view_get_selection(GTK_TREE_VIEW(dlg->recv_file_tree)), NULL);
	if(!g_list_length(slist))
		return;
	if(!(save_path = get_save_path(GTK_WINDOW(dlg->dialog))))
		return;
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dlg->recv_file_tree));
	while (slist) {
		path = (GtkTreePath *) slist->data;
		if (gtk_tree_model_get_iter(model, &iter, path)) {
			gtk_tree_model_get(model, &iter, RF_COL_INFO, &info, -1);
			recv_file_entry(info, save_path, dlg);
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
		}
		slist = slist->next;
	}

	g_list_foreach(slist, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(slist);
}

static void refuse_selected_items(SendDlg *dlg)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	FileInfo *info;
	GList *slist;

	slist = gtk_tree_selection_get_selected_rows(
		gtk_tree_view_get_selection(GTK_TREE_VIEW(dlg->recv_file_tree)), NULL);
	if(!g_list_length(slist))
		return;
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dlg->recv_file_tree));
	while (slist) {
		path = (GtkTreePath *) slist->data;
		if (gtk_tree_model_get_iter(model, &iter, path)) {
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
		}
		slist = slist->next;
	}

	g_list_foreach(slist, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(slist);
}

static void accept_all_items(SendDlg *dlg)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	FileInfo *info;
	gchar *save_path;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dlg->recv_file_tree));
	if (!gtk_tree_model_get_iter_first(model, &iter))
		return;
	if(!(save_path = get_save_path(GTK_WINDOW(dlg->dialog))))
		return;
	do {
		gtk_tree_model_get(model, &iter, RF_COL_INFO, &info, -1);
		recv_file_entry(info, save_path, dlg);
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	} while (gtk_tree_model_iter_next(model, &iter));
}

static void refuse_all_items(SendDlg *dlg)
{
	GtkTreeModel *model;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dlg->recv_file_tree));
	gtk_list_store_clear(GTK_LIST_STORE(model));
}

static void on_recv_menu_accept(GtkMenuItem * menu_item, SendDlg *dlg)
{
	accept_selected_items(dlg);
}

static void on_recv_menu_refuse(GtkMenuItem * menu_item, SendDlg *dlg)
{
	refuse_selected_items(dlg);
}


static void on_recv_menu_accept_all(GtkMenuItem * menu_item, SendDlg *dlg)
{
	accept_all_items(dlg);
}

static void on_recv_menu_refuse_all(GtkMenuItem * menu_item, SendDlg *dlg)
{
	refuse_all_items(dlg);
}

static gboolean 
on_recv_tree_menu_popup(GtkWidget * treeview, GdkEventButton * event, SendDlg *dlg)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkWidget *menu;

	if (event->button != 3
	    || !gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
					      (gint) (event->x),
					      (gint) (event->y), &path, NULL,
					      NULL, NULL))
		return FALSE;
	gtk_tree_selection_select_path(gtk_tree_view_get_selection(
		GTK_TREE_VIEW(treeview)), path);
	gtk_tree_path_free(path);

	menu = gtk_menu_new();
	create_menu_item("Accept", NULL, menu, TRUE,
			 (MenuCallBackFunc) on_recv_menu_accept, dlg);
	create_menu_item("Refuse", NULL, menu, TRUE,
			 (MenuCallBackFunc) on_recv_menu_refuse, dlg);
	create_menu_item("Accept All", NULL, menu, TRUE,
			 (MenuCallBackFunc) on_recv_menu_accept_all, dlg);
	create_menu_item("Refuse All", NULL, menu, TRUE,
			 (MenuCallBackFunc) on_recv_menu_refuse_all, dlg);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		       event->button, event->time);
	
	return TRUE;
}

static void
on_recv_tree_item_activited(GtkTreeView * treeview, GtkTreePath * path,
		       GtkTreeViewColumn * column, SendDlg *dlg)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	FileInfo *info;
	gchar *save_path;

	if(!(save_path = get_save_path(GTK_WINDOW(NULL))))
		return;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	if (!gtk_tree_model_get_iter(model, &iter, path))
		return;
	gtk_tree_model_get(model, &iter, RF_COL_INFO, &info, -1);
	recv_file_entry(info, save_path, dlg);
	gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
}

static GtkWidget *create_recv_file_tree()
{
	GtkWidget *recv_tree;
	GtkListStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	/* create model */
	store = gtk_list_store_new(RF_COL_NUM,
				   GDK_TYPE_PIXBUF, G_TYPE_STRING,
				   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				   G_TYPE_STRING, G_TYPE_POINTER);
	/* create tree */
	recv_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection
				    (GTK_TREE_VIEW(recv_tree)), GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(recv_tree), 
			GTK_TREE_VIEW_GRID_LINES_HORIZONTAL);
	/* create column */
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes("Name",
							  renderer, "pixbuf",
							  RF_COL_ICON, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "text",
					    RF_COL_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(recv_tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Type",
							  renderer, "text",
							  RF_COL_TYPE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(recv_tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Size",
							  renderer, "text",
							  RF_COL_SIZE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(recv_tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Save Path",
							  renderer, "text",
							  RF_COL_PATH, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(recv_tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Recieved Time",
							  renderer, "text",
							  RF_COL_TIME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(recv_tree), column);

	return recv_tree;
}

static void on_refuse_cilcked(GtkButton *button, SendDlg *dlg)
{
	refuse_selected_items(dlg);
}

static void on_accept_cilcked(GtkButton *button, SendDlg *dlg)
{
	accept_selected_items(dlg);
}

static void on_refuse_all_clicked(GtkButton *button, SendDlg *dlg)
{
	refuse_all_items(dlg);
}

static void on_accept_all_clicked(GtkButton *button, SendDlg *dlg)
{
	accept_all_items(dlg);
}

static GtkWidget *create_recv_action_area(SendDlg *dlg)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *label;

	vbox = gtk_vbox_new(FALSE, 0);
	dlg->recv_action_vbox = vbox;
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), "<span font='bold 8'>Recieve File</span>");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), 
		create_button("Cancel Transfer", (ButtonFunc)NULL, NULL), 
		FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new(FALSE, 0);
	dlg->recv_action_hbox = hbox;
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	
	gtk_box_pack_end(GTK_BOX(hbox), 
		create_button("Refuse", (ButtonFunc)on_refuse_cilcked, dlg), 
		FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), 
		create_button("Accept", (ButtonFunc)on_accept_cilcked, dlg), 
		FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), 
		create_button("Refuse All", (ButtonFunc)on_refuse_all_clicked, dlg), 
		FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), 
		create_button("Accept All", (ButtonFunc)on_accept_all_clicked, dlg), 
		FALSE, FALSE, 0);

	return vbox;
}

static void on_close_recv_box_clicked(GtkWidget *button, SendDlg *dlg)
{
	show_recv_file_box(dlg, FALSE);
}

static void on_local_folder_clicked(GtkWidget *button, SendDlg *dlg)
{
}

static void on_local_open_clicked(GtkWidget *button, SendDlg *dlg)
{
}

static GtkWidget *create_local_action_area(SendDlg *dlg)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *label;

	vbox = gtk_vbox_new(FALSE, 0);
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), "<span font='bold 8'>Recieved File</span>");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), 
		create_button("Close", (ButtonFunc)on_close_recv_box_clicked, dlg), 
		FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), 
		create_button("Open", (ButtonFunc)on_local_open_clicked, dlg), 
		FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), 
		create_button("Folder", (ButtonFunc)on_local_folder_clicked, dlg), 
		FALSE, FALSE, 0);

	return vbox;
}

GtkWidget *create_recv_file_box(SendDlg *dlg)
{
	GtkWidget *vbox;
	GtkWidget *scroll;
	
	vbox = gtk_vbox_new(FALSE, 0);
	dlg->recv_file_box = vbox;
	
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll),
			GTK_SHADOW_ETCHED_IN);
	gtk_widget_set_size_request(scroll, 180, 90);
	dlg->recv_file_tree = create_recv_file_tree();
	g_signal_connect(dlg->recv_file_tree, "button-press-event",
			 G_CALLBACK(on_recv_tree_menu_popup), dlg);
	g_signal_connect(dlg->recv_file_tree, "row-activated",
			 G_CALLBACK(on_recv_tree_item_activited), dlg);
	gtk_container_add(GTK_CONTAINER(scroll), dlg->recv_file_tree);
	gtk_box_pack_start(GTK_BOX(vbox), create_recv_action_area(dlg), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
	
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll),
			GTK_SHADOW_ETCHED_IN);
	gtk_widget_set_size_request(scroll, 180, 90);
	dlg->local_file_tree = create_recv_file_tree();
	gtk_container_add(GTK_CONTAINER(scroll), dlg->local_file_tree);
	gtk_box_pack_start(GTK_BOX(vbox), create_local_action_area(dlg), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

	return vbox;
}

static void senddlg_ui_init(SendDlg * dlg)
{
	GtkWidget *vbox;
	GtkWidget *lvbox;
	GtkWidget *rvbox;
	GtkWidget *hpaned;

	dlg->dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_modal(GTK_WINDOW(dlg->dialog), FALSE);
	gtk_window_set_default_size(GTK_WINDOW(dlg->dialog), 650, 450);
	gtk_container_set_border_width(GTK_CONTAINER(dlg->dialog), 10);
	g_signal_connect(dlg->dialog, "delete-event", G_CALLBACK(on_detete),
			 dlg);

	vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(dlg->dialog), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), create_title_area(dlg), FALSE, FALSE, 0);

	hpaned = gtk_hpaned_new();
	gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);
	lvbox = gtk_vbox_new(FALSE, 2);
	rvbox = gtk_vbox_new(FALSE, 2);
	dlg->rvbox = rvbox;
	gtk_paned_pack1(GTK_PANED(hpaned), lvbox, TRUE, FALSE);
	gtk_paned_pack2(GTK_PANED(hpaned), rvbox, FALSE, FALSE);
	
	gtk_box_pack_start(GTK_BOX(lvbox), 
		create_recv_text_view(dlg), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(lvbox), 
		create_send_toolbar(dlg), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(lvbox), 
		create_send_text_view(dlg), FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(lvbox), 
		create_send_action_area(dlg), FALSE, FALSE, 0);

	create_recv_file_box(dlg);
	gtk_widget_show_all(dlg->recv_file_box);
	
	gtk_box_pack_start(GTK_BOX(rvbox), 
		create_info_area(dlg), TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(rvbox),
		create_send_file_box(dlg), TRUE, TRUE, 0);

	dlg->recv_progress_bar = gtk_progress_bar_new();
	gtk_widget_show(dlg->recv_progress_bar);
	dlg->send_progress_bar = gtk_progress_bar_new();
	gtk_widget_show(dlg->send_progress_bar);
	
	gtk_window_set_position(GTK_WINDOW(dlg->dialog), GTK_WIN_POS_CENTER);
	gint x, y;
	gtk_window_get_position(GTK_WINDOW(dlg->dialog), &x, &y);
	gtk_window_move(GTK_WINDOW(dlg->dialog), x + rand() % 100,
			y + rand() % 100);
	
	update_dlg_info(dlg);
	
	gtk_widget_show_all(vbox);
}

void senddlg_add_message(SendDlg * dlg, const char *msg, bool issendmsg)
{
	GtkTextChildAnchor *anchor;
	GtkWidget *pb;
	gchar text[4096];
	gchar color[10];
	gchar time[30];

	if (issendmsg == 1)
		strcpy(color, "blue");
	else
		strcpy(color, "red");

	strftime(time, sizeof(time), "%H:%M:%S", get_currenttime());
	if (issendmsg) {
		g_snprintf(text, sizeof(text) - 1,
			   "%s %s\n", config_get_nickname(), time);
	} else {
		g_snprintf(text, sizeof(text) - 1,
			   "%s %s\n", dlg->user->nickName, time);
		//add history
	}

	gtk_text_buffer_insert_with_tags_by_name(dlg->recv_buffer,
						 &dlg->recv_iter, text, -1,
						 color, NULL);
	//process the emotion tags
	msg_parse_emotion(dlg->recv_text, &dlg->recv_iter, msg);

	gtk_text_buffer_insert(dlg->recv_buffer, &dlg->recv_iter, "\n", -1);
	gtk_text_iter_set_line_offset(&dlg->recv_iter, 0);
	dlg->mark = gtk_text_buffer_get_mark(dlg->recv_buffer, "scroll");
	gtk_text_buffer_move_mark(dlg->recv_buffer, dlg->mark, &dlg->recv_iter);
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(dlg->recv_text),
					   dlg->mark);
}

void senddlg_add_info(SendDlg * dlg, const char *msg)
{
	GtkTextChildAnchor *anchor;
	GtkWidget *image;
	gchar text[MAX_BUF];
	gchar time[30];

	strftime(time, sizeof(time), "%H:%M:%S", get_currenttime());
	image = gtk_image_new_from_stock(GTK_STOCK_INFO, GTK_ICON_SIZE_MENU);
	gtk_widget_show(image);
	anchor =
	    gtk_text_buffer_create_child_anchor(dlg->recv_buffer,
						&dlg->recv_iter);
	gtk_text_view_add_child_at_anchor(GTK_TEXT_VIEW(dlg->recv_text), image,
					  anchor);

	g_snprintf(text, MAX_BUF, "  (%s) %s\n", time, msg);
	gtk_text_buffer_insert_with_tags_by_name(dlg->recv_buffer,
						 &dlg->recv_iter, text, -1,
						 "blue", "small", "tm10", NULL);
	gtk_text_buffer_insert(dlg->recv_buffer, &dlg->recv_iter, "\n", -1);

	gtk_text_iter_set_line_offset(&dlg->recv_iter, 0);
	dlg->mark = gtk_text_buffer_get_mark(dlg->recv_buffer, "scroll");
	gtk_text_buffer_move_mark(dlg->recv_buffer, dlg->mark, &dlg->recv_iter);
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(dlg->recv_text),
					   dlg->mark);
}

static void
recv_text_add_button(SendDlg * dlg, const char *label, const char *tip_text,
		     LinkButtonFunc func, gpointer data)
{
	GtkTextChildAnchor *anchor;
	GtkWidget *widget;

	widget = gtk_link_button_new_with_label(tip_text, label);
	gtk_link_button_set_uri_hook((GtkLinkButtonUriFunc) func, data, NULL);
	anchor =
	    gtk_text_buffer_create_child_anchor(dlg->recv_buffer,
						&dlg->recv_iter);
	gtk_text_view_add_child_at_anchor(GTK_TEXT_VIEW(dlg->recv_text), widget,
					  anchor);
	gtk_widget_show(widget);
	gtk_text_buffer_insert(dlg->recv_buffer, &dlg->recv_iter, "\n", -1);

	gtk_text_iter_set_line_offset(&dlg->recv_iter, 0);
	dlg->mark = gtk_text_buffer_get_mark(dlg->recv_buffer, "scroll");
	gtk_text_buffer_move_mark(dlg->recv_buffer, dlg->mark, &dlg->recv_iter);
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(dlg->recv_text),
					   dlg->mark);
}

static void show_recv_file_box(SendDlg *dlg , gboolean show)
{
	if((show && dlg->is_show_recv_file)
		|| (!show && !dlg->is_show_recv_file))
		return;
	if(show && !dlg->is_show_recv_file) {
		dlg->info_area_box = (GtkWidget *)g_object_ref(dlg->info_area_box);
		gtk_container_remove(GTK_CONTAINER(dlg->rvbox), dlg->info_area_box);
		gtk_container_add(GTK_CONTAINER(dlg->rvbox), dlg->recv_file_box);
		dlg->is_show_recv_file = TRUE;
	}
	if(!show && dlg->is_show_recv_file) {
		dlg->recv_file_box = (GtkWidget *)g_object_ref(dlg->recv_file_box);
		gtk_container_remove(GTK_CONTAINER(dlg->rvbox), dlg->recv_file_box);
		gtk_container_add(GTK_CONTAINER(dlg->rvbox), dlg->info_area_box);
		dlg->is_show_recv_file = FALSE;
	}
}

static void update_dlg_info(SendDlg * dlg)
{
	GdkPixbuf *pb;
	gchar *title;
	gchar *sign;
	gchar *name;
	gchar *info;
	char *addr_str;

	addr_str = get_addr_str(dlg->user->ipaddr);
	title =
	    g_strdup_printf("Chating with %s(IP: %s)", dlg->user->nickName,
			    addr_str);
	if(dlg->user->personalSign) {
		sign = g_strdup_printf("<i>%s</i>", dlg->user->personalSign);
	}
	else {
		sign = g_strdup_printf("<i>%s</i>", "No personal sign yet.");
	}
	name = g_strdup_printf("<b>%s</b>(%s) %s", dlg->user->nickName,
		dlg->user->hostName, addr_str);
	info = g_strdup_printf("<i>NickName</i>  :%s\n<i>HostName</i>  :%s\n"
		"<i>LoginName</i> :%s\n<i>GroupName</i>:%s\n<i>IPAddress</i>    :%s",
			       dlg->user->nickName, dlg->user->hostName,
			       dlg->user->userName, dlg->user->groupName, addr_str);
	FREE_WITH_CHECK(addr_str);

	if (dlg->user->headIcon) {
		pb =
		    gdk_pixbuf_new_from_file_at_size(dlg->user->headIcon, 30,
						     30, NULL);
		gtk_window_set_icon(GTK_WINDOW(dlg->dialog), pb);
		gtk_image_set_from_pixbuf(GTK_IMAGE(dlg->head_icon), pb);
		g_object_unref(pb);
	}
	if (dlg->user->photoImage) {
		pb =
		    gdk_pixbuf_new_from_file_at_size(dlg->user->photoImage, 140,
						     140, NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(dlg->photo_image), pb);
		g_object_unref(pb);
	}
	gtk_window_set_title(GTK_WINDOW(dlg->dialog), title);
	gtk_label_set_markup(GTK_LABEL(dlg->sign_label), sign);
	gtk_label_set_markup(GTK_LABEL(dlg->name_label), name);
	gtk_label_set_markup(GTK_LABEL(dlg->info_label), info);
	gtk_label_set_markup(GTK_LABEL(dlg->state_label), "(online)");

	g_free(sign);
	g_free(title);
	g_free(name);
	g_free(info);
}

static void update_recv_file_tree(GtkWidget * recv_file_tree, GList * files)
{
	GtkTreeModel *model;
	GdkPixbuf *pixbuf;
	FileInfo *info;
	bool is_folder;
	char filesize[MAX_NAMEBUF];
	char time[30];

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(recv_file_tree));
	GList *entry = files;
	strftime(time, sizeof(time), "%F %T", get_currenttime()); 
	while (entry) {
		GtkTreeIter iter;

		info = (FileInfo *) entry->data;
		is_folder = (info->attr & IPMSG_FILE_DIR) ? true : false;
		format_filesize(filesize, info->size);
		if (is_folder)
			pixbuf =
			    gdk_pixbuf_new_from_file(ICON_PATH "folder.png",
						     NULL);
		else
			pixbuf =
			    gdk_pixbuf_new_from_file(ICON_PATH "file.png",
						     NULL);
		gtk_list_store_append(GTK_LIST_STORE(model), &iter);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
				   RF_COL_ICON, pixbuf,
				   RF_COL_NAME, info->name,
				   RF_COL_TYPE, is_folder ? "folder" : "file",
				   RF_COL_SIZE, filesize,
				   RF_COL_PATH, "",
				   RF_COL_TIME, time,
				   RF_COL_INFO, info, -1);
		entry = entry->next;
	};
}

static void show_recv_progress_bar(SendDlg *dlg, gboolean show)
{
	if((show && dlg->is_show_recv_progress)
		|| (!show && !dlg->is_show_recv_progress))
		return;
	if(show && !dlg->is_show_recv_progress) {
		dlg->recv_action_hbox = (GtkWidget *)g_object_ref(dlg->recv_action_hbox);
		gtk_container_remove(GTK_CONTAINER(dlg->recv_action_vbox), dlg->recv_action_hbox);
		gtk_box_pack_start(GTK_BOX(dlg->recv_action_vbox),
			dlg->recv_progress_bar, FALSE, FALSE, 0);
		dlg->is_show_recv_progress = TRUE;
	}
	if(!show && dlg->is_show_recv_progress) {
		dlg->recv_progress_bar = (GtkWidget *)g_object_ref(dlg->recv_progress_bar);
		gtk_container_remove(GTK_CONTAINER(dlg->recv_action_vbox), dlg->recv_progress_bar);
		gtk_box_pack_start(GTK_BOX(dlg->recv_action_vbox),
			dlg->recv_action_hbox, FALSE, FALSE, 0);
		dlg->is_show_recv_progress = FALSE;
	}
}

static gboolean update_progress_bar(SendDlg *dlg)
{
	char buf[MAX_BUF];
	char tsize[100];
	char ssize[100];
	static char speed[100];

	format_filesize(tsize, dlg->recv_progress.tsize);
	format_filesize(ssize, dlg->recv_progress.ssize);
	if(dlg->recv_progress.speed) {
		format_filesize(speed, dlg->recv_progress.speed);
	}
	switch(dlg->recv_progress.tstatus) {
	case FILE_TS_READY:
	{
		snprintf(buf, MAX_BUF, "Recieve file %s", dlg->recv_progress.fname);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(dlg->recv_progress_bar), buf);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(dlg->recv_progress_bar),0);
		show_recv_progress_bar(dlg, TRUE);
		break;
	}
	case FILE_TS_DOING:
	{
		if(dlg->recv_progress.info->attr & IPMSG_FILE_REGULAR) {
			double progress;
			
			progress = percent(dlg->recv_progress.tsize, dlg->recv_progress.ssize);
			snprintf(buf, MAX_BUF, "%s/%s %s/s", tsize, ssize, speed);
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(dlg->recv_progress_bar),progress/100);
		}
		if(dlg->recv_progress.info->attr & IPMSG_FILE_DIR) {
			snprintf(buf, MAX_BUF, "%ld %s/s", dlg->recv_progress.ntrans, speed);
		}
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(dlg->recv_progress_bar), buf);
		show_recv_progress_bar(dlg, TRUE);
		break;
	}
	case FILE_TS_FINISH:
	{
		char recvsize[30];
		
		format_filesize(recvsize, dlg->recv_progress.tsize);
		dlg->recv_progress.tstatus = FILE_TS_NONE;

		snprintf(buf, MAX_BUF, "%s/%s %s/s", tsize, ssize, speed);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(dlg->recv_progress_bar), buf);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(dlg->recv_progress_bar),1);
		show_recv_progress_bar(dlg, FALSE);
		
		if(dlg->recv_progress.info->attr & IPMSG_FILE_DIR) {
			snprintf(buf, MAX_BUF, "Folder %s recieve finish! recieved size: %s, average speed %s/s",
				dlg->recv_progress.info->name, recvsize, speed);
		}
		else {
			snprintf(buf, MAX_BUF, "File %s recieve finish! recieved size: %s, average speed %s/s",
				dlg->recv_progress.info->name, recvsize, speed);
		}
		
		senddlg_add_info(dlg, buf);
		show_recv_progress_bar(dlg, FALSE);
		break;
	}
	case FILE_TS_NONE:
		break;
	default:
		break;
	}
	
	switch(dlg->send_progress.tstatus) {
	case FILE_TS_READY:
		break;
	case FILE_TS_DOING:
		break;
	case FILE_TS_FINISH:
		break;
	case FILE_TS_NONE:
		break;
	default:
		break;
	}

	return TRUE;
}

bool notify_send_finish(SendDlg * dlg, ulong ipaddr, packet_no_t packet_no)
{
	GList *list = dlg->send_list;
	bool ret = false;

	g_static_mutex_lock(&dlg->mutex);
	while (list) {
		SendEntry *entry = (SendEntry *) list->data;
		if (entry->status == ST_SENDMSG
		    && entry->user->ipaddr == ipaddr
		    && entry->packet_no == packet_no) {
			entry->status = ST_DONE;
			ret = true;
		}
		list = list->next;
	};
	g_static_mutex_unlock(&dlg->mutex);

	return ret;
}

void senddlg_add_fileattach(SendDlg * dlg, GList * files)
{
	char buf[MAX_BUF];
	snprintf(buf, MAX_BUF, "%s request send %d files(folders) to you,"
		 "please recieve at the right.",
		 dlg->user->nickName, g_list_length(files));
	senddlg_add_info(dlg, buf);
	update_recv_file_tree(dlg->recv_file_tree, files);
	show_recv_file_box(dlg, TRUE);
}
