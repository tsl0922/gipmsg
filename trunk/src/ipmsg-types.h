#ifndef _IPMSG_TYPES_H_
#define _IPMSG_TYPES_H_

/* Message Definitions begin  */
typedef ulong command_no_t;
typedef long packet_no_t;

#define MAX_UDPBUF		32768
#define MAX_BUF			1024
#define MAX_NAMEBUF		80
#define MAX_FILENAME	260

#define FEIQ           "lbt4_0"
#define GIPMSG_VERSION "gipmsg_0.2"

/* 
 * ipmsg packet format:
 *
 *	Version(1):PacketNo:SenderName:SenderHost:CommandNo:AdditionalSection
 *
 */
#define IPMSG_COMMON_MSG_FMT    "1_" GIPMSG_VERSION ":%ld:%s:%s:%lu:"
#define IPMSG_ALLMSG_PACKET_FMT "%s%s%c%s"
/**
 *
 * An IPMSG_SENDMSG command with an IPMSG_FILEATTACHOPT flag for File transfer
 * 
 * attachment info format:(AdditionalSection)
 *
 *    fileID:filename:size:mtime:fileattr[:extend-attr=val1 [,val2...][:extend-attr2=...]]:\a:fileID...
 *
 *	(size, mtime, and fileattr describe hex format. If a filename contains ':', please replace with "::".)
 */
#define IPMSG_FILEATTACH_INFO_FMT   "%d:%s:%lx:%lx:%lx:%lx=%lx:%lx=%lx:%c:"
#define IPMSG_MSG_EXT_DELIM     '\0'
#define IPMSG_UNKNOWN_NAME      "unknown"

#define IPMSG_COMMAND_MASK         (0xffUL)
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
	char path[MAX_FILENAME];
	char name[MAX_FILENAME];
	command_no_t attr;
	ulong size;
	ulong atime;
	ulong mtime;
	ulong ctime;
}FileInfo;


#endif /*_IPMSG_TYPES_H_*/

