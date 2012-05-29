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

static int file_id = 0;

typedef struct {
	SendDlg *dlg;
	FileInfo *info;
	char path[MAX_PATH];
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

typedef struct {
	int sock;
	int fd;
	char fname[MAX_FNAME];
	ulong size;
	FileInfo *info;
	ulong *pnfilesrecv;
} RecvDataEntry;

typedef struct {
	SOCKET sock;
	char path[MAX_PATH];
	char fname[MAX_FNAME];
	FileInfo *info;
	ulong *pnfilesend;
} SendDataEntry;

static void *recv_file_thread(void *data);
static void *send_file_thread(void *data);

bool setup_file_info(FileInfo * info, const char *path)
{
	struct stat statbuf;
	char *sp;

	if (stat(path, &statbuf) == -1)
		return false;
	info->id = ++file_id;
	info->offset = 0;
	strcpy(info->path, path);
	sp = strrchr(path, '/');
	if (sp)
		strcpy(info->name, sp + 1);
	else
		strcpy(info->name, path);
	info->mtime = statbuf.st_mtime;
	info->atime = statbuf.st_atime;
	info->ctime = statbuf.st_ctime;
	info->size = S_ISDIR(statbuf.st_mode) ? 0 : statbuf.st_size;

	info->attr =
	    S_ISDIR(statbuf.st_mode) ? IPMSG_FILE_DIR : IPMSG_FILE_REGULAR;
	
	return true;
}

static void setup_progress_info(ProgressInfo *ppinfo, FileInfo *info, 
	const char *fname, TransStatus tstatus, ulong ssize, ulong tsize,
	ulong speed, ulong ntrans)
{
	ppinfo->info = info;
	strcpy(ppinfo->fname, fname);
	ppinfo->tstatus = tstatus;
	ppinfo->ssize = ssize;
	ppinfo->tsize = tsize;
	ppinfo->speed = speed;
	ppinfo->ntrans = ntrans;
}

static void setup_recv_data_entry(RecvDataEntry *entry, int sock, int fd, 
	char *fname, ulong size, FileInfo *info, ulong *pnfilesrecv)
{
	entry->sock = sock;
	entry->fd = fd;
	strcpy(entry->fname, fname);
	entry->size = size;
	entry->info = info;
	entry->pnfilesrecv = pnfilesrecv;
}

static void setup_send_data_entry(SendDataEntry *entry, SOCKET sock,
	char *path, char *fname, FileInfo *info, ulong *pnfilesend)
{
	entry->sock = sock;
	strcpy(entry->path, path);
	strcpy(entry->fname, fname);
	entry->info = info;
	entry->pnfilesend = pnfilesend;
}

/*
  * format:
  *
  * fileID:filename:size:mtime:fileattr[:extend-attr=val1 [,val2...][:extend-attr2=...]]:\a:fileID...
  * (size, mtime, and fileattr describe hex format. If a filename contains ':', please replace with "::".)
  *
  */
void send_file_info(User  *user, GSList * files)
{
	GSList *entry = files;
	FileInfo *info;
	char packet[MAX_UDPBUF];
	packet_no_t packet_no;
	size_t packet_len;
	char attach[MAX_UDPBUF];
	char *ptr;
	size_t len = 0;
	char *tattach;

	ptr = attach;
	while (entry) {
		info = (FileInfo *) entry->data;
		snprintf(ptr, MAX_UDPBUF - len, IPMSG_FILEATTACH_INFO_FMT,
			 info->id, info->name, info->size, info->mtime,
			 info->attr, IPMSG_FILE_ATIME, info->atime,
			 IPMSG_FILE_CTIME, info->ctime, FILELIST_SEPARATOR);

		len += strlen(ptr);
		ptr = attach + len;
		entry = entry->next;
	};

	tattach = convert_encode(user->encode, "utf-8", attach, strlen(attach));
	if(!tattach) {
		tattach = strdup(attach);
	}
	build_packet(packet,
		     IPMSG_SENDMSG | IPMSG_FILEATTACHOPT | IPMSG_SENDCHECKOPT,
		     NULL, tattach, &packet_len, &packet_no);
	ipmsg_send_packet(user->ipaddr, packet, packet_len);
	FREE_WITH_CHECK(tattach);

	//set packet_no
	entry = files;
	while (entry) {
		info = (FileInfo *) entry->data;
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
GList *parse_file_info(char *attach, packet_no_t packet_no)
{
	GList *files = NULL;
	FileInfo *file = NULL;
	char *tok = NULL;
	char *ptr = NULL;

	if (!attach)
		return NULL;

	DEBUG_INFO("parsing file info: %s", attach);
process_file:
	file = (FileInfo *) malloc(sizeof(FileInfo));
	memset(file, 0, sizeof(FileInfo));
	file->packet_no = packet_no;
	//fileileID
	tok = seprate_token(attach, ':', &ptr);
	file->id = strtol(tok, (char **)NULL, 16);
	if (!ptr) {
		FREE_WITH_CHECK(file);
		goto err_out;
	}
	//filename
	tok = seprate_token(attach, ':', &ptr);
	strcpy(file->name, tok);
	if (!ptr) {
		FREE_WITH_CHECK(file);
		goto err_out;
	}
	//size
	tok = seprate_token(attach, ':', &ptr);
	file->size = strtoll(tok, (char **)NULL, 16);
	if (!ptr) {
		FREE_WITH_CHECK(file);
		goto err_out;
	}
	//mtime
	tok = seprate_token(attach, ':', &ptr);
	file->mtime = strtoll(tok, (char **)NULL, 16);
	if (!ptr) {
		FREE_WITH_CHECK(file);
		goto err_out;
	}
	//fileattr
	tok = seprate_token(attach, ':', &ptr);
	file->attr = strtoll(tok, (char **)NULL, 16);
	if (!ptr) {
		FREE_WITH_CHECK(file);
		goto err_out;
	}

	files = g_list_append(files, file);
	//ignore extend attr
	tok = seprate_token(attach, FILELIST_SEPARATOR, &ptr);
	if (ptr && *ptr != '\0') {
		if (*ptr == ':')
			++ptr;
		goto process_file;
	}
err_out:
	return files;
}

/*
 * send tcp request for downloading regular/dir file
 * @param sock
 * @param info
 * @param user
 */
static bool send_request(SOCKET sock, FileInfo * info, User * user)
{
	struct sockaddr_in sockaddr;
	char packet[MAX_UDPBUF];
	size_t packet_len;
	packet_no_t packet_no;
	command_no_t command;
	char attr[50];

	command = IPMSG_FILEATTACHOPT;
	if (info->attr & IPMSG_FILE_REGULAR) {
		/* format: packetID:fileID:offset */
		snprintf(attr, 50, "%lx:%x:%lx", info->packet_no, info->id,
			 (ulong) 0);
		command |= IPMSG_GETFILEDATA;
	} else if (info->attr & IPMSG_FILE_DIR) {
		/* format: packetID:fileID */
		snprintf(attr, 50, "%lx:%x", info->packet_no, info->id);
		command |= IPMSG_GETDIRFILES;
	}
	build_packet(packet, command, attr, NULL, &packet_len, &packet_no);
	setup_sockaddr(&sockaddr, user->ipaddr, config_get_port());

	/* send request */
	while (CONNECT(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) <
	       0) {
		if (errno == EINTR)
			continue;
		DEBUG_ERROR("connect error!");
		return false;
	}
	if (xsend(sock, packet, packet_len, 0) < 0)
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
static ulong recv_file_data(RecvDataEntry *entry, SendDlg *dlg, bool is_dir)
{
	ulong max_n_recv = entry->size;
	ulong total_n_recv = 0;
	size_t n_recv = 0;
	size_t n_write = 0;
	char buf[MAX_TCPBUF];
	struct timeval tv1, tv2;
	ulong speed = 0;
	ProgressInfo pinfo;

	DEBUG_INFO("file size: %ld", max_n_recv);
	/* recieve data */
	while (max_n_recv) {
		memset(buf, 0, MAX_TCPBUF);
		GET_CLOCK_COUNT(&tv1);
		n_recv = xrecv(entry->sock, buf, MIN(max_n_recv, MAX_TCPBUF), 0);
		if (n_recv <= 0)
			break;
		n_write = xwrite(entry->fd, buf, n_recv);
		if (n_write == -1)
			break;
		max_n_recv -= n_recv;
		total_n_recv += n_recv;
		
		GET_CLOCK_COUNT(&tv2);
		speed = total_n_recv/difftimeval(tv2,tv1);
		if(!is_dir) {
			setup_progress_info(&dlg->recv_progress, entry->info, entry->fname, FILE_TS_DOING,
				entry->size, total_n_recv, speed, *(entry->pnfilesrecv));
		}
	}
	(*(entry->pnfilesrecv))++;
	
	return total_n_recv;
}

/*
 * recieve regular file
 * @param info file information
 * @param path save path
 * @param dlg
 */
static ulong recv_regular_file(FileInfo * info, const char *path, SendDlg *dlg)
{
	SOCKET sock;
	int fd;
	ulong total_n_recv = 0;
	static ulong nfilerecv = 0;
	struct timeval tv1, tv2;
	ulong speed = 0;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		DEBUG_ERROR("can not create socket!");
		return -1;
	}
	if (!send_request(sock, info, dlg->user)) {
		DEBUG_ERROR("send_request error!");
		CLOSE(sock);
		return -1;
	}
	setup_progress_info(&dlg->recv_progress, info, info->name, FILE_TS_READY,
		info->size, 0, 0, 0);

	/* create file */
	DEBUG_INFO("save file: %s", path);
	if ((fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 00644)) < 0) {
		DEBUG_ERROR("open error!");
		return -1;
	}
	RecvDataEntry entry;
	setup_recv_data_entry(&entry, sock, fd, info->name, info->size - info->offset,
		info, &nfilerecv);
	/* recieve file data */
	GET_CLOCK_COUNT(&tv1);
	total_n_recv = recv_file_data(&entry, dlg, false);
	GET_CLOCK_COUNT(&tv2);
	close(fd);
	CLOSE(sock);

	DEBUG_INFO("total recv size: %ld", total_n_recv);
	speed = total_n_recv/difftimeval(tv2,tv1);
	setup_progress_info(&dlg->recv_progress, info, info->name, FILE_TS_FINISH,
		info->size, total_n_recv, speed, nfilerecv);
	
	return total_n_recv;
}

#define PEEK_SIZE 8

/*
 * see @send_dir_header
 * @param buf
 * @param header
 */
static bool parse_dir_header(const char *buf, DirHeader * header)
{
	char *tok = NULL;
	char *ptr = NULL;

	if (!buf || !header)
		return false;

	//header-size
	tok = seprate_token(buf, ':', &ptr);
	header->head_size = strtol(tok, (char **)NULL, 16);
	if (ptr == NULL)
		return false;
	//filename
	tok = seprate_token(buf, ':', &ptr);
	strcpy(header->name, tok);
	if (ptr == NULL)
		return false;
	//file-size
	tok = seprate_token(buf, ':', &ptr);
	header->size = strtoll(tok, (char **)NULL, 16);
	if (ptr == NULL)
		return false;
	tok = seprate_token(buf, ':', &ptr);
	header->attr = strtoll(tok, (char **)NULL, 16);

	return true;
}

#define HEAD_PEEK_SIZE (10)

/* 
 * recieve the dir header to the given buffer, see @send_dir_header
 * @param sock
 * @param buf
 */
static bool recv_dir_header(SOCKET sock, const char *buf)
{
	char *tok = NULL;
	char *ptr = NULL;
	int header_size = 0;

	if (xrecv(sock, buf, HEAD_PEEK_SIZE, MSG_PEEK) == -1) {	//only peek header_size
		return false;
	}
	tok = seprate_token(buf, ':', &ptr);
	header_size = strtol(tok, (char **)NULL, 16);

	if (xrecv(sock, buf, header_size, 0) == -1) {	//this time is real recieve
		return false;
	}
	DEBUG_INFO("recieved dir header: %s", buf);

	return true;
}

/*
 * recieve dir file
 * @param info file information
 * @param path save path
 * @param dlg
 */
static ulong recv_dir_file(FileInfo * info, const char *path, SendDlg * dlg)
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
	static ulong nfilerecv = 0;
	struct timeval tv1, tv2;
	ulong speed = 0;
	ProgressInfo pinfo;
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		DEBUG_ERROR("can not create socket!\n");
		return;
	}
	if (!send_request(sock, info, dlg->user)) {
		DEBUG_ERROR("send_request error!");
		CLOSE(sock);
		return -1;
	}
	setup_progress_info(&dlg->recv_progress, info, info->name, FILE_TS_READY,
		info->size, 0, 0, 0);

	build_path(tpath, path, ".");
	memset(buf, 0, MAX_TCPBUF);
	GET_CLOCK_COUNT(&tv1);
	while (!is_end) {
		//recieve dir header
		memset(buf, 0, MAX_TCPBUF);
		if (!recv_dir_header(sock, buf)
		    || !parse_dir_header(buf, &header))
			break;
		switch (ipmsg_flags_get_command(header.attr)) {
		case IPMSG_FILE_RETPARENT:	//parent dir
			{
				DEBUG_INFO("Dispatch file_retparent\n");
				/* return to parent path */
				build_path(tpath, tpath, "..");
				if (strlen(path) >= strlen(tpath)) {	//return to base path
					is_end = true;
					break;
				}
				continue;
			}
		case IPMSG_FILE_DIR:
			{
				DEBUG_INFO("Dispatch file_dir\n");
				//jump into the dir
				build_path(tpath, tpath, header.name);
				if (access(tpath, F_OK) != 0) {	//check whether dir exists
					mkdir(tpath, 00777);
				}
				continue;
			}
		case IPMSG_FILE_REGULAR:
			{
				struct timeval tv11, tv22;
				DEBUG_INFO("Dispatch file_regular\n");
				//construct full path
				build_path(file_path, tpath, header.name);
				/* create file */
				DEBUG_INFO("save file: %s", file_path);
				if ((fd =
				     open(file_path,
					  O_WRONLY | O_CREAT | O_TRUNC,
					  00644)) < 0) {
					DEBUG_ERROR("open error!");
					is_end = true;
					break;
				}
				RecvDataEntry entry;
				setup_recv_data_entry(&entry, sock, fd, header.name, header.size,
					info, &nfilerecv);
				// recieve file data
				GET_CLOCK_COUNT(&tv11);
				n_recv = recv_file_data(&entry, dlg, true);
				total_n_recv += n_recv;
				GET_CLOCK_COUNT(&tv22);
				speed = total_n_recv/difftimeval(tv22,tv11);
				setup_progress_info(&dlg->recv_progress, entry.info, entry.fname, FILE_TS_DOING,
					entry.size, total_n_recv, speed, *(entry.pnfilesrecv));
				close(fd);
				continue;
			}
		default:
			break;
		}
	}
	GET_CLOCK_COUNT(&tv2);
	CLOSE(sock);
	DEBUG_INFO("total recv size: %ld", total_n_recv);
	speed = total_n_recv/difftimeval(tv2,tv1);
	setup_progress_info(&dlg->recv_progress, info, info->name, FILE_TS_FINISH, info->size,
		total_n_recv, speed, nfilerecv);
	
	return total_n_recv;
}

