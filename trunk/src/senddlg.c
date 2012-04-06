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

#include "common.h"

#define MSG_SEND_MAX_RETRY		3
#define MSG_RETRY_INTERVAL		1500

extern MainWindow main_win;

enum {
	SF_COL_ICON,
	SF_COL_NAME,
	SF_COL_TYPE,
	SF_COL_SIZE,
	SF_COL_PATH,
	SF_COL_INFO,
	SF_COL_NUM
};

enum {
	RF_COL_ICON,
	RF_COL_NAME,
	RF_COL_TYPE,
	RF_COL_SIZE,
	RF_COL_PATH,
	RF_COL_INFO,
	RF_COL_NUM
};

struct ButtonCallbackArgs {
	SendDlg *dlg;
	void *data;
};

typedef void *(*LinkButtonFunc) (GtkLinkButton * button, const gchar * link_,
				 gpointer data);

static void senddlg_ui_init(SendDlg * dlg);
static GtkTreeModel *create_file_tree_model();
static void create_file_tree_column(GtkTreeView * file_tree);
static void update_dlg_info(SendDlg * dlg);
static void send_msg(SendEntry * entry);
static void recv_text_add_button(SendDlg * dlg, const char *label,
				 const char *tip_text, LinkButtonFunc func,
				 gpointer data);
static gboolean retry_message_handler(gpointer data);

SendDlg *senddlg_new(User * user)
{
	SendDlg *dlg = (SendDlg *) malloc(sizeof(SendDlg));

	memset(dlg, 0, sizeof(SendDlg));

	if (!user) {
		free(dlg);
		return NULL;
	}
	dlg->timerId = 0;

	dlg->user = user;
	dlg->fontName = NULL;
	dlg->emotionDlg = NULL;
	dlg->send_list = NULL;
	g_static_mutex_init(&dlg->mutex);
	dlg->msg = NULL;
	dlg->is_recv_file = false;

	senddlg_ui_init(dlg);

	return dlg;
}

static gboolean on_close_clicked(GtkWidget * widget, SendDlg * dlg)
{
	main_win.sendDlgList = g_list_remove(main_win.sendDlgList, dlg);
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
	struct ButtonCallbackArgs *args = (struct ButtonCallbackArgs *)data;
	send_msg((SendEntry *) (args->data));
	args->dlg->timerId =
	    g_timeout_add(MSG_RETRY_INTERVAL, retry_message_handler, args->dlg);
	gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
	FREE_WITH_CHECK(args);
}

static gboolean retry_message_handler(gpointer data)
{
	SendDlg *dlg = (SendDlg *) data;
	GList *list = dlg->send_list;
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
				struct ButtonCallbackArgs *args =
				    (struct ButtonCallbackArgs *)
				    malloc(sizeof(struct ButtonCallbackArgs));
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
			list = dlg->send_list;
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
		send_file_info(dlg->user->ipaddr, files);
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
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), TRUE);
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

static void update_send_file_tree(GFile * file, SendDlg * dlg)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkIconInfo *iconinfo;
	GdkPixbuf *pixbuf;
	FileInfo *info;
	char filesize[MAX_NAMEBUF];

	info = (FileInfo *) malloc(sizeof(FileInfo));
	if (!setup_file_info(info, g_file_get_path(file)))
		return;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dlg->send_file_tree));
	//check whether the file is already added
	if (gtk_tree_model_get_iter_first(model, &iter)) {
		gchar *path;
		do {
			gtk_tree_model_get(model, &iter, SF_COL_PATH, &path,
					   -1);
			if (!strcmp(path, info->path)) {
				g_free(path);
				return;
			}
			g_free(path);
		} while (gtk_tree_model_iter_next(model, &iter));
	}
	pixbuf = get_file_icon(file);
	format_filesize(filesize, info->size);
	gtk_list_store_append(GTK_LIST_STORE(model), &iter);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			   SF_COL_ICON, pixbuf,
			   SF_COL_NAME, info->name,
			   SF_COL_TYPE,
			   (info->attr & IPMSG_FILE_DIR) ? "folder" : "file",
			   SF_COL_SIZE, filesize, SF_COL_PATH, info->path,
			   SF_COL_INFO, info, -1);
	g_object_unref(file);
	add_file(info);
}

