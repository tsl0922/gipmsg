#include "common.h"

#define MSG_SEND_MAX_RETRY		3
#define MSG_RETRY_INTERVAL		1500

enum {
	U_COL_HEADICON,
	U_COL_NICKNAME,
	U_COL_HOSTNAME,
	U_COL_USERNAME,
	U_COL_GROUPNAME,
	U_COL_IPADDRESS,
	U_COL_DATA,
	U_COL_NUM
};

enum {
	F_COL_ICON,
	F_COL_NAME,
	F_COL_TYPE,
	F_COL_SIZE,
	F_COL_PATH,
	F_COL_NUM
};

static GtkTreeModel *create_user_tree_model();
static void create_user_tree_column(GtkTreeView *user_tree);
static void init_user_tree_data(GtkTreeView *user_tree, User **users, size_t len);
static GtkTreeModel *create_file_tree_model();
static void create_file_tree_column(GtkTreeView *file_tree);
static void update_dlg_info(SendDlg *dlg);

SendDlg *
senddlg_new(DialogArgs *args)
{
	SendDlg *dlg = (SendDlg *)malloc(sizeof(SendDlg));
	
	memset(dlg, 9, sizeof(SendDlg));

	if(!args) {
		free(dlg);
		return NULL;
	}
	dlg->sendEntry = NULL;
	dlg->sendEntryNum = 0;
	dlg->msgBuf = NULL;
	dlg->timerId = 0;
	dlg->retryCount = 0;
	dlg->recv_anchor = NULL;
	
	dlg->main_win = args->main_win;
	dlg->users = args->users;
	dlg->len = args->len;
	dlg->fontName = NULL;
	dlg->emotionDlg = NULL;
	
	senddlg_init(dlg);

	return dlg;
}

void
senddlg_free(SendDlg *dlg)
{
	FREE_WITH_CHECK(dlg->users);
	FREE_WITH_CHECK(dlg);
}

User *
getUser(SendDlg *dlg, ulong ipaddr)
{
	User *user = NULL;
	int i;
	for(i = 0;i < dlg->len;i++) {
		user = dlg->users[i];
		if(ipaddr == user->ipaddr) {
			break;
		}
	}
	
	return user;
}

bool
IsSamePacket(Message *msg)
{
}

static void
MakePacket(const char *msgBuf, SendEntry *entry)
{
	packet_no_t packet_no;
	
	build_packet(entry->packet, entry->command, msgBuf,
		NULL, &entry->packet_len, &entry->packet_no);
	entry->status = ST_SENDMSG;
}

static void
SendMsgEntry(SendDlg *dlg, SendEntry *entry)
{
	if(entry->status == ST_MAKEMSG) {
		MakePacket((const char *)dlg->msgBuf, entry);
	}
	if(entry->status == ST_SENDMSG) {
		ipmsg_send_packet(entry->user->ipaddr, entry->packet, entry->packet_len);
	}
}

static gpointer
send_msg_thread(gpointer data)
{
	SendDlg *dlg = (SendDlg *)data;
	gint i;

	for(i = 0; i < dlg->sendEntryNum; i++) {
		SendMsgEntry(dlg, dlg->sendEntry + i);
	}
	return NULL;
}

static void
SendMsg(SendDlg *dlg)
{

	if(dlg->sendEntry == NULL || dlg->sendEntryNum < 1) return;
	g_thread_create((GThreadFunc)send_msg_thread, dlg, FALSE, NULL);
}

static bool
IsSendFinish(SendDlg *dlg)
{
	bool finish = true;
	int i;

	if(dlg->sendEntry == NULL) return true;
	for (i = 0; i < dlg->sendEntryNum; i++) {
		if (dlg->sendEntry[i].status != ST_DONE) {
			finish = FALSE;
			break;
		}
	}
	
	return	finish;
}