/*
 * see @send_request
 * @param extend
 * @param request
 */
static bool parse_request(const char *extend, RequestInfo * request)
{
	char *tok = NULL;
	char *ptr = NULL;

	if (!extend || !request)
		return false;

	DEBUG_INFO("parsing request: %s", extend);
	memset(request, 0, sizeof(RequestInfo));
	//packetId
	tok = seprate_token(extend, ':', &ptr);
	request->packet_no = strtoll(tok, (char **)NULL, 16);
	if (!ptr)
		return false;

	//fileId
	tok = seprate_token(extend, ':', &ptr);
	request->file_id = strtol(tok, (char **)NULL, 16);

	//offset
	if (ptr)
		request->offset = strtoll(ptr, (char **)NULL, 16);
	else
		request->offset = 0;
	DEBUG_INFO("result: packetId: %ld, fileId :%d, offset: %ld",
		   request->packet_no, request->file_id, request->offset);

	return true;
}

/*
 * send file data
 * @param entry
 * @param dlg
 */
static ulong send_file_data(SendDataEntry *entry, SendDlg *dlg)
{
	int fd;
	struct stat statbuf;
	char buf[MAX_TCPBUF];
	ulong max_n_read;
	ulong total_n_send = 0;
	size_t n_send = 0;
	size_t n_read = 0;
	struct timeval tv1, tv2;
	ulong speed = 0;

	DEBUG_INFO("sending file: %s", entry->path);
	stat(entry->path, &statbuf);
	max_n_read = statbuf.st_size;
	/* open file */
	if ((fd = open(entry->path, O_RDONLY)) < 0) {
		DEBUG_ERROR("open error!");
		return 0;
	}
	/* send file */
	while (max_n_read) {
		memset(buf, 0, MAX_TCPBUF);
		GET_CLOCK_COUNT(&tv1);
		n_read = xread(fd, buf, MIN(max_n_read, MAX_TCPBUF));
		if (n_read <= 0)
			break;
		n_send = xsend(entry->sock, buf, n_read, 0);
		if (n_send == -1)
			break;
		GET_CLOCK_COUNT(&tv2);
		max_n_read -= n_read;
		total_n_send += n_send;
		speed = total_n_send/difftimeval(tv2,tv1);
		setup_progress_info(&dlg->send_progress, 
			entry->info, entry->fname, FILE_TS_DOING,
			statbuf.st_size, total_n_send, speed, *(entry->pnfilesend));
	}
	(*(entry->pnfilesend))++;
	close(fd);

	return total_n_send;
}

