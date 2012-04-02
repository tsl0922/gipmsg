#include "common.h"

bool
config_init() {
}

bool
config_read(Config *config) {
	GKeyFile *keyfile;
	GError *error = NULL;
	const gchar *file = CONFIG_FILE_PATH;

	keyfile = g_key_file_new();
	if(!g_key_file_load_from_file(keyfile, file, G_KEY_FILE_NONE, &error)) {
		g_key_file_free(keyfile);
		return false;
	}

	return true;
}

bool
config_write(Config *config) {
	return false;
}

int
config_get_port() {
	return IPMSG_DEFAULT_PORT;
}

char *
config_get_home_path() {
	return HOME_PATH;
}

char *
config_get_icon_path() {
	return ICON_PATH;
}

char *
config_get_photo_path() {
	return PHOTO_PATH;
}

char *
config_get_locale_path() {
	return LOCALE_PATH;
}

char *
config_get_nickname() {
	return "lucky";
}

char *
config_get_groupname() {
	return "gipmsg";
}

command_no_t
config_get_normal_send_flags() {
	return IPMSG_SENDCHECKOPT;
}
command_no_t
config_get_normal_entry_flags() {
	return 0;
}

char *
config_get_default_headicon() {
	return DEFAULT_HEAD_ICON;
}

char *
config_get_icon_image() {
	return ICON_IMAGE;
}

char *
config_get_background_image() {
	return BACK_GROUND_IMAGE;
}

char *
config_get_expander_closed_icon() {
	return EXPANDER_CLOSED_ICON;
}

char *
config_get_expander_open_icon() {
	return EXPANDER_OPEN_ICON;
}

char *
config_get_emotion_dir() {
	return EMOTION_DIR;
}