static gboolean 
retry_message_handler(gpointer data)
{
	SendDlg *dlg = (SendDlg *)data;

	g_source_remove(dlg->timerId);
	dlg->timerId = 0;
	if(IsSendFinish(dlg)) {
		return TRUE;
	}
	if(dlg->retryCount++ < MSG_SEND_MAX_RETRY) {
		DEBUG_INFO("retry: %d", dlg->retryCount);
		SendMsg(dlg);
		dlg->timerId = g_timeout_add(MSG_RETRY_INTERVAL, retry_message_handler, dlg);
		return TRUE;
	}
	//send fail
	DEBUG_ERROR("Send fail!");
	char *tmpBuf;
	gint i;
	
	dlg->timerId = 0;
	tmpBuf = malloc(MAX_BUF);
	memset(tmpBuf, 0, sizeof(tmpBuf));
	for(i = 0;i < dlg->sendEntryNum;i++) {
		if(dlg->sendEntry[i].status != ST_DONE) {
			char *addr_str = get_addr_str(dlg->sendEntry[i].user->ipaddr);
			sprintf(tmpBuf+strlen(tmpBuf), "%s %s\n",
				dlg->sendEntry[i].user->nickName, addr_str);
			FREE_WITH_CHECK(addr_str);
		}
	}
	senddlg_add_fail_info(dlg, tmpBuf);
	FREE_WITH_CHECK(tmpBuf);

	return TRUE;
}

bool
SendFinishNotify(SendDlg *dlg, ulong ipaddr, packet_no_t packet_no)
{
	gint i;
	bool ret = false;

	for(i = 0;i < dlg->sendEntryNum;i++) {
		if(dlg->sendEntry[i].status == ST_SENDMSG && 
			dlg->sendEntry[i].user->ipaddr == ipaddr && 
			dlg->sendEntry[i].packet_no == packet_no)
		{
			dlg->sendEntry[i].status = ST_DONE;
			if(IsSendFinish(dlg)) {
				FREE_WITH_CHECK(dlg->sendEntry);
			}
			ret = true;
		}
	}
	
	return ret;
}

static gboolean
on_close_clicked(GtkWidget *widget, SendDlg *dlg)
{
	if(!IsSendFinish(dlg)) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(dlg->dialog), 
			GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
			"Some Message didn't send finish, close anyway?");
		gint result = gtk_dialog_run(GTK_DIALOG(dialog));
		if(result == GTK_RESPONSE_NO) {
			return FALSE;
		}
		gtk_widget_destroy(dialog);
	}
	dlg->main_win->sendDlgList = g_list_remove(dlg->main_win->sendDlgList, dlg);
	senddlg_free(dlg);
	gtk_widget_destroy(dlg->dialog);
	
	return TRUE;
}

static gboolean
on_detete(GtkWidget *widget, GdkEvent *event, SendDlg *dlg)
{
	return on_close_clicked(NULL, dlg);
}

static void
send_file_attach_info(SendDlg *dlg)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GSList *files = NULL;
	gchar *path;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dlg->file_tree));
	if(!gtk_tree_model_get_iter_first(model, &iter))
		return;
	do {
		gtk_tree_model_get(model, &iter, F_COL_PATH, &path, -1);
		files = g_slist_append(files, path);
	} while(gtk_tree_model_iter_next(model, &iter));
	if(files) SendFileInfo(dlg->users[0]->ipaddr, files);
}

static void
on_send_clicked(GtkWidget *widget, SendDlg *dlg)
{
	GtkTextIter      begin, end;
	gchar           *text;
	gint count, i;
	command_no_t command = IPMSG_SENDMSG | IPMSG_SENDCHECKOPT;

	send_file_attach_info(dlg);
	gtk_text_buffer_get_start_iter(dlg->send_buffer , &begin);
	gtk_text_buffer_get_end_iter(dlg->send_buffer , &end);
	dlg->msgBuf = gtk_text_buffer_get_text(dlg->send_buffer,
			&begin, &end, TRUE);
	if(*dlg->msgBuf == '\0') return;
	if(dlg->len == 1) {
		count = 1;dlg->sendEntryNum = 1;
		dlg->sendEntry = (SendEntry *)malloc(sizeof(SendEntry));
		dlg->sendEntry[0].user = dlg->users[0];
	}
	else {
		//FIXME:multiuser send
		return;
		GtkTreeModel *model;
		GtkTreeSelection *selection;
		GtkTreePath *path;
		GtkTreeIter iter;
		GList *slist;
		
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(dlg->user_tree));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dlg->user_tree));
		slist = gtk_tree_selection_get_selected_rows(selection, NULL);
		count = gtk_tree_selection_count_selected_rows(selection);
		
		dlg->sendEntry = (SendEntry *)malloc(count * sizeof(SendEntry));
		dlg->sendEntryNum = count;
		for(i = 0;i < count;i++) {
			path = g_list_nth_data(slist, i);
			if(gtk_tree_model_get_iter(model, &iter, path)) {
				gtk_tree_model_get(model, &iter, DATA_COLUMN, &dlg->sendEntry[i], -1);
			}
		}
	}
	if (dlg->sendEntryNum > 1)
		command |= IPMSG_MULTICASTOPT;
	for(i = 0;i< count;i++) {
		dlg->sendEntry[i].status = ST_MAKEMSG;
		dlg->sendEntry[i].command = command;
	}

	senddlg_add_message(dlg, NULL, dlg->msgBuf, get_currenttime(), true);
	dlg->retryCount = 0;
	SendMsg(dlg);
	
	gtk_text_buffer_delete(dlg->send_buffer , &begin , &end);
	dlg->timerId = g_timeout_add(MSG_RETRY_INTERVAL, retry_message_handler, dlg);
}

