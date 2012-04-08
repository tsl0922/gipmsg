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

command_no_t config_get_normal_send_flags()
{
	return IPMSG_SENDCHECKOPT;
}

command_no_t config_get_normal_entry_flags()
{
	return 0;
}
