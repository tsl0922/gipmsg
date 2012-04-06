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

#define GROUP_FRIEND "My Friends"
#define GROUP_MYSELF "Myself"

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

MainWindow main_win;
static gint user_count = 0;

static void init_users_tree();
static void users_tree_add_group(GtkTreeModel * model,
				 GtkTreeIter * iter, gchar * groupName);
static bool users_tree_get_group_iter(GtkTreeModel * model,
				      GtkTreeIter * iter, gchar * groupName);
static void users_tree_add_user0(GtkTreeModel * model,
				 GtkTreeIter * parent, User * user);
static bool users_tree_find_user_in_group(GtkTreeModel * model,
					  GtkTreeIter * parent,
					  GtkTreeIter * iter, ulong ipaddr);
static GList *users_tree_get_child_users(GtkTreeModel * model,
					 GtkTreeIter * parent);
static GList *users_tree_get_selected_users();
static bool is_groupName_valid(gchar * groupName);

static gboolean
on_window_delete(GtkWidget * widget, GdkEvent * event, gpointer data)
{
	gtk_widget_hide(widget);
	return TRUE;
}

static void dialog_args_init(DialogArgs * args, User ** users, size_t len)
{
	if (!args || !users || len < 1)
		return;
	memset(args, 0, sizeof(DialogArgs));

	args->main_win = &main_win;
	args->users = users;
	args->len = len;
}

static void ActiveDlg(SendDlg * dlg, bool active)
{
	if (active) {
		//gtk_window_deiconify(GTK_WINDOW(dlg->dialog));
		gtk_window_present(GTK_WINDOW(dlg->dialog));
	} else {
		gtk_window_iconify(GTK_WINDOW(dlg->dialog));
	}
}

SendDlg *SendDlgOpen(User * user, bool * is_new)
{
	GList *entry;
	SendDlg *dlg;

	if (user == NULL)
		return false;
	entry = g_list_first(main_win.sendDlgList);
	while (entry) {
		dlg = (SendDlg *) entry->data;
		if (dlg->user->ipaddr == user->ipaddr) {
			*is_new = false;
			return dlg;
		}
		entry = entry->next;
	};

	dlg = senddlg_new(user);
	main_win.sendDlgList = g_list_append(main_win.sendDlgList, dlg);
	*is_new = true;

	return dlg;
}

static void on_statusicon_activate(GtkStatusIcon * statusicon, gpointer data)
{
	if (gtk_status_icon_get_blinking(statusicon)) {
		gtk_status_icon_set_blinking(statusicon, FALSE);
		gtk_status_icon_set_tooltip(statusicon, GIPMSG_VERSION);
		gtk_status_icon_set_from_file(statusicon, ICON_PATH "icon.png");
		ActiveDlg(main_win.dlg, true);
		return;
	}
	if (gtk_widget_get_visible(main_win.window)) {
		gtk_widget_hide(main_win.window);
	} else {
		gtk_window_present(GTK_WINDOW(main_win.window));
	}
}

typedef void *(*MenuCallBackFunc) (GtkWidget * item, gpointer data);

static GtkWidget *create_menu_item(const char *name, const char *iconpath,
				   GtkWidget * parent, gboolean sensitive,
				   MenuCallBackFunc func, gpointer data)
{
	GtkWidget *item = gtk_image_menu_item_new_with_label(name);
	if (iconpath != NULL) {
		GdkPixbuf *pb =
		    gdk_pixbuf_new_from_file_at_size(iconpath, 16, 16, NULL);
		GtkWidget *img = gtk_image_new_from_pixbuf(pb);
		g_object_unref(pb);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), img);
	}
	gtk_menu_shell_append(GTK_MENU_SHELL(parent), item);
	if (sensitive == FALSE) {
		gtk_widget_set_sensitive(item, FALSE);
		return item;
	}
	if (func != NULL)
		g_signal_connect(item, "activate", G_CALLBACK(func), data);
	return item;
}

