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

static GSList *file_list = NULL;
static int file_id = 0;
static GStaticMutex mutex = G_STATIC_MUTEX_INIT;

typedef struct {
	FileInfo *info;
	char path[MAX_PATH];
	User *user;
} RecvThreadArgs;

typedef struct {
	SOCKET client_sock;
	ulong ipaddr;
} TcpRequestArgs;

typedef struct {
	ulong packet_no;
	int file_id;
	ulong offset;
} RequestInfo;

typedef struct {
	int head_size;
	char name[MAX_FNAME];
	ulong size;
	command_no_t attr;
} DirHeader;

static gpointer recv_file_thread(gpointer data);
static gpointer send_file_thread(gpointer data);

bool
setup_file_info(FileInfo *info, const char *path)
{
	struct stat statbuf;
	char *sp;

	DEBUG_INFO("setup file info: %s", path);
	if(stat(path, &statbuf) == -1) return false;
	info->id = ++file_id;
	info->offset = 0;
	strcpy(info->path, path);
	sp = strrchr(path, '/');
	if(sp)
		strcpy(info->name, sp + 1);
	else
		strcpy(info->name, path);
	info->mtime = statbuf.st_mtime;
	info->atime = statbuf.st_atime;
	info->ctime = statbuf.st_ctime;
	info->size = S_ISDIR(statbuf.st_mode)?0:statbuf.st_size;
	
	info->attr = S_ISDIR(statbuf.st_mode)?IPMSG_FILE_DIR:IPMSG_FILE_REGULAR;

	return true;
}

void
add_file(FileInfo *info) {
	g_static_mutex_lock(&mutex);
	file_list = g_slist_append(file_list, info);
	g_static_mutex_unlock(&mutex);
}

bool
del_file(const char *path)
{
	GSList *entry = file_list;
	FileInfo *info;
	bool ret = false;

	while(entry) {
		info = (FileInfo *)entry->data;
		if(!strcmp(info->path, path)) {
			g_static_mutex_lock(&mutex);
			FREE_WITH_CHECK(info);
			file_list = g_slist_delete_link(file_list, entry);
			g_static_mutex_unlock(&mutex);
			ret = true;
			break;
		}
		entry = entry->next;
	}
	
	return ret;
}

bool
validate_request(RequestInfo *request, FileInfo **info_p)
{
	GSList *entry = file_list;
	FileInfo *file;
	bool ret = false;

	while(entry) {
		file = (FileInfo *)entry->data;
		if(file->id = request->file_id && file->packet_no == request->packet_no) {
			*info_p = file;
			ret = true;
			break;
		}
		entry = entry->next;
	}

	return ret;
}

/*
  * format:
  *
  * fileID:filename:size:mtime:fileattr[:extend-attr=val1 [,val2...][:extend-attr2=...]]:\a:fileID...
  * (size, mtime, and fileattr describe hex format. If a filename contains ':', please replace with "::".)
  *
  */
void
send_file_info(ulong toAddr, GSList *files)
{
	GSList *entry = files;
	FileInfo *info;
	char packet[MAX_UDPBUF];
	packet_no_t packet_no;
	size_t packet_len;
	char attach[MAX_UDPBUF];
	char *ptr;
	size_t len = 0;

	ptr = attach;
	while(entry) {
		info = (FileInfo *)entry->data;
		snprintf(ptr, MAX_UDPBUF - len, IPMSG_FILEATTACH_INFO_FMT, info->id,
			info->name, info->size, info->mtime, info->attr,
			IPMSG_FILE_ATIME, info->atime,
			IPMSG_FILE_CTIME, info->ctime,
			FILELIST_SEPARATOR);

		len += strlen(ptr);
		ptr = attach + len;
		entry = entry->next;
	};
	build_packet(packet, IPMSG_SENDMSG | IPMSG_FILEATTACHOPT | IPMSG_SENDCHECKOPT, NULL,
		attach, &packet_len, &packet_no);
	ipmsg_send_packet(toAddr, packet, packet_len);

	//set packet_no
	entry = files;
	while(entry) {
		info = (FileInfo *)entry->data;
		info->packet_no = packet_no;
		entry = entry->next;
	};
	
	g_slist_free(files);
}

/**
  * see IPMSG_FILEATTACH_INFO_FMT
  * example:
  *               1:AUTHORS:0:4f793031:101:15=4f793031:13=4f793031:\a:
  */