/*
 * send regular file
 * @param sock
 * @param info
 * @param dlg
 */
static ulong send_regular_file(SOCKET sock, FileInfo * info, SendDlg *dlg)
{
	ulong total_n_send = 0;
	static ulong nfilesend = 0;
	struct timeval tv1, tv2;
	ulong speed = 0;
	SendDataEntry entry;

	DEBUG_INFO("sending reagular file: %s(id: %d)", info->name, info->id);
	setup_progress_info(&dlg->send_progress, info, info->name, FILE_TS_DOING,
		info->size, 0, 0, 0);
	setup_send_data_entry(&entry, sock, info->path, info->name,
		info, &nfilesend);
	GET_CLOCK_COUNT(&tv1);
	total_n_send = send_file_data(&entry, dlg);
	GET_CLOCK_COUNT(&tv2);
	CLOSE(sock);

	DEBUG_INFO("total send size: %ld", total_n_send);
	speed = total_n_send/difftimeval(tv2,tv1);
	setup_progress_info(&dlg->send_progress, info, info->name, FILE_TS_FINISH,
		info->size, total_n_send, speed, nfilesend);
	
	return total_n_send;
}

/*
 * format:
 *    header-size:filename:file-size:fileattr[:extend-attr=val1
 *	[,val2...][:extend-attr2=...]]:contents-data
 *	Next headersize: Next filename...
 *	(All hex format except for filename and contetns-data)
 */
