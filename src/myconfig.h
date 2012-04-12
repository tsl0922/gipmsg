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

#ifndef __GIPMSG_CONFIG_H__
#define __GIPMSG_CONFIG_H__

#ifdef _LINUX
#define HOME_PATH            "/usr/share/gipmsg/"
#else
#define HOME_PATH            "./"
#endif
#define LOCALE_PATH          "/usr/share/locale"

#define CONFIG_FILE_PATH     "~/.config/ichat/config.ini"
#define ICON_PATH            HOME_PATH "icons/"
#define EMOTION_PATH         HOME_PATH "emotions/"

typedef struct {
} Config;

bool config_init();
bool config_read(Config * config);
bool config_write(Config * config);

int config_get_port();

char *config_get_nickname();
char *config_get_groupname();
char *config_get_default_encode();

extern Config config;
#endif