GSList *
parse_file_info(char *attach, packet_no_t packet_no) {
	GSList *files = NULL;
	FileInfo *file = NULL;
	char *tok = NULL;
	char *ptr = NULL;

	if(!attach) return NULL;

	DEBUG_INFO("parsing file info: %s", attach);
process_file:
	file = (FileInfo *)malloc(sizeof(FileInfo));
	memset(file, 0, sizeof(FileInfo));
	file->packet_no = packet_no;
	//fileileID
	tok = seprate_token(attach, ':', &ptr);
	file->id = strtol(tok, (char **) NULL, 16);
	if(!ptr) {
		FREE_WITH_CHECK(file);
		goto err_out;
	}
	//filename
	tok = seprate_token(attach, ':', &ptr);
	strcpy(file->name, tok);
	if(!ptr) {
		FREE_WITH_CHECK(file);
		goto err_out;
	}
	//size
	tok = seprate_token(attach, ':', &ptr);
	file->size = strtoll(tok, (char **)NULL, 16);
	if(!ptr) {
		FREE_WITH_CHECK(file);
		goto err_out;
	}
	//mtime
	tok = seprate_token(attach, ':', &ptr);
	file->mtime= strtoll(tok, (char **)NULL, 16);
	if(!ptr) {
		FREE_WITH_CHECK(file);
		goto err_out;
	}
	//fileattr
	tok = seprate_token(attach, ':', &ptr);
	file->attr = strtoll(tok, (char **)NULL, 16);
	if(!ptr) {
		FREE_WITH_CHECK(file);
		goto err_out;
	}
	
	files = g_slist_append(files, file);
	//ignore extend attr
	tok = seprate_token(attach, FILELIST_SEPARATOR, &ptr);
	if(ptr && *ptr != '\0') {
		if(*ptr == ':') ++ptr;
		goto process_file;
	}
err_out:	
	return files;
}

static bool
send_request(SOCKET sock, FileInfo *info, User *user)
{
	struct sockaddr_in sockaddr;
	char packet[MAX_UDPBUF];
	size_t packet_len;
	packet_no_t packet_no;
	command_no_t command;
	char attr[50];

	command = IPMSG_FILEATTACHOPT;
	if(info->attr & IPMSG_FILE_REGULAR) {
		/* format: packetID:fileID:offset */
		snprintf(attr, 50, "%lx:%x:%lx", info->packet_no, info->id, (ulong)0);
		command |= IPMSG_GETFILEDATA;
	}
	else if(info->attr & IPMSG_FILE_DIR) {
		/* format: packetID:fileID */
		snprintf(attr, 50, "%lx:%x", info->packet_no, info->id);
		command |= IPMSG_GETDIRFILES;
	}
	build_packet(packet, command, attr, NULL, &packet_len, &packet_no);
	setup_sockaddr(&sockaddr, user->ipaddr, config_get_port());

	/* send request */
	while(CONNECT(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
		if(errno == EINTR)
			continue;
		DEBUG_ERROR("connect error!");
		return false;
	}
	if(xsend(sock, packet, packet_len, 0) < 0)
		return false;
	
	return true;	
}


/**
 * recieve data and write to file descriptor fd.
 * @param sock socket to recieve data from.
 * @param fd file desciptor to write data.
 * @param size data length max recieve.
 * @return length of recieved data
 */
static ulong
recv_file_data(int sock, int fd, ulong size)
{
	ulong max_n_recv = size;
	ulong total_n_recv = 0;
	size_t n_recv = 0;
	size_t n_write = 0;
	char buf[MAX_TCPBUF];
	
	/* recieve data */
	while(max_n_recv) {
		memset(buf, 0, MAX_TCPBUF);
		n_recv = xrecv(sock, buf,MIN(max_n_recv, MAX_TCPBUF), 0);
		if(n_recv <= 0)
			break;
		n_write = xwrite(fd, buf, n_recv);
		if(n_write == -1)
			break;
		max_n_recv -= n_recv;
		total_n_recv += n_recv;
	}
	
	return total_n_recv;
}

static ulong
recv_regular_file(FileInfo *info, const char *path, User *user)
{
	SOCKET sock;
	int fd;
	ulong total_n_recv = 0;

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		DEBUG_ERROR("can not create socket!");
		return -1;
	}
	if(!send_request(sock, info, user)) {
		DEBUG_ERROR("send_request error!");
		CLOSE(sock);
		return -1;
	}

	/* create file */
	DEBUG_INFO("save file: %s", path);
	if((fd = open(path, O_WRONLY | O_CREAT |O_TRUNC, 00644)) < 0) {
		DEBUG_ERROR("open error!");
		return -1;
	}
	
	/* recieve file data */
	total_n_recv = recv_file_data(sock, fd, info->size - info->offset);
	close(fd);
	CLOSE(sock);

	DEBUG_INFO("total recv size: %ld", total_n_recv);

	return total_n_recv;
}

