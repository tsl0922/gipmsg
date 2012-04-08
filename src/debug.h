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
#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DEBUG_FFL __FILE__, __LINE__, __FUNCTION__

struct tm *get_currenttime();
void debug_info(const char *file, int line, const char *function,
		const char *format, ...);
void debug_error(const char *file, int line, const char *function,
		 const char *format, ...);

#ifdef DEBUG
#define DEBUG_INFO(format, args...) \
	debug_info(DEBUG_FFL, format, ##args)
#define DEBUG_ERROR(format, args...) \
	debug_error(DEBUG_FFL, format, ##args)
#else
#define DEBUG_INFO(format, args...)
#define DEBUG_ERROR(format, args...)
#endif

#endif