static void on_send_file_clicked(GtkWidget * widget, SendDlg * dlg)
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Select file",
					     GTK_WINDOW(dlg->dialog),
					     GTK_FILE_CHOOSER_ACTION_OPEN,
					     GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OPEN,
					     GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), TRUE);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *files =
		    gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dialog));
		g_slist_foreach(files, (GFunc) update_send_file_tree, dlg);
		g_slist_free(files);
	}
	gtk_widget_destroy(dialog);
}

static void on_send_folder_clicked(GtkWidget * widget, SendDlg * dlg)
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Select file",
					     GTK_WINDOW(dlg->dialog),
					     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					     GTK_STOCK_CANCEL,
					     GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OPEN,
					     GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), TRUE);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *files =
		    gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dialog));
		g_slist_foreach(files, (GFunc) update_send_file_tree, dlg);
		g_slist_free(files);
	}
	gtk_widget_destroy(dialog);
}

static void senddlg_ui_init(SendDlg * dlg)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *halign;
	GtkWidget *hpaned;
	GtkWidget *vpaned;
	GtkWidget *lvbox;
	GtkWidget *rvbox;
	GtkWidget *tool_icon;
	GtkWidget *action_area;
	GtkWidget *button;
	GtkWidget *scroll;
	GtkWidget *frame;
	GdkPixbuf *pb;
	gchar nametext[512];
	gchar portraitPath[512];

	dlg->dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_modal(GTK_WINDOW(dlg->dialog), FALSE);
	gtk_window_set_default_size(GTK_WINDOW(dlg->dialog), 600, 430);
	gtk_container_set_border_width(GTK_CONTAINER(dlg->dialog), 10);
	g_signal_connect(dlg->dialog, "delete-event", G_CALLBACK(on_detete),
			 dlg);
	GdkColor bgcolor;
	if (gdk_color_parse("#c0ffff", &bgcolor))
		gtk_widget_modify_bg(dlg->dialog, GTK_STATE_NORMAL, &bgcolor);

	hpaned = gtk_hpaned_new();
	vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(dlg->dialog), vbox);
	action_area = gtk_hbox_new(FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 5);
	pb = gdk_pixbuf_new_from_file_at_size(ICON_PATH "icon.png", 40, 40,
					      NULL);
	gtk_window_set_icon(GTK_WINDOW(dlg->dialog), pb);
	dlg->head_icon = gtk_image_new_from_pixbuf(pb);
	g_object_unref(pb);

	gtk_box_pack_start(GTK_BOX(hbox), dlg->head_icon, FALSE, FALSE, 0);

	GtkWidget *vbox1 = gtk_vbox_new(TRUE, 0);
	GtkWidget *hbox1 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0);

	dlg->name_label = gtk_label_new(NULL);
	gtk_label_set_justify(GTK_LABEL(dlg->name_label), GTK_JUSTIFY_LEFT);

	gtk_box_pack_start(GTK_BOX(hbox1), dlg->name_label, FALSE, FALSE, 0);
	dlg->state_label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox1), dlg->state_label, FALSE, FALSE, 10);

	dlg->info_label = gtk_label_new(NULL);
	gtk_widget_set_size_request(dlg->info_label, 450, 20);
	gtk_misc_set_alignment(GTK_MISC(dlg->info_label), 0, 0);
	gtk_label_set_justify(GTK_LABEL(dlg->info_label), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX(vbox1), dlg->info_label, FALSE, FALSE, 0);

	halign = gtk_alignment_new(0, 0, 0, 0);
	gtk_container_add(GTK_CONTAINER(halign), hbox);

	gtk_box_pack_start(GTK_BOX(vbox), halign, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);
	lvbox = gtk_vbox_new(FALSE, 0);
	rvbox = gtk_vbox_new(FALSE, 0);
	gtk_paned_pack1(GTK_PANED(hpaned), lvbox, TRUE, FALSE);
	gtk_paned_pack2(GTK_PANED(hpaned), rvbox, FALSE, FALSE);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(lvbox), scroll, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
				       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll),
					    GTK_SHADOW_ETCHED_IN);
	dlg->recv_text = gtk_text_view_new();
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(dlg->recv_text), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(dlg->recv_text),
				    GTK_WRAP_CHAR);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(dlg->recv_text), FALSE);
	gtk_container_add(GTK_CONTAINER(scroll), dlg->recv_text);

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

	/*toolbar begin */
	dlg->toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(dlg->toolbar), GTK_TOOLBAR_ICONS);
	gtk_box_pack_start(GTK_BOX(lvbox), dlg->toolbar, FALSE, FALSE, 0);

	tool_icon =
	    gtk_image_new_from_stock(GTK_STOCK_SELECT_FONT,
				     GTK_ICON_SIZE_SMALL_TOOLBAR);
	dlg->change_font =
	    gtk_toolbar_append_item(GTK_TOOLBAR(dlg->toolbar), "Font",
				    "change font.", NULL, tool_icon,
				    G_CALLBACK(on_change_font_clicked), dlg);
	gtk_toolbar_append_space(GTK_TOOLBAR(dlg->toolbar));

	tool_icon = gtk_image_new_from_file(ICON_PATH "emotion.png");
	dlg->send_emotion = gtk_toolbar_append_item(GTK_TOOLBAR(dlg->toolbar),
						    "Emotion", "send emotion.",
						    NULL, tool_icon,
						    G_CALLBACK
						    (on_send_emotion_clicked),
						    dlg);
	gtk_toolbar_append_space(GTK_TOOLBAR(dlg->toolbar));

	tool_icon = gtk_image_new_from_file(ICON_PATH "image.png");
	dlg->send_image = gtk_toolbar_append_item(GTK_TOOLBAR(dlg->toolbar),
						  "Picture", "send picture.",
						  NULL, tool_icon,
						  G_CALLBACK
						  (on_send_image_clicked), dlg);
	gtk_toolbar_append_space(GTK_TOOLBAR(dlg->toolbar));

	tool_icon = gtk_image_new_from_file(ICON_PATH "file.png");
	dlg->send_file = gtk_toolbar_append_item(GTK_TOOLBAR(dlg->toolbar),
						 "File", "send file.", NULL,
						 tool_icon,
						 G_CALLBACK
						 (on_send_file_clicked), dlg);
	gtk_toolbar_append_space(GTK_TOOLBAR(dlg->toolbar));

	tool_icon = gtk_image_new_from_file(ICON_PATH "folder.png");
	dlg->send_folder = gtk_toolbar_append_item(GTK_TOOLBAR(dlg->toolbar),
						   "Folder", "send folder.",
						   NULL, tool_icon,
						   G_CALLBACK
						   (on_send_folder_clicked),
						   dlg);
	gtk_toolbar_append_space(GTK_TOOLBAR(dlg->toolbar));

	/*toolbar end */

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(lvbox), scroll, FALSE, FALSE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
				       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll)
					    , GTK_SHADOW_ETCHED_IN);
	dlg->send_text = gtk_text_view_new();
	gtk_widget_set_size_request(dlg->send_text, 0, 100);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(dlg->send_text),
				    GTK_WRAP_WORD_CHAR);
	gtk_container_add(GTK_CONTAINER(scroll), dlg->send_text);

	dlg->send_buffer =
	    gtk_text_view_get_buffer(GTK_TEXT_VIEW(dlg->send_text));
	gtk_text_buffer_get_iter_at_offset(dlg->send_buffer, &(dlg->send_iter),
					   0);

	halign = gtk_alignment_new(1, 0, 0, 0);
	gtk_container_add(GTK_CONTAINER(halign), action_area);
	gtk_box_pack_start(GTK_BOX(vbox), halign, FALSE, FALSE, 0);

	button = gtk_button_new_with_label("Close");
	gtk_widget_set_size_request(button, 100, 30);
	gtk_box_pack_start(GTK_BOX(action_area), button, FALSE, TRUE, 2);
	g_signal_connect(button, "clicked", G_CALLBACK(on_close_clicked), dlg);

	button = gtk_button_new_with_label("Send");
	gtk_widget_set_size_request(button, 100, 30);
	gtk_box_pack_start(GTK_BOX(action_area), button, FALSE, TRUE, 2);
	g_signal_connect(button, "clicked", G_CALLBACK(on_send_clicked), dlg);

	gtk_window_set_position(GTK_WINDOW(dlg->dialog), GTK_WIN_POS_CENTER);
	gint x, y;
	gtk_window_get_position(GTK_WINDOW(dlg->dialog), &x, &y);
	gtk_window_move(GTK_WINDOW(dlg->dialog), x + rand() % 100,
			y + rand() % 100);

	/*right box start */
	//top: photo Image/recieve file
	dlg->rt_box = gtk_vbox_new(FALSE, 0);
	dlg->rt_evbox = gtk_event_box_new();
	dlg->rt_label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(dlg->rt_label), "<b>Photo</b>");
	gtk_misc_set_alignment(GTK_MISC(dlg->rt_label), 0, 0.5);
	gtk_container_add(GTK_CONTAINER(dlg->rt_evbox), dlg->rt_label);
	gtk_box_pack_start(GTK_BOX(dlg->rt_box), dlg->rt_evbox, FALSE, FALSE,
			   0);
	dlg->photo_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(dlg->photo_frame),
				  GTK_SHADOW_ETCHED_IN);
	gtk_widget_set_size_request(dlg->photo_frame, 180, 160);

	sprintf(portraitPath, "%s", ICON_PATH "icon.png");
	pb = gdk_pixbuf_new_from_file_at_size(portraitPath, 140, 140, NULL);
	dlg->photo_image = gtk_image_new_from_pixbuf(pb);
	g_object_unref(pb);
	gtk_container_add(GTK_CONTAINER(dlg->photo_frame), dlg->photo_image);
	gtk_box_pack_start(GTK_BOX(dlg->rt_box), dlg->photo_frame, FALSE, FALSE,
			   0);
	gtk_box_pack_start(GTK_BOX(rvbox), dlg->rt_box, FALSE, FALSE, 0);

	//buttom: send file
	dlg->rb_box = gtk_vbox_new(FALSE, 0);
	dlg->rb_evbox = gtk_event_box_new();
	dlg->rb_label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(dlg->rb_label), "<b>Send File</b>");
	gtk_misc_set_alignment(GTK_MISC(dlg->rb_label), 0, 0.5);
	gtk_container_add(GTK_CONTAINER(dlg->rb_evbox), dlg->rb_label);
	gtk_box_pack_start(GTK_BOX(dlg->rb_box), dlg->rb_evbox, FALSE, FALSE,
			   0);
	dlg->send_file_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW
				       (dlg->send_file_scroll),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					    (dlg->send_file_scroll),
					    GTK_SHADOW_ETCHED_IN);
	gtk_widget_set_size_request(dlg->send_file_scroll, 180, 120);
	dlg->send_file_tree =
	    gtk_tree_view_new_with_model(create_file_tree_model());
	create_file_tree_column(GTK_TREE_VIEW(dlg->send_file_tree));
	gtk_container_add(GTK_CONTAINER(dlg->send_file_scroll),
			  dlg->send_file_tree);
	gtk_box_pack_start(GTK_BOX(dlg->rb_box), dlg->send_file_scroll, FALSE,
			   FALSE, 0);
	gtk_box_pack_start(GTK_BOX(rvbox), dlg->rb_box, TRUE, TRUE, 0);

	dlg->recv_file_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW
				       (dlg->recv_file_scroll),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					    (dlg->recv_file_scroll),
					    GTK_SHADOW_ETCHED_IN);
	gtk_widget_set_size_request(dlg->recv_file_scroll, 180, 160);
	dlg->recv_file_tree =
	    gtk_tree_view_new_with_model(create_file_tree_model());
	create_file_tree_column(GTK_TREE_VIEW(dlg->recv_file_tree));
	gtk_container_add(GTK_CONTAINER(dlg->recv_file_scroll),
			  dlg->recv_file_tree);

	/*end right box */

	GTK_WIDGET_SET_FLAGS(dlg->send_text, GTK_CAN_FOCUS);
	gtk_widget_grab_focus(dlg->send_text);

	update_dlg_info(dlg);
	gtk_widget_show_all(dlg->dialog);
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