static void on_menu_exit(GtkMenuItem * menu_item, gpointer data)
{
	gtk_main_quit();
}

static void
statusicon_popup_menu(GtkStatusIcon * statusicon, guint button,
		      guint activate_time, gpointer data)
{
	GtkWidget *menu;

	menu = gtk_menu_new();
	create_menu_item("Exit", NULL, menu, TRUE,
			 (MenuCallBackFunc) on_menu_exit, NULL);
	gtk_widget_show_all(menu);

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
		       gtk_status_icon_position_menu,
		       statusicon, button, activate_time);
}

static gboolean
users_tree_select_func(GtkTreeSelection * selection,
		       GtkTreeModel * model,
		       GtkTreePath * path,
		       gboolean path_currently_selected, gpointer data)
{
	GtkTreeIter iter;
	gint type;

	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, TYPE_COLUMN, &type, -1);
	if (type == COL_TYPE_GROUP)	//group can not be selected
		return false;
	else
		return true;
}

static void on_menu_send_message(GtkMenuItem * menu_item, gpointer data)
{

}

static void on_menu_refresh(GtkMenuItem * menu_item, gpointer data)
{
	clear_users_tree();
	clear_user_list();
	users_tree_create_default_group();
	ipmsg_send_br_entry();
}

static gboolean UserstreePopupMenu(GtkWidget * treeview, GdkEventButton * event)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkWidget *menu;
	gint type;
	GList *users = NULL;

	if (event->button != 3
	    || !gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
					      (gint) (event->x),
					      (gint) (event->y), &path, NULL,
					      NULL, NULL))
		return FALSE;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(main_win.users_treeview));
	gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(main_win.users_treeview),
				      (gint) event->x, (gint) event->y, &path,
				      NULL, NULL, NULL);

	if (!path)
		return FALSE;
	if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path))
		return FALSE;
	gtk_tree_model_get(model, &iter, TYPE_COLUMN, &type, -1);
	DEBUG_INFO("type: %d", type);

	switch (type) {
	case COL_TYPE_USER:
		{
			if (gtk_tree_selection_path_is_selected
			    (gtk_tree_view_get_selection
			     (GTK_TREE_VIEW(treeview)), path)) {
				users = users_tree_get_selected_users();
			} else {
				User *tuser;
				gtk_tree_model_get(model, &iter,
						   DATA_COLUMN, &tuser, -1);
				users = g_list_append(users, tuser);
			}
			break;
		}
	case COL_TYPE_GROUP:
		{
			users = users_tree_get_child_users(model, &iter);
			break;
		}
	}
	gtk_tree_path_free(path);

	menu = gtk_menu_new();
	create_menu_item("Send Message", NULL, menu, TRUE,
			 (MenuCallBackFunc) on_menu_send_message, users);
	create_menu_item("Refresh", NULL, menu, TRUE,
			 (MenuCallBackFunc) on_menu_refresh, NULL);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		       event->button, event->time);

	return TRUE;
}

static gboolean
UserstreeChangeStatus(GtkWidget * treeview, GdkEventButton * event)
{
	GtkTreeModel *model;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GtkTreePath *path;
	GtkTreeIter iter;
	gint cellx, startpos, width;
	gint type;

	if (event->button != 1
	    || !gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
					      (gint) event->x, (gint) event->y,
					      &path, &column, &cellx, NULL))
		return FALSE;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, TYPE_COLUMN, &type, -1);
	if (type != COL_TYPE_GROUP) {
		gtk_tree_path_free(path);
		return FALSE;
	}

	cell =
	    GTK_CELL_RENDERER(g_object_get_data
			      (G_OBJECT(column), "expander-cell"));
	gtk_tree_view_column_cell_get_position(column, cell, &startpos, &width);
	if ((cellx < startpos) || (cellx > startpos + width)) {
		gtk_tree_path_free(path);
		return FALSE;
	}

	if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(treeview), path))
		gtk_tree_view_collapse_row(GTK_TREE_VIEW(treeview), path);
	else
		gtk_tree_view_expand_row(GTK_TREE_VIEW(treeview), path, FALSE);
	gtk_tree_path_free(path);

	return TRUE;

}