#define PEEK_SIZE 8

static bool
parse_dir_header(const char *buf, DirHeader *header)
{
	char *tok = NULL;
	char *ptr = NULL;

	if(!buf || !header) return false;

	//header-size
	tok = seprate_token(buf, ':', &ptr);
	header->head_size = strtol(tok, (char **)NULL, 16);
	if(ptr == NULL)
		return false;
	//filename
	tok = seprate_token(buf, ':', &ptr);
	strcpy(header->name, tok);
	if(ptr == NULL)
		return false;
	//file-size
	tok = seprate_token(buf, ':', &ptr);
	header->size = strtoll(tok, (char **)NULL, 16);
	if(ptr == NULL)
		return false;
	tok = seprate_token(buf, ':', &ptr);
	header->attr = strtoll(tok, (char **)NULL, 16);

	return true;
}

#define HEAD_PEEK_SIZE (10)

static bool
recv_dir_header(SOCKET sock, const char *buf)
{
	char *tok = NULL;
	char *ptr = NULL;
	int header_size = 0;

	if(xrecv(sock, buf, HEAD_PEEK_SIZE, MSG_PEEK) == -1) {//only peek header_size
		return false;
	}
	tok = seprate_token(buf, ':', &ptr);
	header_size = strtol(tok, (char **)NULL, 16);

	if(xrecv(sock, buf, header_size, 0) == -1) {//this time is real recieve
		return false;
	}
	DEBUG_INFO("recieved dir header: %s", buf);

	return true;
}


static ulong
recv_dir_file(FileInfo *info, const char *path, User *user)
{
	SOCKET sock;
	int fd;
	ulong total_n_recv = 0;
	size_t n_recv = 0;
	DirHeader header;
	char buf[MAX_TCPBUF];
	char tpath[MAX_PATH];
	char file_path[MAX_PATH];
	bool is_end = false;

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		DEBUG_ERROR("can not create socket!\n");
		return;
	}
	if(!send_request(sock, info, user)) {
		DEBUG_ERROR("send_request error!");
		CLOSE(sock);
		return -1;
	}
	
	build_path(tpath, path, ".");
	memset(buf, 0, MAX_TCPBUF);
	while(!is_end) {
		//recieve dir header
		memset(buf, 0, MAX_TCPBUF);
		if(!recv_dir_header(sock, buf) || !parse_dir_header(buf, &header))
			break;
		switch(ipmsg_flags_get_command(header.attr)) {
		case IPMSG_FILE_RETPARENT://parent dir
		{
			DEBUG_INFO("Dispatch file_retparent\n");
			build_path(tpath, tpath, "..");
			if(strlen(info->name) >= strlen(tpath)) {//return to base path
				is_end = true;
				break;
			}
			continue;
		}
		case IPMSG_FILE_DIR:
		{
			DEBUG_INFO("Dispatch file_dir\n");
			//construct full path
			build_path(tpath, tpath, header.name);
			if(access(tpath, F_OK) != 0) {//check whether dir exists
				mkdir(tpath, 00777);
			}
			continue;
		}
		case IPMSG_FILE_REGULAR:
		{
			DEBUG_INFO("Dispatch file_regular\n");
			//construct full path
			build_path(file_path, tpath, header.name);
			/* create file */
			DEBUG_INFO("save file: %s", file_path);
			if((fd = open(file_path, O_WRONLY | O_CREAT |O_TRUNC, 00644)) < 0) {
				DEBUG_ERROR("open error!");
				is_end = true;
				break;
			}
			// recieve file data
			n_recv = recv_file_data(sock, fd, header.size);
			total_n_recv += n_recv;
			close(fd);
			continue;
		}
		default:
			break;
		}
	}

	CLOSE(sock);
	DEBUG_INFO("total recv size: %ld", total_n_recv);

	return total_n_recv;
}

