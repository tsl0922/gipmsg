#include "common.h"

static int file_id = 0;

void
SendFileInfo(ulong toAddr, GSList *files)
{
	FileInfo info;
	gchar *path;
	char attach[MAX_UDPBUF];
	char *ptr;
	size_t len = 0;

	ptr = attach;
	while(files) {
		path = (char *)files->data;
		if(!setup_file_info(&info, ++file_id, path))
			continue;
		
		snprintf(ptr, MAX_UDPBUF - len, IPMSG_FILEATTACH_INFO_FMT, info.id,
			info.name, info.size, info.mtime, info.attr,
			IPMSG_FILE_ATIME, info.atime,
			IPMSG_FILE_CTIME, info.ctime,
			FILELIST_SEPARATOR);

		len += strlen(ptr);
		ptr = attach + len;
		g_free(path);
		files = files->next;
	};
	ipmsg_send_file_info(toAddr, 0, attach);
	g_slist_free(files);
}