static GtkTreeModel *create_file_tree_model()
{
	GtkListStore *store;

	store = gtk_list_store_new(SF_COL_NUM,
				   GDK_TYPE_PIXBUF, G_TYPE_STRING,
				   G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
				   G_TYPE_POINTER);

	return GTK_TREE_MODEL(store);
}

static void create_file_tree_column(GtkTreeView * file_tree)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes("Name",
							  renderer, "pixbuf",
							  SF_COL_ICON, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "text",
					    SF_COL_NAME, NULL);
	gtk_tree_view_append_column(file_tree, column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Type",
							  renderer, "text",
							  SF_COL_TYPE, NULL);
	gtk_tree_view_append_column(file_tree, column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Size",
							  renderer, "text",
							  SF_COL_SIZE, NULL);
	gtk_tree_view_append_column(file_tree, column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Path",
							  renderer, "text",
							  SF_COL_PATH, NULL);
	gtk_tree_view_append_column(file_tree, column);
}

static void update_dlg_info(SendDlg * dlg)
{
	GdkPixbuf *pixbuf;
	gchar *title;
	gchar *name;
	gchar *info;
	char *addr_str;

	addr_str = get_addr_str(dlg->user->ipaddr);
	title =
	    g_strdup_printf("Chating with %s(IP: %s)", dlg->user->nickName,
			    addr_str);
	name = g_strdup_printf("<b>%s</b>", dlg->user->nickName);
	info = g_strdup_printf("<i>%s</i>(<i>%s</i>) IP: %s Group: <i>%s</i>",
			       dlg->user->userName, dlg->user->hostName,
			       addr_str, dlg->user->groupName);
	FREE_WITH_CHECK(addr_str);

	if (dlg->user->headIcon) {
		pixbuf =
		    gdk_pixbuf_new_from_file_at_size(dlg->user->headIcon, 30,
						     30, NULL);
		gtk_window_set_icon(GTK_WINDOW(dlg->dialog), pixbuf);
		gtk_image_set_from_pixbuf(GTK_IMAGE(dlg->head_icon), pixbuf);
		g_object_unref(pixbuf);
	}
	if (dlg->user->photoImage) {
		pixbuf =
		    gdk_pixbuf_new_from_file_at_size(dlg->user->photoImage, 140,
						     140, NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(dlg->photo_image), pixbuf);
		g_object_unref(pixbuf);
	}
	gtk_window_set_title(GTK_WINDOW(dlg->dialog), title);
	gtk_label_set_markup(GTK_LABEL(dlg->name_label), name);
	gtk_label_set_markup(GTK_LABEL(dlg->info_label), info);
	gtk_label_set_markup(GTK_LABEL(dlg->state_label), "(online)");

	if (title)
		g_free(title);
	if (name)
		g_free(name);
	if (info)
		g_free(info);
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

static void update_recv_file_tree(SendDlg * dlg, GSList * files)
{
	GtkTreeModel *model;
	GdkPixbuf *pixbuf;
	FileInfo *info;
	bool is_folder;
	char filesize[MAX_NAMEBUF];

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dlg->recv_file_tree));
	GSList *entry = files;
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
				   RF_COL_INFO, info, -1);
		recv_file_entry(info, "/home/tsl0922/recv", dlg->user);
		entry = entry->next;
	};
}

static void show_recv_file_area(SendDlg * dlg)
{
	if (dlg->is_recv_file)
		return;
	gtk_container_remove(GTK_CONTAINER(dlg->rt_box), dlg->photo_frame);
	gtk_label_set_markup(GTK_LABEL(dlg->rt_label), "<b>Recieve File</b>");
	gtk_container_add(GTK_CONTAINER(dlg->rt_box), dlg->recv_file_scroll);
	gtk_widget_show_all(dlg->rt_box);

}

void senddlg_add_fileattach(SendDlg * dlg, GSList * files)
{
	char buf[MAX_BUF];
	snprintf(buf, MAX_BUF, "%s request send %d files(folders) to you,\n"
		 "please recieve at the right.",
		 dlg->user->nickName, g_slist_length(files));
	senddlg_add_info(dlg, buf);
	show_recv_file_area(dlg);
	dlg->is_recv_file = true;
	update_recv_file_tree(dlg, files);
}