static bool
parse_request(const char *extend, RequestInfo *request) 
{
	char *tok = NULL;
	char *ptr = NULL;

	if(!extend || !request) return false;

	DEBUG_INFO("parsing request: %s", extend);
	memset(request, 0, sizeof(RequestInfo));
	//packetId
	tok = seprate_token(extend, ':', &ptr);
	request->packet_no = strtoll(tok, (char **)NULL, 16);
	if(!ptr)
		return false;

	//fileId
	tok = seprate_token(extend, ':', &ptr);
	request->file_id = strtol(tok, (char **) NULL, 16);

	//offset
	if(ptr)
		request->offset = strtoll(ptr, (char **)NULL, 16);
	else
		request->offset = 0;
	DEBUG_INFO("result: packetId: %ld, fileId :%d, offset: %ld",
		request->packet_no, request->file_id, request->offset);

	return true;
}

static ulong
send_file_data(SOCKET sock, const char *path)
{
	int fd;
	struct stat statbuf;
	char buf[MAX_TCPBUF];
	ulong max_n_read;
	ulong total_n_send = 0;
	size_t n_send = 0;
	size_t n_read = 0;

	DEBUG_INFO("sending file: %s", path);
	stat(path,&statbuf);
	max_n_read = statbuf.st_size;
	/* open file */
	if((fd = open(path, O_RDONLY)) < 0) {
		DEBUG_ERROR("open error!");
		return 0;
	}
	/* send file */
	while(max_n_read) {
		memset(buf, 0, MAX_TCPBUF);
		n_read = xread(fd, buf, MIN(max_n_read, MAX_TCPBUF));
		if(n_read <= 0)
			break;
		n_send = xsend(sock, buf, n_read, 0);
		if(n_send == -1)
			break;
		max_n_read -= n_read;
		total_n_send += n_send;
	}
	close(fd);

	return total_n_send;
}
static ulong
send_regular_file(SOCKET sock, FileInfo *info, User *user)
{
	ulong total_n_send = 0;

	DEBUG_INFO("sending reagular file: %s(id: %d)", info->name, info->id);
	total_n_send = send_file_data(sock, info->path);
	CLOSE(sock);
	
	DEBUG_INFO("total send size: %ld", total_n_send);
	
	return total_n_send;
}

static bool
send_dir_header(SOCKET sock, const char *name,  struct stat *statbuf, bool is_retparent)
{
	char header_buf[MAX_TCPBUF];
	size_t header_size = 0;

	memset(header_buf, 0, MAX_TCPBUF);
	if(is_retparent) {
		header_size = sprintf(header_buf, "0000:.:0:%lx:", IPMSG_FILE_RETPARENT);
	}
	else {
		if(!name || !statbuf) return false;
		command_no_t attr = S_ISDIR(statbuf->st_mode)?IPMSG_FILE_DIR:IPMSG_FILE_REGULAR;
		header_size = sprintf(header_buf, "0000:%s:%lx:%lx:%lx=%lx:%lx=%lx:",
			name, statbuf->st_size, attr,
			IPMSG_FILE_MTIME, statbuf->st_mtime,
			IPMSG_FILE_CREATETIME, statbuf->st_ctime);
	}
	header_buf[sprintf(header_buf, "%04x", header_size)] = ':';
	DEBUG_INFO("sending dir header: %s", header_buf);
	if(xsend(sock, header_buf, header_size, 0) == -1)
		return false;
	return true;
}

ulong
send_dir_data(SOCKET sock, const char *path, const char *dirname)
{
	DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
	char tpath[MAX_PATH];
	char file_path[MAX_PATH];
	char header_buf[MAX_TCPBUF];
	size_t header_size = 0;
	command_no_t attr = 0;
	ulong total_n_send = 0;
	size_t n_send = 0;
	size_t n_read = 0;

	build_path(tpath, path, ".");
	DEBUG_INFO("sending dir: %s", tpath);
	if((dp = opendir(tpath)) == NULL) {
        DEBUG_ERROR("opendir error!");
        return 0;
    }
	if(stat(tpath,&statbuf) == -1) {
		DEBUG_ERROR("stat error!");
		goto err_out;
    }
	if(!send_dir_header(sock, dirname, &statbuf, false))
		goto err_out;
    
    while((entry = readdir(dp)) != NULL) {
		build_path(file_path, tpath, entry->d_name);
        if(stat(file_path,&statbuf) == -1) {
			DEBUG_ERROR("stat error!");
			goto err_out;
        }
		if(S_ISDIR(statbuf.st_mode) && 
			(strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0))
        	continue;
        if(S_ISDIR(statbuf.st_mode)) {
            /* Recurse at a new indent level */
			total_n_send += send_dir_data(sock, file_path, entry->d_name);
        }
        else if(S_ISREG(statbuf.st_mode)){
			//construct absolute file path
			if(!send_dir_header(sock, entry->d_name, &statbuf, false))
				goto err_out;
			total_n_send += send_file_data(sock, file_path);
        }
    }
	//change to parent dir
	if(!send_dir_header(sock, NULL, NULL, true))
		goto err_out;
	build_path(tpath, tpath, "..");
err_out:
	closedir(dp);
	return total_n_send;
}