static bool
send_dir_header(SOCKET sock, const char *name, struct stat *statbuf,
		bool is_retparent)
{
	char header_buf[MAX_TCPBUF];
	size_t header_size = 0;

	memset(header_buf, 0, MAX_TCPBUF);
	if (is_retparent) {
		header_size =
		    sprintf(header_buf, "0000:.:0:%lx:", IPMSG_FILE_RETPARENT);
	} else {
		if (!name || !statbuf)
			return false;
		command_no_t attr =
		    S_ISDIR(statbuf->
			    st_mode) ? IPMSG_FILE_DIR : IPMSG_FILE_REGULAR;
		header_size =
		    sprintf(header_buf, "0000:%s:%lx:%lx:%lx=%lx:%lx=%lx:",
			    name, statbuf->st_size, attr, IPMSG_FILE_MTIME,
			    statbuf->st_mtime, IPMSG_FILE_CREATETIME,
			    statbuf->st_ctime);
	}
	header_buf[sprintf(header_buf, "%04x", header_size)] = ':';
	DEBUG_INFO("sending dir header: %s", header_buf);
	if (xsend(sock, header_buf, header_size, 0) == -1)
		return false;
	return true;
}

/*
 * send dir data
 * @param sock
 * @param path file/dir path
 * @param dirname dir name
 * @param info file information
 * @param pnfilesend (out)num of files sended
 * @param dlg
 */
