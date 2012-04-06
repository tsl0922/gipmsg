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

#include <common.h>

struct emotion{
	const char* name;
	const char* symbol;
	int id;
};

static struct emotion emotions[] = {
	{ N_("Smile") , ":)", 1 } ,
	{ N_("Twitch nouth") , ":~", 2 } ,
	{ N_("Se") , ":*", 3 } ,
	{ N_("Silly") , ":|", 4 } ,
	{ N_("Pride") , "8-)", 5 } ,
	{ N_("Tears") , ":<", 6 } ,
	{ N_("Shy") , ":$", 7 } ,
	{ N_("Shut up") , ":X", 8 } ,
	{ N_("Tired") , ":Z", 9 } ,
	{ N_("Cry") , ":'(", 10 } ,
	{ N_("Awkward") , ":-|", 11 } ,
	{ N_("Angry") , ":@", 12 } ,
	{ N_("Naughty") , ":P", 13 } ,
	{ N_("Laugh") , ":D", 14 } ,
	{ N_("Surprise") , ":O", 15 } ,
	{ N_("Rotate") , "<rotate>", 16 } ,
	{ N_("Sad") , ":(", 17 } ,
	{ N_("Cool") , ":+", 18 } ,
	{ N_("Lenhan") , ":lenhan", 19 } ,
	{ N_("Zhuakuang") , ":Q", 20 } ,
	{ N_("Shuai") , ":T", 21 } ,
	{ N_("Human skeleton") , ";P", 22 } ,
	{ N_("Konck") , ";-D", 23 } ,
	{ N_("Bye") , ";d", 24 } ,
	{ N_("CSweat") , ";o", 25 } ,
	{ N_("KNose") , ":g", 26 } ,
	{ N_("Applause") , "|-)", 27 } ,
	{ N_("Qiudale") , ":!", 28 } ,
	{ N_("Huaixiao") , ":L", 29 } ,
	{ N_("Red Lips") , ":>", 30 } ,
	{ N_("Red Rose") , ";bin", 31 } ,
	{ N_("Withered Rose") , ":fw", 32 } ,
	{ N_("Gift Box") , ";fd", 33 } ,
	{ N_("Birthday Cake") , ":-S", 34 } ,
	{ N_("Music") , ";?", 35 } ,
	{ N_("Bulb") , ";x", 36 } ,
	{ N_("Idea") , ";@", 37 } ,
	{ N_("Coffee") , ":8", 38 } ,
	{ N_("Umbrella") , ";!", 39 } ,
	{ N_("Mobile Phone") , "!!!", 40 } ,
	{ N_("Computer") , ":xx", 41 } ,
	{ N_("Disappointed") , ":bye", 42 } ,
	{ N_("Confused") , ":csweat", 43 } ,
	{ N_("Worried") , ":knose", 44 } ,
	{ N_("Worried") , ":applause", 45 } ,
	{ N_("Worried") , ":cdale", 46 } ,
	{ N_("Worried") , ":huaixiao", 47 } ,
	{ N_("Worried") , ":shake", 48 } ,
	{ N_("Drinks") , ":lhenhen", 49 } ,
	{ N_("Goblet") , ":rhenhen", 50 } ,
	{ N_("Angel") , ":yawn", 51 } ,
	{ N_("Thinking") , ":snooty", 52 } ,
	{ N_("Great") , ":chagrin", 53 } ,
	{ N_("Naughty") , ":kcry", 54 } ,
	{ N_("Idiot") , ":yinxian", 55 } ,
	{ N_("Sunglasses") , ":qinqin", 56 } ,
	{ N_("Smile") , ":xiaren", 57 } ,
	{ N_("Laugh") , ":kelin", 58 } ,
	{ N_("Wink") , ":caidao", 59 } ,
	{ N_("Surprised") , ":xig", 60 } ,
	{ N_("Tongue smile") , ":bj", 61 } ,
	{ N_("Warm smile") , ":basketball", 62 } ,
	{ N_("Angry") , ":pingpong", 63 } ,
	{ N_("Sad") , ":jump", 64 } ,
	{ N_("Cry") , ":coffee", 65 } ,
	{ N_("Awkward") , ":eat", 66 } ,
	{ N_("Irony") , ":pig", 67 } ,
	{ N_("Illed") , ":rose", 68 } ,
	{ N_("Gritting my teeth") , ":fade", 69 } ,
	{ N_("Tired") , ":kiss", 70 } ,
	{ N_("Secrecy") , ":heart", 71 } ,
	{ N_("Googly eyes") , ":break", 72 } ,
	{ N_("Sleeping Moon") , ":cake", 73 } ,
	{ N_("Rain") , ":shd", 74 } ,
	{ N_("Clock") , ":bomb", 75 } ,
	{ N_("Red Heart") , ":dao", 76 } ,
	{ N_("Broken Heart") , ":footb", 77 } ,
	{ N_("Face of Cat") , ":piaocon", 78 } ,
	{ N_("Face of Dog") , ":shit", 79 } ,
	{ N_("Snail") , ":oh", 80 } ,
	{ N_("Star") , ":moon", 81 } ,
	{ N_("Sun") , ":sun", 82 } ,
	{ N_("Rainbow") , ";gift", 83 } ,
	{ N_("Hug left") , ":hug", 84 } ,
	{ N_("Hug right") , ":strong", 85 } ,
	{ N_("Red Lips") , ";weak", 86 } ,
	{ N_("Red Rose") , ":share", 87 } ,
	{ N_("Withered Rose") , ":shl", 88 } ,
	{ N_("Gift Box") , ":baoquan", 89 } ,
	{ N_("Birthday Cake") , ":cajole", 90 } ,
	{ N_("Music") , ":quantou", 91 } ,
	{ N_("Bulb") , ":chajin", 92 } ,
	{ N_("Idea") , ":aini", 93 } ,
	{ N_("Coffee") , ":sayno", 94 } ,
	{ N_("Umbrella") , ":sayok", 95 } ,
	{ N_("Mobile Phone") , ":love", 96 } ,
	{ "" , "" , 0}
};