/*
  *	format:
  *	header-size:filename:file-size:fileattr[:extend-attr=val1 [,val2...][:extend-attr2=...]]:contents-data
  *	Next headersize: Next filename...
  * (All hex format except for filename and contetns-data)
  */
static ulong
send_dir_file(SOCKET sock, FileInfo *info, User *user)
{
	size_t total_n_send = 0;

	DEBUG_INFO("sending dir file: %s(id: %d)", info->name, info->id);
	total_n_send = send_dir_data(sock, info->path, info->name);
	CLOSE(sock);
	
	DEBUG_INFO("total send size: %ld", total_n_send);
	
	return total_n_send;
}

static gpointer
process_request_thread(gpointer data)
{
	TcpRequestArgs *args = (TcpRequestArgs *)data;
	SOCKET client_sock = args->client_sock;
	ulong ipaddr = args->ipaddr;
	FREE_WITH_CHECK(args);
	
	User *user = NULL;
	RequestInfo request;
	FileInfo *info = NULL;
	char buffer[MAX_TCPBUF];
	ssize_t n_recv = 0;
	Message msg;

	user = find_user(ipaddr);
	if(!user) return;

	memset(buffer, 0, MAX_TCPBUF);
	if((n_recv = RECV(client_sock, buffer, MAX_TCPBUF, 0)) <= 0) {
		DEBUG_ERROR("recv error!");
		return (gpointer)0;
	}
	DEBUG_INFO("recieved packet(TCP):%s", buffer);
	if(!(parse_message(ipaddr, &msg, buffer, n_recv)
		&& parse_request(msg.message, &request)
		&& validate_request(&request, &info)))
	{
		DEBUG_INFO("invalid request!");
		return (gpointer)0;
	}
	info->offset = request.offset;
	switch(msg.commandNo) {
		case IPMSG_GETFILEDATA:
		{
			DEBUG_INFO("Dispatch getfiledata\n");
			send_regular_file(client_sock, info, user);
			break;
		}
		case IPMSG_GETDIRFILES:
		{
			DEBUG_INFO("Dispatch getdirfiles\n");
			send_dir_file(client_sock, info, user);
			break;
		}
		default:
			break;
	};

	free_message_data(&msg);
	
	return (gpointer)0;
}

static gpointer 
recv_file_thread(gpointer data)
{
	RecvThreadArgs *args = (RecvThreadArgs *)data;
	FileInfo *info = args->info;
	User *user = args->user;
	
	switch(ipmsg_flags_get_command(info->attr)) {
		case IPMSG_FILE_REGULAR:
			build_path(args->path, args->path, info->name);
			recv_regular_file(info, args->path, user);
			break;
		case IPMSG_FILE_DIR:
			recv_dir_file(info, args->path, user);
			break;
		default:
			break;
	}
	FREE_WITH_CHECK(args);
}

void
tcp_request_entry(SOCKET client_sock, ulong ipaddr)
{
	TcpRequestArgs *args = (TcpRequestArgs *)malloc(sizeof(TcpRequestArgs));
	args->client_sock = client_sock;
	args->ipaddr = ipaddr;

	g_thread_create((GThreadFunc)process_request_thread, args, FALSE, NULL);
}

void
recv_file_entry(FileInfo *info, const char *path, User *user)
{
	RecvThreadArgs *args = (RecvThreadArgs *)malloc(sizeof(RecvThreadArgs));
	args->info = info;
	args->user = user;
	strcpy(args->path, path);

	g_thread_create((GThreadFunc)recv_file_thread, args, FALSE, NULL);
}