static void
on_change_font_clicked(GtkWidget *widget, SendDlg *dlg)
{
	GtkWidget *dialog;
	PangoFontDescription *fontDesc;
	
	dialog = gtk_font_selection_dialog_new("Change Font");
	gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(dialog), dlg->fontName);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		dlg->fontName = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(dialog));
		fontDesc = pango_font_description_from_string(dlg->fontName);
		gtk_widget_modify_font(dlg->send_text, fontDesc);
		gtk_widget_modify_font(dlg->recv_text, fontDesc);
	}
	gtk_widget_destroy(dialog);
}

static void
on_send_emotion_clicked(GtkWidget *widget, SendDlg *dlg)
{
	
	int x , y , ex , ey , root_x , root_y;

	gtk_widget_translate_coordinates(widget , dlg->dialog, 0 , 0 , &ex , &ey );
	gtk_window_get_position(GTK_WINDOW(dlg->dialog) , &root_x , &root_y);
	x = root_x + ex + 3;
	y = root_y + ey - 165;

	show_emotion_dialog(dlg, x, y);
}

static void
on_send_image_clicked(GtkWidget *widget, SendDlg *dlg)
{
	GtkWidget *dialog;
	gchar *filename = NULL;
	GtkFileFilter *filter;
		
	dialog = gtk_file_chooser_dialog_new("Select file",
				GTK_WINDOW(dlg->dialog),
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), TRUE);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Image Files(*.jpg;*.jpeg;*.png;*.gif)");
	gtk_file_filter_add_pattern(filter, "*.jpg");
	gtk_file_filter_add_pattern(filter, "*.jpeg");
	gtk_file_filter_add_pattern(filter, "*.png");
	gtk_file_filter_add_pattern(filter, "*.gif");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		if(filename) g_print("%s\n", filename);
		//FIXME: send image
		if(filename) g_free(filename);
	}
	gtk_widget_destroy(dialog);
}

static void
update_file_tree(GFile *file, SendDlg *dlg) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkIconInfo *iconinfo;
	GdkPixbuf *pixbuf;
	GFileInfo *fileinfo;
	GFileType filetype;
	char filesize[MAX_NAMEBUF];

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(dlg->file_tree));
	//check whether the file is already added
	if(gtk_tree_model_get_iter_first(model, &iter)) {
		gchar *path;
		do {
			gtk_tree_model_get(model, &iter, F_COL_PATH, &path, -1);
			if(!strcmp(path, g_file_get_path(file))){
				g_free(path);
				return;
			}
			g_free(path);
		} while(gtk_tree_model_iter_next(model, &iter));
	}
	
	fileinfo = g_file_query_info(file, "standard::*", 0, NULL, NULL);
	g_print("%s\n", g_file_info_get_name(fileinfo));
	filetype = g_file_info_get_file_type(fileinfo);
	iconinfo = gtk_icon_theme_lookup_by_gicon(gtk_icon_theme_get_default(),
		g_file_info_get_icon(fileinfo), 24, 0);
	pixbuf = gtk_icon_info_load_icon(iconinfo, NULL);
	gtk_icon_info_free(iconinfo);
	format_filesize(filesize, g_file_info_get_size(fileinfo));
	gtk_list_store_append(GTK_LIST_STORE(model), &iter);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter,
		F_COL_ICON, pixbuf,
		F_COL_NAME, g_file_info_get_display_name(fileinfo),
		F_COL_TYPE, (filetype == G_FILE_TYPE_DIRECTORY)?"deiectory":"file",
		F_COL_SIZE, (filetype == G_FILE_TYPE_DIRECTORY)?"":filesize,
		F_COL_PATH, g_file_get_path(file),
		-1
		);
	g_object_unref(file);
}

static void
on_send_file_clicked(GtkWidget *widget, SendDlg *dlg)
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Select file",
				GTK_WINDOW(dlg->dialog),
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), TRUE);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *files = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dialog));
		g_slist_foreach(files, (GFunc)update_file_tree, dlg);
		g_slist_free(files);
	}
	gtk_widget_destroy(dialog);
}

