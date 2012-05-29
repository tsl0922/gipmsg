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

bool config_init()
{
}

bool config_read(Config * config)
{
	GKeyFile *keyfile;
	GError *error = NULL;
	const gchar *file = CONFIG_FILE_PATH;

	keyfile = g_key_file_new();
	if (!g_key_file_load_from_file(keyfile, file, G_KEY_FILE_NONE, &error)) {
		g_key_file_free(keyfile);
		return false;
	}

	return true;
}

bool config_write(Config * config)
{
	return false;
}

int config_get_port()
{
	return IPMSG_DEFAULT_PORT;
}

char *config_get_nickname()
{
	return "lucky";
}

char *config_get_groupname()
{
	return "gipmsg";
}

char *config_get_default_encode()
{
	return "gb18030";
}

char *config_get_candidate_encode()
{
	return "gb18030,big5";
}

static GtkWidget *create_system_page() 
{
	return gtk_label_new("system");
}

static GtkWidget *create_personal_page() 
{
	return gtk_label_new("personal");
}

GtkWidget *create_prefs_dialog(GtkWindow *parent)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *notebook;

	dialog = gtk_dialog_new_with_buttons("Preference",
			parent, 
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK, GTK_RESPONSE_OK, 
			GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			NULL);
	gtk_widget_set_sensitive(gtk_dialog_get_widget_for_response(
		GTK_DIALOG(dialog), GTK_RESPONSE_OK), FALSE);
	gtk_widget_set_sensitive(gtk_dialog_get_widget_for_response(
		GTK_DIALOG(dialog), GTK_RESPONSE_APPLY), FALSE);
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
			create_system_page(),
			gtk_label_new("System"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
			create_personal_page(),
			gtk_label_new("Personal"));

	gtk_widget_show_all(notebook);
	
	return dialog;
}

void show_prefs_dialog(GtkWindow *parent)
{
	static GtkWidget *dialog = NULL;

	if(!dialog) {
		dialog = create_prefs_dialog(parent);
	}
	else {
		gtk_window_present(GTK_WINDOW(dialog));
		return;
	}
	gint result = gtk_dialog_run(GTK_DIALOG(dialog));
	switch(result)
	{
	case GTK_RESPONSE_OK:
		break;
	case GTK_RESPONSE_APPLY:
		break;
	case GTK_RESPONSE_CANCEL:
	default:
		break;
	}
	gtk_widget_destroy(dialog);
	dialog = NULL;
}
