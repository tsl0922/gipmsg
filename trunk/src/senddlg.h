#ifndef _GIPMSG_SENDDLG_H_
#define _GIPMSG_SENDDLG_H_

SendDlg *senddlg_new(DialogArgs *args);
User *getUser(SendDlg *dlg, ulong ipaddr);
void senddlg_init(SendDlg *dlg);
static bool IsSendFinish(SendDlg *dlg);
void senddlg_add_message(SendDlg *dlg, User *user, const char *msg,
	const struct tm* datetime , bool issendmsg);
void senddlg_add_fail_info(SendDlg *dlg, const char *msg);

bool SendFinishNotify(SendDlg *dlg, ulong ipaddr, packet_no_t packet_no);

#endif