static void
on_send_folder_clicked(GtkWidget *widget, SendDlg *dlg)
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new("Select file",
				GTK_WINDOW(dlg->dialog),
				GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), TRUE);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *files = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dialog));
		g_slist_foreach(files, (GFunc)update_file_tree, dlg);
		g_slist_free(files);
	}
	gtk_widget_destroy(dialog);
}

void
senddlg_init(SendDlg *dlg) {
	GtkWidget *vbox;
	GtkWidget *halign;
	GtkWidget *halign1;
	GtkWidget *hpaned;
	GtkWidget *lvbox;
	GtkWidget *rvbox;
	GtkWidget *tool_icon;
	GtkWidget *action_area;
	GtkWidget *send_button;
	GtkWidget *close_button;
	GtkWidget *frame;
	GtkWidget *img;
	GdkPixbuf *pb;
	gchar      nametext[512];
	gchar      portraitPath[512];
	
	dlg->dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_modal(GTK_WINDOW(dlg->dialog) , FALSE);
	gtk_window_set_default_size(GTK_WINDOW(dlg->dialog) , 600 , 430);
	gtk_container_set_border_width(GTK_CONTAINER(dlg->dialog) , 10);
	g_signal_connect(dlg->dialog , "delete-event" , G_CALLBACK(on_detete) , dlg);
	
	hpaned = gtk_hpaned_new();
	vbox = gtk_vbox_new(FALSE , 2);
	gtk_container_add(GTK_CONTAINER(dlg->dialog) , vbox);
	action_area = gtk_hbox_new(FALSE , 0);

	dlg->headbox = gtk_hbox_new(FALSE, 5);

	pb = gdk_pixbuf_new_from_file_at_size(config_get_icon_image(), 40 , 40 , NULL);
	gtk_window_set_icon(GTK_WINDOW(dlg->dialog), pb);
	dlg->headimage = gtk_image_new_from_pixbuf(pb);

	gtk_box_pack_start(GTK_BOX(dlg->headbox), dlg->headimage,
								FALSE, FALSE, 0);

	dlg->name_label = gtk_label_new(NULL);
	gtk_label_set_justify(GTK_LABEL(dlg->name_label) , GTK_JUSTIFY_LEFT);

	dlg->name_box = gtk_event_box_new();

	gtk_container_add(GTK_CONTAINER(dlg->name_box) , dlg->name_label);

	GtkWidget *vbox1 = gtk_vbox_new(TRUE, 0);
	GtkWidget *hbox1 = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(dlg->headbox), vbox1,
								FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox1), dlg->name_box, FALSE, FALSE, 0);
	dlg->input_label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox1), dlg->input_label, FALSE, FALSE, 10);

	dlg->info_label = gtk_label_new(NULL);
	gtk_widget_set_size_request(dlg->info_label, 450, 20);
	gtk_misc_set_alignment(GTK_MISC(dlg->info_label), 0, 0);
	gtk_label_set_justify(GTK_LABEL(dlg->info_label) , GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX(vbox1), dlg->info_label, FALSE, FALSE, 0);

	halign = gtk_alignment_new( 0 , 0 , 0 , 0);
	gtk_container_add(GTK_CONTAINER(halign) , dlg->headbox);

	gtk_box_pack_start(GTK_BOX(vbox) , halign , FALSE , FALSE , 5);
	gtk_box_pack_start(GTK_BOX(vbox) , hpaned , TRUE , TRUE , 0);
	lvbox = gtk_vbox_new(FALSE , 0);
	rvbox = gtk_vbox_new(FALSE , 0);
	gtk_paned_pack1(GTK_PANED(hpaned) , lvbox , TRUE , FALSE);
	gtk_paned_pack2(GTK_PANED(hpaned) , rvbox , FALSE , FALSE);
	dlg->recv_scroll = gtk_scrolled_window_new(NULL , NULL);
	gtk_box_pack_start(GTK_BOX(lvbox) , dlg->recv_scroll, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(dlg->recv_scroll)
								 , GTK_POLICY_NEVER
								 , GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(dlg->recv_scroll)
									  , GTK_SHADOW_ETCHED_IN);
	dlg->recv_text = gtk_text_view_new();
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(dlg->recv_text) , FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(dlg->recv_text) , GTK_WRAP_CHAR);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(dlg->recv_text) , FALSE);
	gtk_container_add(GTK_CONTAINER(dlg->recv_scroll) , dlg->recv_text);

	dlg->recv_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(dlg->recv_text));
	gtk_text_buffer_create_tag(dlg->recv_buffer , "blue" , "foreground" , "#639900" , NULL);
	gtk_text_buffer_create_tag(dlg->recv_buffer , "grey" , "foreground" , "#808080" , NULL);
	gtk_text_buffer_create_tag(dlg->recv_buffer , "green" , "foreground" , "green" , NULL);
	gtk_text_buffer_create_tag(dlg->recv_buffer , "red" , "foreground" , "#0088bf" , NULL);
	gtk_text_buffer_create_tag(dlg->recv_buffer , "bred" , "background" , "red" , NULL);
	gtk_text_buffer_create_tag(dlg->recv_buffer , "lm10" , "left_margin" , 10 , NULL);
	gtk_text_buffer_create_tag(dlg->recv_buffer , "tm10" , "pixels-below-lines" , 2 , NULL);
	gtk_text_buffer_create_tag(dlg->recv_buffer , "small" , "left_margin" , 5 , NULL);
	gtk_text_buffer_get_end_iter(dlg->recv_buffer , &(dlg->recv_iter));
	gtk_text_buffer_create_mark(dlg->recv_buffer , "scroll" , &(dlg->recv_iter) , FALSE);

	/*toolbar begin*/
	dlg->toolbar = gtk_toolbar_new();
	gtk_toolbar_set_style(GTK_TOOLBAR(dlg->toolbar) , GTK_TOOLBAR_ICONS);
	gtk_box_pack_start(GTK_BOX(lvbox) , dlg->toolbar , FALSE , FALSE , 0);

	tool_icon = gtk_image_new_from_stock(GTK_STOCK_SELECT_FONT, GTK_ICON_SIZE_SMALL_TOOLBAR);
	dlg->change_font = gtk_toolbar_append_item(GTK_TOOLBAR(dlg->toolbar),
					    "Font" , "change font." , NULL , tool_icon,
					    G_CALLBACK(on_change_font_clicked), dlg );
	gtk_toolbar_append_space(GTK_TOOLBAR(dlg->toolbar));

	tool_icon = gtk_image_new_from_file(IMAGE_DIR "emotion.png");
	dlg->send_emotion = gtk_toolbar_append_item(GTK_TOOLBAR(dlg->toolbar),
					    "Emotion" , "send emotion." , NULL , tool_icon,
					    G_CALLBACK(on_send_emotion_clicked), dlg );
	gtk_toolbar_append_space(GTK_TOOLBAR(dlg->toolbar));
	
	tool_icon = gtk_image_new_from_file(IMAGE_DIR "image.png");
	dlg->send_image = gtk_toolbar_append_item(GTK_TOOLBAR(dlg->toolbar),
					    "Picture" , "send picture." , NULL , tool_icon,
					    G_CALLBACK(on_send_image_clicked), dlg );
	gtk_toolbar_append_space(GTK_TOOLBAR(dlg->toolbar));

	tool_icon = gtk_image_new_from_file(IMAGE_DIR "file.png");
	dlg->send_file = gtk_toolbar_append_item(GTK_TOOLBAR(dlg->toolbar),
					    "File" , "send file." , NULL , tool_icon,
					    G_CALLBACK(on_send_file_clicked), dlg );
	gtk_toolbar_append_space(GTK_TOOLBAR(dlg->toolbar));

	tool_icon = gtk_image_new_from_file(IMAGE_DIR "folder.png");
	dlg->send_folder = gtk_toolbar_append_item(GTK_TOOLBAR(dlg->toolbar),
					    "Folder" , "send folder." , NULL , tool_icon,
					    G_CALLBACK(on_send_folder_clicked), dlg );
	gtk_toolbar_append_space(GTK_TOOLBAR(dlg->toolbar));
	
	/*toolbar end*/
	
	dlg->send_scroll = gtk_scrolled_window_new(NULL , NULL);
	gtk_box_pack_start(GTK_BOX(lvbox) , dlg->send_scroll , FALSE , FALSE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(dlg->send_scroll)
								 , GTK_POLICY_NEVER
								 , GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(dlg->send_scroll)
									  , GTK_SHADOW_ETCHED_IN);
	dlg->send_text = gtk_text_view_new();
	gtk_widget_set_size_request(dlg->send_text , 0 , 100);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(dlg->send_text) , GTK_WRAP_WORD_CHAR);
	gtk_container_add(GTK_CONTAINER(dlg->send_scroll) , dlg->send_text);

 	dlg->send_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(dlg->send_text));
	gtk_text_buffer_get_iter_at_offset(dlg->send_buffer , &(dlg->send_iter) , 0);

	halign1 = gtk_alignment_new( 1 , 0 , 0 , 0);
	gtk_container_add(GTK_CONTAINER(halign1) , action_area);
	gtk_box_pack_start(GTK_BOX(vbox) , halign1 , FALSE , FALSE , 0);

	close_button = gtk_button_new_with_label("Close");
	gtk_widget_set_size_request(close_button , 100 , 30);
	gtk_box_pack_start(GTK_BOX(action_area) , close_button , FALSE , TRUE , 2);
	g_signal_connect(close_button , "clicked" , G_CALLBACK(on_close_clicked) , dlg);

	send_button = gtk_button_new_with_label("Send");
	gtk_widget_set_size_request(send_button , 100 , 30);
	gtk_box_pack_start(GTK_BOX(action_area) , send_button , FALSE , TRUE , 2);
	g_signal_connect(send_button , "clicked" , G_CALLBACK(on_send_clicked) , dlg);

	gtk_window_set_position(GTK_WINDOW(dlg->dialog) , GTK_WIN_POS_CENTER);
	gint x,y;
	gtk_window_get_position(GTK_WINDOW(dlg->dialog), &x, &y);
	gtk_window_move(GTK_WINDOW(dlg->dialog), x + rand()%100, y + rand()%100);

	/*right box start */
	if(dlg->len == 1) {//single user
		//top: photo Image
		frame = gtk_frame_new(NULL);
		gtk_frame_set_shadow_type(GTK_FRAME(frame) , GTK_SHADOW_ETCHED_IN);
		gtk_widget_set_size_request(frame , 180 , 160);

		sprintf(portraitPath , "%s" , config_get_icon_image());
		pb = gdk_pixbuf_new_from_file_at_size(portraitPath , 140 , 140 , NULL);

		img = gtk_image_new_from_pixbuf(pb);
		gtk_container_add(GTK_CONTAINER(frame) , img);
		gtk_box_pack_start(GTK_BOX(rvbox) , frame , FALSE , FALSE , 0);
		//buttom: send file		
		dlg->file_tree_scroll = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(dlg->file_tree_scroll),
										GTK_POLICY_AUTOMATIC,
										GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(dlg->file_tree_scroll),
										GTK_SHADOW_ETCHED_IN);
		gtk_widget_set_size_request(dlg->file_tree_scroll, 180, 120);
		dlg->file_tree = gtk_tree_view_new_with_model(create_file_tree_model());
		create_file_tree_column(GTK_TREE_VIEW(dlg->file_tree));
		gtk_container_add(GTK_CONTAINER(dlg->file_tree_scroll), dlg->file_tree);
		gtk_box_pack_start(GTK_BOX(rvbox) , dlg->file_tree_scroll , TRUE , TRUE , 0);
	}
	else {//multiple user
		//user list
		dlg->user_tree_scroll = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(dlg->user_tree_scroll),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(dlg->user_tree_scroll),
									GTK_SHADOW_ETCHED_IN);
		gtk_widget_set_size_request(dlg->user_tree_scroll, 180, 200);
		dlg->user_tree = gtk_tree_view_new_with_model(create_user_tree_model());
		create_user_tree_column(GTK_TREE_VIEW(dlg->user_tree));
		gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(dlg->user_tree)), GTK_SELECTION_MULTIPLE);
		init_user_tree_data(GTK_TREE_VIEW(dlg->user_tree), dlg->users, dlg->len);
		gtk_tree_selection_select_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(dlg->user_tree)));
		gtk_container_add(GTK_CONTAINER(dlg->user_tree_scroll), dlg->user_tree);
		gtk_box_pack_start(GTK_BOX(rvbox) , dlg->user_tree_scroll , TRUE , TRUE , 0);
	}
	GtkWidget *spliter = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(rvbox) , spliter , FALSE , TRUE , 0);
	/*end right box */
	
	GTK_WIDGET_SET_FLAGS(dlg->send_text, GTK_CAN_FOCUS);
	gtk_widget_grab_focus(dlg->send_text);
	
	update_dlg_info(dlg);

	gtk_widget_show_all (vbox);
	gtk_widget_show(dlg->dialog);
}

