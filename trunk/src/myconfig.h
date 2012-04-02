#ifndef __GIPMSG_CONFIG_H__
#define __GIPMSG_CONFIG_H__

#ifdef _LINUX
#define HOME_PATH      "/home/tsl0922/workspace/gipmsg/"
#endif
#ifdef WIN32
#define HOME_PATH      "/home/tsl0922/workspace/gipmsg/"
#endif
#define CONFIG_FILE_PATH     "~/.config/ichat/config.ini"
#define ICON_PATH      HOME_PATH "icons/"
#define PHOTO_PATH     HOME_PATH "photos/"
#define LOCALE_PATH    HOME_PATH "locales/"

#define DEFAULT_HEAD_ICON    HOME_PATH "pixmaps/icon_linux.png"
#define ICON_IMAGE           HOME_PATH "pixmaps/ubuntu1_64.png"
#define BACK_GROUND_IMAGE    HOME_PATH "pixmaps/background.png"
#define EXPANDER_CLOSED_ICON HOME_PATH "pixmaps/expender-closed.png"
#define EXPANDER_OPEN_ICON   HOME_PATH "pixmaps/expender-open.png"

#define IMAGE_DIR            HOME_PATH "pixmaps/"
#define EMOTION_DIR          HOME_PATH "pixmaps/emotions/"

typedef struct {
} Config;

bool config_init();
bool config_read(Config *config);
bool config_write(Config *config);

int config_get_port();
char *config_get_home_path();
char *config_get_icon_path();
char *config_get_photo_path();
char *config_get_locale_path();

char *config_get_nickname();
char *config_get_groupname();

command_no_t config_get_normal_send_flags();
command_no_t config_get_normal_entry_flags();

char *config_get_default_headicon();
char *config_get_icon_image();
char *config_get_background_image();
char *config_get_expander_closed_icon();
char *config_get_expander_open_icon();
char *config_get_emotion_dir();

extern Config config;
#endif
