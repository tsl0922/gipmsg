#include <common.h>

GList *user_list = NULL;
GStaticMutex user_list_mutex = G_STATIC_MUTEX_INIT;

static void
fill_user_info(Message *msg, User *user)
{
	if(!msg || !user) return;
	memset(user, 0, sizeof(User));

	user->ipaddr = msg->fromAddr;
	user->version = strdup(msg_get_version(msg));
	user->nickName = strdup(msg_get_nickName(msg));
	user->userName = strdup(msg_get_userName(msg));
	user->hostName = strdup(msg_get_hostName(msg));
	user->groupName = strdup(msg_get_groupName(msg));
	user->headIcon = strdup(config_get_default_headicon());
	user->encode = strdup(msg_get_encode(msg));
	user->state = USER_STATE_ONLINE;
}

User *
create_user(Message *msg)
{
	User *new_user;

	if(!msg) return NULL;
	new_user = (User *)malloc(sizeof(User));
	fill_user_info(msg, new_user);
	
	return new_user;
}

void
free_user(User *user)
{
	if(user != NULL) {
		FREE_WITH_CHECK(user->version);
		FREE_WITH_CHECK(user->nickName);
		FREE_WITH_CHECK(user->userName);
		FREE_WITH_CHECK(user->hostName);
		FREE_WITH_CHECK(user->groupName);
		FREE_WITH_CHECK(user->photoImage);
		FREE_WITH_CHECK(user->headIcon);
		FREE_WITH_CHECK(user->personalSign);
		FREE_WITH_CHECK(user->encode);
		user->ipaddr = 0;
		free(user);
	}
}

void
clear_user_list()
{
	GList *entry;

	g_static_mutex_lock(&user_list_mutex);
	g_list_foreach(user_list, (GFunc)free_user, NULL);
	g_list_free(user_list);
	user_list = NULL;
	g_static_mutex_unlock(&user_list_mutex);
}

void
print_user(User *user)
{
	if(!user) return;
	char *addr_str = get_addr_str(user->ipaddr);
	printf("\tipaddr: %s\n", addr_str);
	printf("\tnickName: %s\n", user->nickName);
	printf("\tuserName: %s\n", user->userName);
	printf("\thostName: %s\n", user->hostName);
	printf("\tgroupName: %s\n", user->groupName);
	printf("****************\n");
	FREE_WITH_CHECK(addr_str);
}

void
print_user_list()
{
	printf("-------user list-------\n");
	g_list_foreach(user_list, (GFunc)print_user, NULL);
	printf("-------   end   -------\n");
}

bool
add_user(Message *msg)
{
	User *new_user;
	bool rc = false;
	
	if(!msg) return rc;
	
	new_user = create_user(msg);
	if(!find_user(msg->fromAddr)) {
		g_static_mutex_lock(&user_list_mutex);
		user_list = g_list_append(user_list, new_user);
		g_static_mutex_unlock(&user_list_mutex);
		users_tree_add_user(new_user);
		rc = true;
	}
	else {
		free_user(new_user);
		rc = false;
	}
	
	return rc;
}

static gint
compare_by_ipaddr(gconstpointer a, gconstpointer b)
{
	User *ua, *ub;

	ua = (User *)a;
	ub = (User *)b;

	return (ua->ipaddr - ub->ipaddr);
}

User *
find_user(ulong ipaddr)
{
	GList *entry;
	User tmp_user;
	User *user = NULL;

	if(!user_list || !ipaddr) return NULL;
	
	memset(&tmp_user, 0, sizeof(User));
	tmp_user.ipaddr = ipaddr;
	entry = g_list_find_custom(user_list, &tmp_user, compare_by_ipaddr);
	if(entry) {
		g_assert(entry->data);
		user = entry->data;
	}
	
	return user;
}

bool
del_user(Message *msg)
{
	GList *entry;
	User tmp_user;
	User *user;
	bool rc = false;

	if(!user_list || !msg) return rc;
	
	memset(&tmp_user, 0, sizeof(User));
	tmp_user.ipaddr = msg->fromAddr;
	g_static_mutex_lock(&user_list_mutex);
	entry = g_list_find_custom(user_list, &tmp_user, compare_by_ipaddr);
	if(!entry) {
		rc = false;
	} 
	else {
		user = entry->data;
		users_tree_del_user(msg->fromAddr);
//			free_user(user);
//			user_list = g_list_remove_link(user_list, entry);
//			g_list_free(entry);
		user->state = USER_STATE_OFFLINE;
		rc = true;
	}
	g_static_mutex_unlock(&user_list_mutex);
	
	return rc;
}

bool
update_user(Message *msg)
{
	GList *entry;
	User tmp_user;
	User *user;
	bool rc = false;

	if(!user_list || !msg) return rc;
	
	memset(&tmp_user, 0, sizeof(User));
	tmp_user.ipaddr = msg->fromAddr;
	g_static_mutex_lock(&user_list_mutex);
	entry = g_list_find_custom(user_list, &tmp_user, compare_by_ipaddr);
	if(!entry) {
		rc = false;
	} 
	else {
		user = entry->data;
		fill_user_info(msg, user);
		users_tree_update_user(user);
		rc = true;
	}
	g_static_mutex_unlock(&user_list_mutex);
	
	return rc;
}