void
senddlg_add_message(SendDlg *dlg, User *user, const char *msg,
	const struct tm *datetime , bool issendmsg)
{
	GtkTextChildAnchor *anchor;
	GtkTextIter         iter;
	GtkWidget          *pb;
	GtkTextBuffer      *buffer;
	gchar              text[4096];
	gchar              color[10];
	gchar              time[30];
	struct tm         *now;

	if(issendmsg == 1)
		strcpy(color , "blue");
	else
		strcpy(color , "red");

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(dlg->recv_text));

	if(issendmsg){
		strftime(time, sizeof(time), "%H:%M:%S", datetime);
		g_snprintf(text, sizeof(text) - 1,
				"%s %s\n", config_get_nickname(), time);
	}
	else{
		if(user == NULL) return;
		now = get_currenttime();
		strftime(time, sizeof(time), "%H:%M:%S", now);
		
		g_snprintf(text, sizeof(text) - 1,
				"%s %s\n", user->nickName, time);
		//add history
	}

	gtk_text_buffer_get_end_iter(buffer , &iter );
	gtk_text_buffer_insert_with_tags_by_name(buffer
					, &iter , text , -1 , color , NULL);
	//process the emotion tags
	msg_parse_emotion(dlg->recv_text, &iter, msg);
//		gtk_text_buffer_insert_with_tags_by_name(buffer
//						, &iter , msg , -1 , "tm10" , NULL);
	
	gtk_text_buffer_insert(buffer , &iter , "\n" , -1);
	gtk_text_iter_set_line_offset (&iter, 0);
	dlg->mark = gtk_text_buffer_get_mark (buffer, "scroll");
	gtk_text_buffer_move_mark (buffer, dlg->mark, &iter);
	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW(dlg->recv_text), dlg->mark);
}