static void
UserstreeItemActivited(GtkTreeView * treeview,
		       GtkTreePath * path,
		       GtkTreeViewColumn * column, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gint type;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	if (!gtk_tree_model_get_iter(model, &iter, path))
		return;
	gtk_tree_model_get(model, &iter, TYPE_COLUMN, &type, -1);

	switch (type) {
	case COL_TYPE_USER:
		{
			User *tuser;
			bool is_new;

			gtk_tree_model_get(model, &iter,
					   DATA_COLUMN, &tuser, -1);
			SendDlg *dlg = SendDlgOpen(tuser, &is_new);
			if (is_new)
				ActiveDlg(dlg, true);
			break;
		}
	case COL_TYPE_GROUP:
		{
			if (gtk_tree_view_row_expanded
			    (GTK_TREE_VIEW(treeview), path))
				gtk_tree_view_collapse_row(GTK_TREE_VIEW
							   (treeview), path);
			else
				gtk_tree_view_expand_row(GTK_TREE_VIEW
							 (treeview), path,
							 FALSE);
			break;
		}
	}
}

static void create_all_widget()
{
	GdkPixbuf *icon = gdk_pixbuf_new_from_file(ICON_PATH "icon.png", NULL);

	main_win.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(main_win.window), "Gipmsg");
	gtk_window_stick(GTK_WINDOW(main_win.window));	//let the window appear on all user desktops
	gtk_window_set_icon(GTK_WINDOW(main_win.window), icon);
	gtk_window_set_default_size(GTK_WINDOW(main_win.window), 250, 500);
	g_signal_connect(main_win.window, "delete-event",
			 G_CALLBACK(on_window_delete), NULL);

	main_win.trayIcon = gtk_status_icon_new_from_pixbuf(icon);
	gtk_status_icon_set_tooltip(main_win.trayIcon, GIPMSG_VERSION);
	gtk_status_icon_set_visible(main_win.trayIcon, TRUE);
	gtk_widget_set_app_paintable(main_win.window, TRUE);
	g_signal_connect(main_win.trayIcon, "activate",
			 G_CALLBACK(on_statusicon_activate), NULL);
	g_signal_connect(main_win.trayIcon, "popup-menu",
			 G_CALLBACK(statusicon_popup_menu), NULL);

	main_win.table = gtk_table_new(3, 1, FALSE);
	main_win.eventbox = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(main_win.eventbox),
					 TRUE);
	gtk_widget_set_has_window(main_win.eventbox, TRUE);
	gtk_container_add(GTK_CONTAINER(main_win.eventbox), main_win.table);
	gtk_container_add(GTK_CONTAINER(main_win.window), main_win.eventbox);

	main_win.info_label = gtk_label_new("Ichat(online:0)");
	gtk_table_attach(GTK_TABLE(main_win.table), main_win.info_label, 0, 1,
			 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);

	main_win.users_scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW
				       (main_win.users_scrolledwindow),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW
					    (main_win.users_scrolledwindow),
					    GTK_SHADOW_IN);
	main_win.users_treeview = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(main_win.users_scrolledwindow),
			  main_win.users_treeview);
	//gtk_tree_view_set_reorderable(GTK_TREE_VIEW(main_win.users_treeview), TRUE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW
					  (main_win.users_treeview), FALSE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection
				    (GTK_TREE_VIEW(main_win.users_treeview)),
				    GTK_SELECTION_MULTIPLE);
	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection
					       (GTK_TREE_VIEW
						(main_win.users_treeview)),
					       (GtkTreeSelectionFunc)
					       users_tree_select_func, NULL,
					       NULL);
	gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW
					    (main_win.users_treeview), FALSE);
	gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(main_win.users_treeview),
					 FALSE);

	g_signal_connect(main_win.users_treeview, "row-activated",
			 G_CALLBACK(UserstreeItemActivited), NULL);
	g_signal_connect(main_win.users_treeview, "button-press-event",
			 G_CALLBACK(UserstreePopupMenu), NULL);
	g_signal_connect(main_win.users_treeview, "button-release-event",
			 G_CALLBACK(UserstreeChangeStatus), NULL);

	gtk_table_attach(GTK_TABLE(main_win.table),
			 main_win.users_scrolledwindow, 0, 1, 1, 2,
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

	GdkScreen *screen;
	GdkColormap *colormap;

	screen = gtk_widget_get_screen(main_win.window);
	colormap = gdk_screen_get_rgba_colormap(screen);
	gtk_widget_set_colormap(main_win.window, colormap);

	gtk_widget_show_all(main_win.window);
}

