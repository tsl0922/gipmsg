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

#ifndef _IPMSG_TYPES_H_
#define _IPMSG_TYPES_H_

/* Message Definitions begin  */
typedef ulong command_no_t;
typedef ulong packet_no_t;

#define MAX_UDPBUF		32768
#define MAX_TCPBUF		8192
#define MAX_BUF			1024
#define MAX_NAMEBUF		80
#ifndef MAX_PATH
#define MAX_PATH        260
#endif
#ifndef MAX_FNAME
#define MAX_FNAME     	255
#endif

#define FEIQ           "lbt4_0"
#define GIPMSG_VERSION "gipmsg_0.2"

#define IPMSG_COMMON_MSG_FMT    "1_" GIPMSG_VERSION ":%ld:%s:%s:%lu:"
#define IPMSG_ALLMSG_PACKET_FMT "%s%s%c%s"
#define IPMSG_FILEATTACH_INFO_FMT   "%d:%s:%lx:%lx:%lx:%lx=%lx:%lx=%lx:%c:"
#define IPMSG_MSG_EXT_DELIM     '\0'
#define IPMSG_UNKNOWN_NAME      "unknown"

#define IPMSG_COMMAND_MASK         (0x000000ffUL)
#define IPMSG_COMMAND_OPT_MASK     (0xffffff00UL)
#define ipmsg_flags_get_command(flags) \
  ( ((command_no_t)(flags)) & (IPMSG_COMMAND_MASK) )
#define ipmsg_flags_get_opt(flags) \
	  ( ((command_no_t)(flags)) & (IPMSG_COMMAND_OPT_MASK) )

typedef struct {
	char *version;				/* version */
	command_no_t commandNo;		/* command */
	command_no_t commandOpts;	/* command option */
	char *userName;				/* user name */
	char *hostName;				/* host name */
	packet_no_t packetNo;		/* packet no */
	struct timeval tv;			/* timestamp */
	char *message;				/* message */
	char *attach;			    /* attachment(file info...) */
	ulong fromAddr;				/* from ipaddr */
} Message;

/* Message Definitions end  */

typedef enum {
	USER_STATE_ONLINE,
	USER_STATE_OFFLINE
} UserState;

typedef struct{
	ulong ipaddr;				/* ipaddress */
	char *version;				/* client version */
	char *nickName;				/* nick name */
	char *userName;				/* user name */
	char *hostName;				/* host name */
	char *groupName;			/* group name */
	char *photoImage;			/* personal photo */
	char *headIcon;				/* head icon file */
	char *personalSign;			/* personal sign */
	char *encode;				/* character encode */
	UserState state;
} User;


typedef enum {
	GroupTypeSystem,						/* system group */
	GroupTypeRegular,						/* regular group */
	GroupTypeBlack							/* black list group */
} GroupType;

typedef struct {
	char groupName[MAX_NAMEBUF];
	GroupType groupType;
	GList *users;
} Group;


typedef struct {
	ulong ipaddr;
	ulong netmask;
	ulong br_addr;
} AddrInfo;

typedef struct {
	int id;
	char path[MAX_PATH];
	char name[MAX_FNAME];
	command_no_t attr;
	packet_no_t packet_no;
	ulong offset;
	ulong size;
	ulong atime;
	ulong mtime;
	ulong ctime;
}FileInfo;

#endif /*_IPMSG_TYPES_H_*/