static void
on_retry_link_clicked(GtkLinkButton *button, const gchar *link_, SendDlg *dlg) {
	if(!IsSendFinish(dlg)) {
		dlg->retryCount = 0;
		SendMsg(dlg);
		dlg->timerId = g_timeout_add(MSG_RETRY_INTERVAL, retry_message_handler, dlg);
	}
	if(dlg->recv_anchor) {
		if(!gtk_text_child_anchor_get_deleted(dlg->recv_anchor)) {
			GList *wlist;
			GtkWidget *widget;
			wlist = gtk_text_child_anchor_get_widgets(dlg->recv_anchor);
			widget = (GtkWidget *)g_list_nth_data(wlist, 0);
			gtk_widget_set_sensitive(widget, FALSE);
			g_list_free(wlist);
		}
		dlg->recv_anchor = NULL;
	}
}

void 
senddlg_add_fail_info(SendDlg *dlg, const char *msg)
{
	GtkTextIter    iter;
	GtkTextBuffer *buffer;
	GtkWidget *widget;
	gchar time[30];

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(dlg->recv_text));

	gtk_text_buffer_get_end_iter(buffer , &iter );
	gtk_text_buffer_insert(buffer , &iter , "" , -1);

	strftime(time, sizeof(time), "%H:%M:%S\n", get_currenttime());
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
					time , -1 , "grey", "small" , NULL);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
					"WARNING:" , -1 , "bred", "small" , NULL);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
					"   Failed to send message to users:\n" , -1 ,
					"grey", "lm10", "small" , NULL);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
					msg , -1 , "blue", "small", "tm10", NULL);
	//create Retry button
	widget = gtk_link_button_new_with_label("Retry send unsended messages.", "Retry");
	gtk_link_button_set_uri_hook((GtkLinkButtonUriFunc)on_retry_link_clicked, dlg, NULL);
	dlg->recv_anchor = gtk_text_buffer_create_child_anchor(buffer, &iter);
	gtk_text_view_add_child_at_anchor(GTK_TEXT_VIEW(dlg->recv_text),widget, dlg->recv_anchor);
	gtk_widget_show(widget);
	gtk_text_buffer_insert(buffer , &iter , "\n" , -1);
	
	gtk_text_iter_set_line_offset (&iter, 0);
	dlg->mark = gtk_text_buffer_get_mark (buffer, "scroll");
	gtk_text_buffer_move_mark(buffer, dlg->mark, &iter);
	gtk_text_view_scroll_mark_onscreen(
			GTK_TEXT_VIEW(dlg->recv_text), dlg->mark);
}