void ipmsg_ui_init()
{
	DEBUG_INFO("Initialize ui...");

	create_all_widget();
	init_users_tree();
	users_tree_create_default_group();
}

static void init_users_tree()
{
	GtkTreeStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	store = gtk_tree_store_new(N_COLUMNS, GDK_TYPE_PIXBUF,
				   GDK_TYPE_PIXBUF, G_TYPE_STRING,
				   G_TYPE_POINTER, G_TYPE_INT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(main_win.users_treeview),
				GTK_TREE_MODEL(store));
	g_object_unref(store);

	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column(GTK_TREE_VIEW(main_win.users_treeview),
				    column);
	//expander
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer,
					    "pixbuf", EXPANDER_CLOSED,
					    "pixbuf-expander-closed",
					    EXPANDER_CLOSED,
					    "pixbuf-expander-open",
					    EXPANDER_OPEN, NULL);
	g_object_set_data(G_OBJECT(column), "expander-cell", renderer);
	//info
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer, "text",
					    INFO_COLUMN, NULL);
}

void users_tree_create_default_group()
{
	GtkTreeModel *model;
	GtkTreeIter iter1, iter2;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(main_win.users_treeview));
	users_tree_add_group(model, &iter1, GROUP_MYSELF);
	users_tree_add_group(model, &iter2, GROUP_FRIEND);
}

static void
users_tree_add_group(GtkTreeModel * model,
		     GtkTreeIter * iter, gchar * groupName)
{
	GdkPixbuf *cpixbuf, *opixbuf;

	DEBUG_INFO("adding group...");
	cpixbuf =
	    gdk_pixbuf_new_from_file(ICON_PATH "expender-closed.png", NULL);
	opixbuf = gdk_pixbuf_new_from_file(ICON_PATH "expender-open.png", NULL);
	gtk_tree_store_append(GTK_TREE_STORE(model), iter, NULL);
	gtk_tree_store_set(GTK_TREE_STORE(model), iter,
			   EXPANDER_CLOSED, cpixbuf,
			   EXPANDER_OPEN, opixbuf,
			   INFO_COLUMN, groupName,
			   DATA_COLUMN, (gpointer) NULL,
			   TYPE_COLUMN, COL_TYPE_GROUP, -1);
	if (cpixbuf)
		g_object_unref(G_OBJECT(cpixbuf));
	if (opixbuf)
		g_object_unref(G_OBJECT(opixbuf));
}

static bool
users_tree_get_group_iter(GtkTreeModel * model,
			  GtkTreeIter * iter, gchar * groupName)
{
	gint type;
	gchar *info;

	DEBUG_INFO("groupName: %s", groupName);
	if (gtk_tree_model_get_iter_first(model, iter)) {
		do {
			gtk_tree_model_get(model, iter, TYPE_COLUMN, &type, -1);
			if (type == COL_TYPE_GROUP) {
				gtk_tree_model_get(model, iter, INFO_COLUMN,
						   &info, -1);
				if (!strcmp(info, groupName))
					return true;
			}
		} while (gtk_tree_model_iter_next(model, iter));
	}

	return false;
}

