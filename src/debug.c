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

#include <common.h>

void
debug_info(const char *file, int line, const char *function, const char *format,
	   ...)
{
	char t_str[32] = { 0 };
	char fmt[4096] = { 0 };
	va_list ap;
	struct tm *t = get_currenttime();
	strftime(t_str, sizeof(t_str), "%T", t);
	sprintf(fmt,
		"[\e[32m\e[1m%s file:%s line: %d function:%s INFO\e[0m]\n%s\n",
		t_str, file, line, function, format);
	va_start(ap, format);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}

void
debug_error(const char *file, int line, const char *function,
	    const char *format, ...)
{
	char t_str[32] = { 0 };
	char fmt[4096] = { 0 };
	va_list ap;
	struct tm *t = get_currenttime();
	strftime(t_str, sizeof(t_str), "%T", t);
	sprintf(fmt,
		"[\e[31m\e[1m%s file:%s line: %d function:%s ERROR\e[0m]\n%s\nerrno: %d(%s)\n",
		t_str, file, line, function, format, errno, strerror(errno));
	va_start(ap, format);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}