static int emotion_count = 96;

static gboolean
emotion_focus_out(GtkWidget *widget, GdkEventFocus *event , gpointer data)
{
	SendDlg *dlg = (SendDlg *)data;
	gtk_widget_destroy(dlg->emotionDlg);
	dlg->emotionDlg = NULL;
	
	return TRUE;
}

static gboolean
emotion_clicked(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	struct EmotionArgs *emotionArgs = (struct EmotionArgs *)data;
	GtkTextBuffer* buffer;
	GtkTextIter iter;
	char buf[MAX_NAMEBUF];

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(emotionArgs->dlg->send_text));

	gtk_text_buffer_get_end_iter(buffer , &iter);
	sprintf(buf, "/%s", emotions[emotionArgs->id - 1].symbol);
	gtk_text_buffer_insert(buffer , &iter , buf , -1);

	gtk_widget_destroy(emotionArgs->dlg->emotionDlg);
	emotionArgs->dlg->emotionDlg = NULL;

	return FALSE;
}

void
show_emotion_dialog(SendDlg *dlg, int x, int y)
{
	GtkWidget *table;
	GtkWidget *frame, *subframe;
	GtkWidget *image, *eventbox;
	char path[MAX_BUF];
	char tooltip[MAX_BUF];
	int i, j, k;
	struct EmotionArgs *emotionArgs;
	
	dlg->emotionDlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_decorated(GTK_WINDOW(dlg->emotionDlg) , FALSE);
	gtk_window_set_type_hint(GTK_WINDOW(dlg->emotionDlg), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_default_size(GTK_WINDOW(dlg->emotionDlg) , 480 , 180);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW(dlg->emotionDlg), TRUE);
	gtk_window_move(GTK_WINDOW(dlg->emotionDlg) , x , y);

	table = gtk_table_new(16 , 6 , FALSE);
	gtk_widget_set_usize(table , 480 , 180);
	gtk_table_set_col_spacings(GTK_TABLE(table), 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), 0);

	frame = gtk_frame_new(NULL);
	
	gtk_widget_set_events(dlg->emotionDlg , GDK_ALL_EVENTS_MASK);
	g_signal_connect(dlg->emotionDlg, "focus-out-event", G_CALLBACK(emotion_focus_out), dlg);

	i = j = k = 0;
	for( j = 0 ; j < 6 ; j ++){
		for( i = 0 ; i < 16 ; i ++){
			subframe = gtk_frame_new(NULL);
			gtk_frame_set_shadow_type(GTK_FRAME(subframe) , GTK_SHADOW_ETCHED_IN);
			memset(path, 0, sizeof(path));
			if( k < emotion_count ){
				sprintf(path , EMOTION_PATH "%d.gif" ,(k++) + 1);
				image = gtk_image_new_from_file(path);
				if(!image)
					continue;
				eventbox = gtk_event_box_new();
				sprintf(tooltip, "%s /%s", emotions[k - 1].name, emotions[k - 1].symbol);
				gtk_widget_set_tooltip_text(eventbox , tooltip);
				gtk_container_add(GTK_CONTAINER(eventbox) , image);
				emotionArgs = (struct EmotionArgs *)malloc(sizeof(struct EmotionArgs));
				emotionArgs->dlg = dlg;
				emotionArgs->id = k;
				g_signal_connect(eventbox , "button-press-event" 
						, GTK_SIGNAL_FUNC(emotion_clicked) , emotionArgs);
				gtk_container_add(GTK_CONTAINER(subframe) , eventbox);
			}
			gtk_table_attach_defaults(GTK_TABLE(table) , subframe , i , i + 1 , j , j + 1);
		}
	}
	
	gtk_container_add(GTK_CONTAINER(frame) , table);
	gtk_container_add(GTK_CONTAINER(dlg->emotionDlg) , frame);

	gtk_widget_show_all(dlg->emotionDlg);
}

void
msg_parse_emotion(GtkWidget *text, GtkTextIter *iter, const char *msg)
{
	GtkTextBuffer *buffer;
	GtkTextChildAnchor *anchor;
	GtkWidget *image;
	char path[MAX_BUF];
	int i, j, p, id, smlen;

	i = j = p = smlen = 0;
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	while(msg[i] != '\0') {
		if(msg[i] == '/') {
			//check emotion
			for(j = 0;j < emotion_count; j++) {
				id = -1;
				smlen = strlen(emotions[j].symbol);
				if(!strncmp(msg + i + 1, emotions[j].symbol, smlen)) {
					id = emotions[j].id;
					break;
				}
			}
			if(id > 0) {
				//insert text before emotion
				gtk_text_buffer_insert_with_tags_by_name(buffer, iter,
					msg + p , i - p , "tm10" , NULL);
				//insert emotion
				sprintf(path , EMOTION_PATH "%d.gif" , id);
				image = gtk_image_new_from_file(path);
				gtk_widget_show(image);
				anchor = gtk_text_buffer_create_child_anchor(buffer , iter);
				gtk_text_view_add_child_at_anchor(text, image , anchor);
				i += smlen +1;
				p = i;
				continue;
			}
		}
		i++;
	}
	if(p < strlen(msg)) {
		gtk_text_buffer_insert_with_tags_by_name(buffer, iter,
			msg + p , strlen(msg) - p , "lm10" , NULL);
	}
}