static void
users_tree_add_user0(GtkTreeModel * model, GtkTreeIter * parent, User * user)
{
	GtkTreeIter iter;
	GdkPixbuf *cpixbuf, *opixbuf;
	gchar *info;
	char *addr_str;

	cpixbuf = gdk_pixbuf_new_from_file(user->headIcon, NULL);
	opixbuf = GDK_PIXBUF(g_object_ref(G_OBJECT(cpixbuf)));
	addr_str = get_addr_str(user->ipaddr);
	info =
	    g_strdup_printf("%s(%s)\n%s", user->nickName, user->hostName,
			    addr_str);
	FREE_WITH_CHECK(addr_str);
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(main_win.users_treeview));
	gtk_tree_store_append(GTK_TREE_STORE(model), &iter, parent);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
			   EXPANDER_CLOSED, cpixbuf,
			   EXPANDER_OPEN, opixbuf,
			   INFO_COLUMN, info,
			   DATA_COLUMN, (gpointer) user,
			   TYPE_COLUMN, COL_TYPE_USER, -1);
	if (cpixbuf)
		g_object_unref(G_OBJECT(cpixbuf));
	if (opixbuf)
		g_object_unref(G_OBJECT(opixbuf));
	if (info)
		g_free(info);
}

void users_tree_add_user(User * user)
{
	GtkTreeModel *model;
	GtkTreeIter iter;	//parent iter

	DEBUG_INFO("---------");
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(main_win.users_treeview));
	if (is_sys_host_addr(user->ipaddr)) {
		GtkTreeIter iter1;
		users_tree_get_group_iter(model, &iter1, GROUP_MYSELF);
		users_tree_add_user0(model, &iter1, user);
	} else {
		if (is_groupName_valid(user->groupName)) {
			//add user to it's group
			GtkTreeIter iter2;
			if (!users_tree_get_group_iter
			    (model, &iter2, user->groupName)) {
				users_tree_add_group(model, &iter2,
						     user->groupName);
				users_tree_add_user0(model, &iter2, user);
			}
		}
	}
	users_tree_get_group_iter(model, &iter, GROUP_FRIEND);
	users_tree_add_user0(model, &iter, user);
	++user_count;
}

void users_tree_del_user(ulong ipaddr)
{
	GtkTreeModel *model;
	GtkTreeIter iter, pos;
	gint type;
	User *tuser;
	gchar *groupName = NULL;

	DEBUG_INFO("---------");
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(main_win.users_treeview));
	if (gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			if (gtk_tree_model_iter_children(model, &pos, &iter)) {
				do {
					gtk_tree_model_get(model, &pos,
							   TYPE_COLUMN, &type,
							   -1);
					if (type == COL_TYPE_USER) {
						gtk_tree_model_get(model, &pos,
								   DATA_COLUMN,
								   &tuser, -1);
						if (tuser->ipaddr == ipaddr) {
							gtk_tree_store_remove
							    (GTK_TREE_STORE
							     (model), &pos);
							groupName =
							    g_strdup(tuser->
								     groupName);
							break;
						}
					}
				} while (gtk_tree_model_iter_next(model, &pos));
			}
		} while (gtk_tree_model_iter_next(model, &iter));
	}
	//remove empty group
	if (is_groupName_valid(groupName)) {
		if (users_tree_get_group_iter(model, &iter, groupName)) {
			if (!gtk_tree_model_iter_has_child(model, &iter)) {
				gtk_tree_store_remove(GTK_TREE_STORE(model),
						      &iter);
			}
		}
		g_free(groupName);
	}
	--user_count;
}

