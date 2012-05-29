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

#include <common.h>

GList *user_list = NULL;
pthread_mutex_t user_list_mutex = PTHREAD_MUTEX_INITIALIZER;

static void fill_user_info(Message * msg, User * user)
{
	if (!msg || !user)
		return;
	memset(user, 0, sizeof(User));

	user->ipaddr = msg->fromAddr;
	STRDUP_WITH_CHECK(user->version, msg_get_version(msg));
	STRDUP_WITH_CHECK(user->nickName, msg_get_nickName(msg));
	STRDUP_WITH_CHECK(user->userName, msg_get_userName(msg));
	STRDUP_WITH_CHECK(user->hostName, msg_get_hostName(msg));
	STRDUP_WITH_CHECK(user->groupName, msg_get_groupName(msg));
	STRDUP_WITH_CHECK(user->encode, msg_get_encode(msg));
	user->headIcon = NULL;
	user->photoImage = NULL;
	user->personalSign = NULL;
}

User *create_user(Message * msg)
{
	User *new_user;

	if (!msg)
		return NULL;
	new_user = (User *) malloc(sizeof(User));
	fill_user_info(msg, new_user);

	return new_user;
}

User *
dup_user(User *src)
{
	User *new_user = NULL;
	
	if(!src)
		return false;
	new_user = (User *)malloc(sizeof(User));
	memset(new_user, 0, sizeof(User));
	new_user->ipaddr = src->ipaddr;
	STRDUP_WITH_CHECK(new_user->version, src->version);
	STRDUP_WITH_CHECK(new_user->nickName, src->nickName);
	STRDUP_WITH_CHECK(new_user->userName, src->userName);
	STRDUP_WITH_CHECK(new_user->hostName, src->hostName);
	STRDUP_WITH_CHECK(new_user->groupName, src->groupName);
	STRDUP_WITH_CHECK(new_user->headIcon, ICON_PATH "icon_linux.png");
	STRDUP_WITH_CHECK(new_user->encode, src->encode);
	
	return new_user;
}

void free_user_data(User * user)
{
	if (user != NULL) {
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
	}
}

void clear_user_list()
{
	GList *entry;

	pthread_mutex_lock(&user_list_mutex);
	g_list_foreach(user_list, (GFunc) free_user_data, NULL);
	g_list_free(user_list);
	user_list = NULL;
	pthread_mutex_unlock(&user_list_mutex);
}

void print_user(User * user)
{
	if (!user)
		return;
	char *addr_str = get_addr_str(user->ipaddr);
	printf("\tipaddr: %s\n", addr_str);
	printf("\tnickName: %s\n", user->nickName);
	printf("\tuserName: %s\n", user->userName);
	printf("\thostName: %s\n", user->hostName);
	printf("\tgroupName: %s\n", user->groupName);
	printf("\tencode: %s\n", user->encode);
	printf("****************\n");
	FREE_WITH_CHECK(addr_str);
}

void print_user_list()
{
	printf("-------user list-------\n");
	g_list_foreach(user_list, (GFunc) print_user, NULL);
	printf("-------   end   -------\n");
}

/* this function always return a no-null user as long as msg is not null */
User *add_user(Message * msg)
{
	User *tuser = NULL;

	if (!msg)
		return NULL;

	tuser = find_user(msg->fromAddr);
	if (!tuser) {
		tuser = create_user(msg);
		pthread_mutex_lock(&user_list_mutex);
		user_list = g_list_append(user_list, tuser);
		pthread_mutex_unlock(&user_list_mutex);
	}

	return tuser;
}

static gint compare_by_ipaddr(gconstpointer a, gconstpointer b)
{
	User *ua, *ub;

	ua = (User *) a;
	ub = (User *) b;

	return (ua->ipaddr - ub->ipaddr);
}

User *find_user(ulong ipaddr)
{
	GList *entry;
	User tmp_user;
	User *user = NULL;

	if (!user_list || !ipaddr)
		return NULL;

	memset(&tmp_user, 0, sizeof(User));
	tmp_user.ipaddr = ipaddr;
	entry = g_list_find_custom(user_list, &tmp_user, compare_by_ipaddr);
	if (entry) {
		g_assert(entry->data);
		user = entry->data;
	}

	return user;
}

bool del_user(Message * msg)
{

	GList *entry;
	User tmp_user;
	User *user;
	bool rc = false;

	if (!user_list || !msg)
		return rc;

	memset(&tmp_user, 0, sizeof(User));
	tmp_user.ipaddr = msg->fromAddr;
	pthread_mutex_lock(&user_list_mutex);
	entry = g_list_find_custom(user_list, &tmp_user, compare_by_ipaddr);
	if (!entry) {
		rc = false;
	} else {
		user = entry->data;
		free_user_data(user);
		user_list = g_list_remove_link(user_list, entry);
		g_list_free(entry);
		rc = true;
	}
	pthread_mutex_unlock(&user_list_mutex);

	return rc;
}

User *update_user(Message * msg)
{
	GList *entry;
	User tmp_user;
	User *user = NULL;

	if (!user_list || !msg)
		return NULL;

	memset(&tmp_user, 0, sizeof(User));
	tmp_user.ipaddr = msg->fromAddr;
	pthread_mutex_lock(&user_list_mutex);
	entry = g_list_find_custom(user_list, &tmp_user, compare_by_ipaddr);
	if (entry) {
		user = entry->data;
		fill_user_info(msg, user);
	}
	pthread_mutex_unlock(&user_list_mutex);

	return user;
}

guint get_online_users() {
	return g_list_length(user_list);
}