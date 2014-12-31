/*
 * Copyright (c) 2014 Janik Kleinhoff
 * Rights to this code are documented in doc/LICENSE.
 *
 * Improved K-lines on individual IRCCloud users.
 *
 * Bans IRCCloud users as ?id12345@*.irccloud.com
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"contrib/banmask_irccloud", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Shaltúre developers <https://github.com/shalture>"
);

#define IRCCLOUD_TAIL ".irccloud.com"
#define TAIL_LEN      (sizeof(IRCCLOUD_TAIL) - 1)

static void banmask_hook(hook_user_get_banmask_t *hdata)
{
	size_t len      = strlen(hdata->u->host);

	if (len >= TAIL_LEN)
	{
		if (!strcmp(hdata->u->host + len - TAIL_LEN, IRCCLOUD_TAIL))
		{
			mowgli_strlcpy(hdata->host, "*.irccloud.com", HOSTLEN);
			mowgli_strlcpy(hdata->user, hdata->u->user, USERLEN);

			if (hdata->user[0] != '\0')
				hdata->user[0] = '?';
		}
	}
}

void _modinit(module_t *m)
{
	hook_add_event("user_get_banmask");
	hook_add_user_get_banmask(banmask_hook);
}

void _moddeinit(module_unload_intent_t intent)
{
	hook_del_user_get_banmask(banmask_hook);
}