static void
users_tree_update_user0(GtkTreeModel * model, GtkTreeIter * iter, User * user)
{
	GdkPixbuf *cpixbuf, *opixbuf;
	gchar *info;
	char *addr_str;

	DEBUG_INFO("updating...");
	cpixbuf = gdk_pixbuf_new_from_file(user->headIcon, NULL);
	opixbuf = GDK_PIXBUF(g_object_ref(G_OBJECT(cpixbuf)));
	addr_str = get_addr_str(user->ipaddr);
	info =
	    g_strdup_printf("%s(%s)\n%s", user->nickName, user->hostName,
			    addr_str);
	FREE_WITH_CHECK(addr_str);
	gtk_tree_store_set(GTK_TREE_STORE(model), iter,
			   EXPANDER_CLOSED, cpixbuf,
			   EXPANDER_OPEN, opixbuf,
			   INFO_COLUMN, info,
			   DATA_COLUMN, (gpointer) user,
			   TYPE_COLUMN, COL_TYPE_USER, -1);
	if (cpixbuf)
		g_object_unref(G_OBJECT(cpixbuf));
	if (opixbuf)
		g_object_unref(G_OBJECT(opixbuf));
	if (info)
		g_free(info);
}

void users_tree_update_user(User * user)
{
	GtkTreeModel *model;
	GtkTreeIter iter, pos;
	gint type;
	User *tuser;
	gchar *groupName = NULL;

	DEBUG_INFO("---------");
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(main_win.users_treeview));
	if (gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			if (gtk_tree_model_iter_children(model, &pos, &iter)) {
				do {
					gtk_tree_model_get(model, &pos,
							   TYPE_COLUMN, &type,
							   -1);
					if (type == COL_TYPE_USER) {
						gtk_tree_model_get(model, &pos,
								   DATA_COLUMN,
								   &tuser, -1);
						if (tuser->ipaddr ==
						    user->ipaddr) {
							users_tree_update_user0
							    (model, &pos, user);
							groupName =
							    g_strdup(tuser->
								     groupName);
							break;
						}
					}
				} while (gtk_tree_model_iter_next(model, &pos));
			}
		} while (gtk_tree_model_iter_next(model, &iter));
	}
	//update group
	DEBUG_INFO("old groupName: %s", groupName);
	//check old groupName
	if (is_groupName_valid(groupName)) {
		if (users_tree_get_group_iter(model, &iter, groupName)) {
			if (strcmp(groupName, user->groupName)) {
				if (users_tree_find_user_in_group
				    (model, &iter, &pos, user->ipaddr)) {
					gtk_tree_store_remove(GTK_TREE_STORE
							      (model), &pos);
				}
			}
			if (!gtk_tree_model_iter_has_child(model, &iter)) {
				gtk_tree_store_remove(GTK_TREE_STORE(model),
						      &iter);
			}
		}
		g_free(groupName);
	}
	DEBUG_INFO("new groupName: %s", user->groupName);
	//check new groupName
	if (is_groupName_valid(user->groupName)
	    && !is_sys_host_addr(user->ipaddr)) {
		if (!users_tree_get_group_iter(model, &iter, user->groupName)) {
			users_tree_add_group(model, &iter, user->groupName);
			users_tree_add_user0(model, &iter, user);
		} else {
			if (users_tree_find_user_in_group
			    (model, &iter, &pos, user->ipaddr)) {
				users_tree_update_user0(model, &pos, user);
			} else {
				users_tree_add_user0(model, &pos, user);
			}
		}
	}
}

static bool
users_tree_find_user_in_group(GtkTreeModel * model,
			      GtkTreeIter * parent,
			      GtkTreeIter * iter, ulong ipaddr)
{
	gint type;
	User *tuser;

	if (gtk_tree_model_iter_children(model, iter, parent)) {
		do {
			gtk_tree_model_get(model, iter, TYPE_COLUMN, &type, -1);
			if (type == COL_TYPE_USER) {
				gtk_tree_model_get(model, iter, DATA_COLUMN,
						   &tuser, -1);
				if (tuser->ipaddr == ipaddr) {
					return true;
				}
			}
		} while (gtk_tree_model_iter_next(model, iter));
	}

	return false;
}