static GtkTreeModel *
create_user_tree_model() {
	GtkListStore *store;

	store = gtk_list_store_new(U_COL_NUM,
		GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

	return GTK_TREE_MODEL(store);
}

static void
create_user_tree_column(GtkTreeView *user_tree) {
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes("NickName",
		renderer, "pixbuf", U_COL_HEADICON, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", U_COL_NICKNAME, NULL);
	gtk_tree_view_append_column(user_tree, column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("HostName",
		renderer, "text", U_COL_HOSTNAME, NULL);
	gtk_tree_view_append_column(user_tree, column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("UserName",
		renderer, "text", U_COL_USERNAME, NULL);
	gtk_tree_view_append_column(user_tree, column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("GroupName",
		renderer, "text", U_COL_GROUPNAME, NULL);
	gtk_tree_view_append_column(user_tree, column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Ipaddress",
		renderer, "text", U_COL_IPADDRESS, NULL);
	gtk_tree_view_append_column(user_tree, column);
}

static void
init_user_tree_data(GtkTreeView *user_tree, User **users, size_t len) {
	GtkTreeModel *model;
	GdkPixbuf *pixbuf;
	gint i;
	
	if(!user_tree || !users || !len) return;
	model = gtk_tree_view_get_model(user_tree);
	for(i = 0;i < len;i++) {
		GtkTreeIter iter;
		char *addr_str;
		
		pixbuf = gdk_pixbuf_new_from_file(users[i]->headIcon, NULL);
		addr_str = get_addr_str(users[i]->ipaddr);
		gtk_list_store_append(GTK_LIST_STORE(model), &iter);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			U_COL_HEADICON, pixbuf,
			U_COL_NICKNAME, users[i]->nickName,
			U_COL_HOSTNAME, users[i]->hostName,
			U_COL_USERNAME, users[i]->userName,
			U_COL_GROUPNAME, users[i]->groupName,
			U_COL_IPADDRESS, addr_str,
			U_COL_DATA, (gpointer)users[i],
			-1);
		FREE_WITH_CHECK(addr_str);
	}
}

static GtkTreeModel *
create_file_tree_model() {
	GtkListStore *store;

	store = gtk_list_store_new(F_COL_NUM,
		GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
		G_TYPE_STRING, G_TYPE_STRING);

	return GTK_TREE_MODEL(store);
}

static void
create_file_tree_column(GtkTreeView *file_tree) {
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes("Name",
		renderer, "pixbuf", F_COL_ICON, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "text", F_COL_NAME, NULL);
	gtk_tree_view_append_column(file_tree, column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Type",
		renderer, "text", F_COL_TYPE, NULL);
	gtk_tree_view_append_column(file_tree, column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Size",
		renderer, "text", F_COL_SIZE, NULL);
	gtk_tree_view_append_column(file_tree, column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Path",
		renderer, "text", F_COL_PATH, NULL);
	gtk_tree_view_append_column(file_tree, column);
}


static void
update_dlg_info(SendDlg *dlg) {
	GdkPixbuf *pixbuf;
	gchar *title;
	gchar *name;
	gchar *info;

	if(dlg->len == 1) {
		char *addr_str;
		User *user = dlg->users[0];
		
		addr_str = get_addr_str(user->ipaddr);
		pixbuf = gdk_pixbuf_new_from_file_at_size(user->headIcon, 30, 30, NULL);
		title = g_strdup_printf("Chating with %s(IP: %s)", user->nickName, addr_str);
		name = g_strdup_printf("<b>%s</b>", user->nickName);
		info = g_strdup_printf("<i>%s</i>(<i>%s</i>) IP: %s Group: <i>%s</i>",
			user->userName, user->hostName, addr_str, user->groupName);
		FREE_WITH_CHECK(addr_str);
	}
	else {
		pixbuf = gdk_pixbuf_new_from_file_at_size(config_get_icon_image(), 30, 30, NULL);
		title = g_strdup_printf("Multi-User Chating(Total:%d)", dlg->len);
		name = g_strdup("<b>Multi-User Chating</b>");
		info = g_strdup_printf("Total <i>%d</i> users.", dlg->len);
	}

	if(pixbuf) {
		gtk_window_set_icon(GTK_WINDOW(dlg->dialog), pixbuf);
		gtk_image_set_from_pixbuf(GTK_IMAGE(dlg->headimage), pixbuf);
		g_object_unref(pixbuf);
	}
	gtk_window_set_title(GTK_WINDOW(dlg->dialog), title);
	gtk_label_set_markup(GTK_LABEL(dlg->name_label) , name);
	gtk_label_set_markup(GTK_LABEL(dlg->info_label) , info);
	
	if(title) g_free(title);
	if(name) g_free(name);
	if(info) g_free(info);
	
}
