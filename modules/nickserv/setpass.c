/*
 * Copyright (c) 2007 Atheme Development Group
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the NickServ SETPASS function.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/setpass", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void clear_setpass_key(user_t *u);
static void ns_cmd_setpass(sourceinfo_t *si, int parc, char *parv[]);
static void show_setpass(hook_user_req_t *hdata);
static void check_expire(hook_expiry_req_t *hdata);
static const char *get_setpass_key(myuser_t *mu);

command_t ns_setpass = { "SETPASS", N_("Changes a password using an authcode."), AC_NONE, 3, ns_cmd_setpass, { .path = "nickserv/setpass" } };

void _modinit(module_t *m)
{
	hook_add_event("user_identify");
	hook_add_user_identify(clear_setpass_key);
	hook_add_event("user_info");
	hook_add_user_info(show_setpass);
	hook_add_event("user_check_expire");
	hook_add_user_check_expire(check_expire);
	service_named_bind_command("nickserv", &ns_setpass);
}

void _moddeinit(module_unload_intent_t intent)
{
	hook_del_user_identify(clear_setpass_key);
	hook_del_user_info(show_setpass);
	hook_del_user_check_expire(check_expire);
	service_named_unbind_command("nickserv", &ns_setpass);
}

static void ns_cmd_setpass(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	const char *stored_key;
	char *nick = parv[0];
	char *key = parv[1];
	char *password = parv[2];

	if (!nick || !key || !password)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SETPASS");
		command_fail(si, fault_needmoreparams, _("Syntax: SETPASS <account> <key> <newpass>"));
		return;
	}

	if (strchr(password, ' '))
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SETPASS");
		command_fail(si, fault_badparams, _("Syntax: SETPASS <account> <key> <newpass>"));
		return;
	}

	if (!(mu = myuser_find(nick)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), nick);
		return;
	}

	if (si->smu == mu)
	{
		command_fail(si, fault_already_authed, _("You are logged in and can change your password using the SET PASSWORD command."));
		return;
	}

	if (strlen(password) >= PASSLEN)
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SETPASS");
		command_fail(si, fault_badparams, _("Registration passwords may not be longer than \2%d\2 characters."), PASSLEN - 1);
		return;
	}

	if (!strcasecmp(password, entity(mu)->name))
	{
		command_fail(si, fault_badparams, _("You cannot use your nickname as a password."));
		command_fail(si, fault_badparams, _("Syntax: SETPASS <account> <key> <newpass>"));
		return;
	}

	stored_key = get_setpass_key(mu);
	if (stored_key != NULL && crypt_verify_password(key, stored_key) != NULL)
	{
		logcommand(si, CMDLOG_SET, "SETPASS: \2%s\2", entity(mu)->name);
		set_password(mu, password);
		metadata_delete(mu, "private:setpass:key");
		metadata_delete(mu, "private:sendpass:sender");
		metadata_delete(mu, "private:sendpass:timestamp");


		command_success_nodata(si, _("The password for \2%s\2 has been changed to \2%s\2."), entity(mu)->name, password);

		return;
	}

	if (stored_key != NULL)
	{
		logcommand(si, CMDLOG_SET, "failed SETPASS (invalid key)");
	}
	command_fail(si, fault_badparams, _("Verification failed. Invalid key for \2%s\2."),
		entity(mu)->name);

	return;
}

static void clear_setpass_key(user_t *u)
{
	myuser_t *mu = u->myuser;

	if (!metadata_find(mu, "private:setpass:key"))
		return;

	metadata_delete(mu, "private:setpass:key");
	metadata_delete(mu, "private:sendpass:sender");
	metadata_delete(mu, "private:sendpass:timestamp");

	notice(nicksvs.nick, u->nick, "Warning: SENDPASS had been used to mail you a password recovery "
		"key. Since you have identified, that key is no longer valid.");
}

static void show_setpass(hook_user_req_t *hdata)
{
	if (has_priv(hdata->si, PRIV_USER_AUSPEX))
	{
		if (get_setpass_key(hdata->mu) != NULL)
			command_success_nodata(hdata->si, "%s has an active password reset key", entity(hdata->mu)->name);

		metadata_t *md;
		char strfbuf[BUFSIZE];

		if ((md = metadata_find(hdata->mu, "private:sendpass:sender")) != NULL)
		{
			const char *sender = md->value;
			time_t ts;
			struct tm tm;

			md = metadata_find(hdata->mu, "private:sendpass:timestamp");
			ts = md != NULL ? atoi(md->value) : 0;

			tm = *localtime(&ts);
			strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, &tm);

			command_success_nodata(hdata->si, _("%s was \2SENDPASSED\2 by %s on %s"), entity(hdata->mu)->name, sender, strfbuf);
		}
	}
}

/* Retrieve the (crypted) setpass key for an account, or NULL if none
 * Side effects: Old keys are automatically expired
 */
static const char *get_setpass_key(myuser_t *mu)
{
	metadata_t *md_key, *md_ts;
	if ((md_key = metadata_find(mu, "private:setpass:key")) == NULL)
		return NULL;

	if ((md_ts = metadata_find(mu, "private:sendpass:timestamp")) != NULL &&
			nicksvs.setpass_expiry > 0 && (unsigned int)(CURRTIME - atoi(md_ts->value)) >= nicksvs.setpass_expiry)
	{
		slog(LG_INFO, "SETPASS:EXPIRE: \2%s\2", entity(mu)->name);
		metadata_delete(mu, "private:setpass:key");
		metadata_delete(mu, "private:sendpass:sender");
		metadata_delete(mu, "private:sendpass:timestamp");
		return NULL;
	}

	return md_key->value;
}

static void check_expire(hook_expiry_req_t *hdata)
{
	/* we call get_setpass_key() for the side effect of expiring
	 * stale keys every once in a while */
	get_setpass_key(hdata->data.mu);
}