static GList *users_tree_get_child_users(GtkTreeModel * model,
					 GtkTreeIter * parent)
{
	GtkTreeIter iter;
	gint type;
	GList *users = NULL;
	User *user = NULL;

	if (gtk_tree_model_iter_children(model, &iter, parent)) {
		do {
			gtk_tree_model_get(model, &iter, TYPE_COLUMN, &type,
					   -1);
			if (type == COL_TYPE_USER) {
				gtk_tree_model_get(model, &iter, DATA_COLUMN,
						   &user, -1);
				users = g_list_append(users, user);
			}
		} while (gtk_tree_model_iter_next(model, &iter));
	}

	return users;
}

static GList *users_tree_get_selected_users()
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	GtkTreeIter iter;
	GList *slist;
	GList *users = NULL;
	User *user = NULL;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(main_win.users_treeview));
	selection =
	    gtk_tree_view_get_selection(GTK_TREE_VIEW(main_win.users_treeview));
	slist = gtk_tree_selection_get_selected_rows(selection, NULL);
	while (slist) {
		path = (GtkTreePath *) slist->data;
		if (gtk_tree_model_get_iter(model, &iter, path)) {
			gtk_tree_model_get(model, &iter, DATA_COLUMN, &user,
					   -1);
			users = g_list_append(users, user);
		}
		slist = slist->next;
	};

	g_list_foreach(slist, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(slist);

	return users;
}

void clear_users_tree()
{
	gtk_tree_store_clear(GTK_TREE_STORE
			     (gtk_tree_view_get_model
			      (GTK_TREE_VIEW(main_win.users_treeview))));
}

static bool is_groupName_valid(gchar * groupName)
{
	if (groupName && strlen(groupName)
	    && strcmp(groupName, IPMSG_UNKNOWN_NAME))
		return true;
	return false;
}

void notify_recvmsg(ulong ipaddr, packet_no_t packet_no)
{
	GList *entry = main_win.sendDlgList;

	while (entry) {
		SendDlg *dlg = (SendDlg *) entry->data;
		if (dlg->user->ipaddr == ipaddr) {
			notify_send_finish(dlg, ipaddr, packet_no);
			break;
		}
		entry = entry->next;
	};
}

void modify_status_icon(char *tip, char *iconfile, bool blinking)
{
	gtk_status_icon_set_tooltip_markup(main_win.trayIcon, tip);
	if (iconfile)
		gtk_status_icon_set_from_file(main_win.trayIcon, iconfile);
	gtk_status_icon_set_blinking(main_win.trayIcon, blinking);
}

void notify_sendmsg(Message * msg)
{
	bool is_new;
	bool isFileAttach = msg->commandOpts & IPMSG_FILEATTACHOPT;

	main_win.user = find_user(msg->fromAddr);
	if (!main_win.user)
		return;
	main_win.dlg = SendDlgOpen(main_win.user, &is_new);
	if (!is_new && is_same_packet(main_win.dlg, msg))
		return;
	main_win.dlg->msg = dup_message(msg);
	if (isFileAttach) {
		GSList *files = parse_file_info(msg->attach, msg->packetNo);
		senddlg_add_fileattach(main_win.dlg, files);
		g_slist_free(files);
	} else
		senddlg_add_message(main_win.dlg, msg->message, false);
	if (is_new) {
		char buf[MAX_BUF];
		char time[MAX_NAMEBUF];

		gtk_widget_hide(main_win.dlg->dialog);
		strftime(time, sizeof(time), "%F %T", get_currenttime());
		sprintf(buf, "<b>New Message\n</b>%s(%s %s@%s) %s",
			main_win.user->nickName, main_win.user->groupName,
			main_win.user->userName, main_win.user->hostName, time);
		modify_status_icon(buf, main_win.user->headIcon, TRUE);
	}
}