ulong send_dir_data(SOCKET sock, const char *path, const char *dirname,
	FileInfo *info, ulong *pnfilesend, SendDlg *dlg)
{
	DIR *dp;
	struct dirent *dentry;
	struct stat statbuf;
	char tpath[MAX_PATH];
	char file_path[MAX_PATH];
	char header_buf[MAX_TCPBUF];
	size_t header_size = 0;
	command_no_t attr = 0;
	ulong total_n_send = 0;
	size_t n_send = 0;
	size_t n_read = 0;
	ProgressInfo pinfo;

	build_path(tpath, path, ".");
	DEBUG_INFO("sending dir: %s", tpath);
	if ((dp = opendir(tpath)) == NULL) {
		DEBUG_ERROR("opendir error!");
		return 0;
	}
	if (stat(tpath, &statbuf) == -1) {
		DEBUG_ERROR("stat error!");
		goto err_out;
	}
	if (!send_dir_header(sock, dirname, &statbuf, false))
		goto err_out;

	while ((dentry = readdir(dp)) != NULL) {
		build_path(file_path, tpath, dentry->d_name);
		if (stat(file_path, &statbuf) == -1) {
			DEBUG_ERROR("stat error!");
			goto err_out;
		}
		if (S_ISDIR(statbuf.st_mode) &&
		    (strcmp(".", dentry->d_name) == 0
		     || strcmp("..", dentry->d_name) == 0))
			continue;
		if (S_ISDIR(statbuf.st_mode)) {
			/* Recurse at a new indent level */
			total_n_send +=
			    send_dir_data(sock, file_path, dentry->d_name, info, pnfilesend, dlg);
		} else if (S_ISREG(statbuf.st_mode)) {
			//construct absolute file path
			if (!send_dir_header
			    (sock, dentry->d_name, &statbuf, false))
				goto err_out;
			SendDataEntry entry;
			setup_send_data_entry(&entry, sock, file_path, dentry->d_name,
				info, pnfilesend);
			total_n_send += send_file_data(&entry, dlg);
		}
	}
	//change to parent dir
	if (!send_dir_header(sock, NULL, NULL, true))
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
static ulong send_dir_file(SOCKET sock, FileInfo * info, SendDlg *dlg)
{
	size_t total_n_send = 0;
	static ulong nfilesend = 0;
	struct timeval tv1, tv2;
	ulong speed = 0;

	DEBUG_INFO("sending dir file: %s(id: %d)", info->name, info->id);
	setup_progress_info(&dlg->send_progress, info, info->name, FILE_TS_DOING, info->size, 0, 0, 0);
	GET_CLOCK_COUNT(&tv1);
	total_n_send = send_dir_data(sock, info->path, info->name, info, &nfilesend, dlg);
	GET_CLOCK_COUNT(&tv2);
	CLOSE(sock);

	DEBUG_INFO("total send size: %ld", total_n_send);
	speed = total_n_send/difftimeval(tv2,tv1);
	setup_progress_info(&dlg->send_progress, info, info->name, FILE_TS_FINISH, info->size,
		total_n_send, speed, nfilesend);
	
	return total_n_send;
}

/*
 * check wether a download request is valid
 * @param request
 * @param dlg
 */
FileInfo *validate_request(RequestInfo * request, SendDlg *dlg)
{
	GList *entry = g_list_first(dlg->file_list);
	FileInfo *file = NULL;

	while (entry) {
		FileInfo *tfile = (FileInfo *) entry->data;
		if ((tfile->id == request->file_id)
		    && (tfile->packet_no == request->packet_no)) {
		    file = tfile;
			break;
		}
		entry = entry->next;
	}
	
	return file;
}

static void *process_request_thread(void *data)
{
	TcpRequestArgs *args = (TcpRequestArgs *) data;
	SOCKET client_sock = args->client_sock;
	ulong ipaddr = args->ipaddr;
	FREE_WITH_CHECK(args);

	User *user = NULL;
	RequestInfo request;
	FileInfo *info = NULL;
	char buffer[MAX_TCPBUF];
	ssize_t n_recv = 0;
	Message msg;
	SendDlg *dlg;
	bool is_new;
	ProgressInfo pinfo;

	user = find_user(ipaddr);
	if (!user)
		return;
	dlg = send_dlg_open(user, &is_new);
	if(is_new) active_dlg(dlg, false);
	memset(buffer, 0, MAX_TCPBUF);
	if ((n_recv = RECV(client_sock, buffer, MAX_TCPBUF, 0)) <= 0) {
		DEBUG_ERROR("recv error!");
		pthread_exit(NULL);
	}
	DEBUG_INFO("recieved packet(TCP):%s", buffer);
	if(!parse_message(ipaddr, &msg, buffer, n_recv))
		pthread_exit(NULL);
	if(!parse_request(msg.message, &request))
		pthread_exit(NULL);
	if(!(info = validate_request(&request, dlg)))
		pthread_exit(NULL);
	info->offset = request.offset;
	setup_progress_info(&pinfo, info, info->name, FILE_TS_READY,
		info->size, 0, 0, 0);
	switch (msg.commandNo) {
	case IPMSG_GETFILEDATA:
		{
			DEBUG_INFO("Dispatch getfiledata\n");
			send_regular_file(client_sock, info, dlg);
			break;
		}
	case IPMSG_GETDIRFILES:
		{
			DEBUG_INFO("Dispatch getdirfiles\n");
			send_dir_file(client_sock, info, dlg);
			break;
		}
	default:
		break;
	};

	free_message_data(&msg);

	pthread_exit(NULL);
}

static void *recv_file_thread(void *data)
{
	RecvThreadArgs *args = (RecvThreadArgs *) data;
	FileInfo *info = args->info;
	
	switch (ipmsg_flags_get_command(info->attr)) {
	case IPMSG_FILE_REGULAR:
		build_path(args->path, args->path, info->name);
		recv_regular_file(info, args->path, args->dlg);
		break;
	case IPMSG_FILE_DIR:
		recv_dir_file(info, args->path, args->dlg);
		break;
	default:
		break;
	}
	FREE_WITH_CHECK(args);

	pthread_exit(NULL);
}

void tcp_request_entry(SOCKET client_sock, ulong ipaddr)
{
	pthread_t process_thread;
	TcpRequestArgs *args =
	    (TcpRequestArgs *) malloc(sizeof(TcpRequestArgs));
	args->client_sock = client_sock;
	args->ipaddr = ipaddr;

	pthread_create(&process_thread, NULL, process_request_thread, args);
}

void recv_file_entry(FileInfo * info, const char *path, SendDlg *dlg)
{
	pthread_t recv_thread;
	RecvThreadArgs *args =
	    (RecvThreadArgs *) malloc(sizeof(RecvThreadArgs));
	args->dlg = dlg;
	args->info = info;
	strcpy(args->path, path);

	pthread_create(&recv_thread, NULL, recv_file_thread, args);
}
