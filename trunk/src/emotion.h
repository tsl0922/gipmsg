#ifndef _EMOTION_H_
#define _EMOTION_H_

struct EmotionArgs {
	SendDlg *dlg;
	int id;
};


void show_emotion_dialog(SendDlg *dlg, int x, int y);
void msg_parse_emotion(GtkWidget *text, GtkTextIter *iter, const char *msg);

#endif /*_EMOTION_H_*/

