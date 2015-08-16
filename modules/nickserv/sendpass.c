/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService SENDPASS function.
 *
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"nickserv/sendpass", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Shaltúre developers <https://github.com/shalture>"
);

static void ns_cmd_sendpass(sourceinfo_t *si, int parc, char *parv[]);
static void clear_setpass_key(user_t *u);
static void ns_cmd_setpass(sourceinfo_t *si, int parc, char *parv[]);
static void show_setpass(hook_user_req_t *hdata);
static void check_expire(hook_expiry_req_t *hdata);
static const char *get_setpass_key(myuser_t *mu);
static void sendpass_paranoid_success(sourceinfo_t *si, myuser_t *mu);
static void sendpass_param_error(sourceinfo_t *si, cmd_faultcode_t fault, bool has_priv);
static int c_ci_sendpass_access(mowgli_config_file_entry_t *ce);

command_t ns_sendpass = { "SENDPASS", N_("Reset registration passwords."), PRIV_USER_SENDPASS, 2, ns_cmd_sendpass, { .path = "nickserv/sendpass" } };
command_t ns_setpass  = { "SETPASS", N_("Verify a password reset."), AC_NONE, 3, ns_cmd_setpass, { .path = "nickserv/setpass" } };

void _modinit(module_t *m)
{
	hook_add_event("user_identify");
	hook_add_user_identify(clear_setpass_key);
	hook_add_event("user_info");
	hook_add_user_info(show_setpass);
	hook_add_event("user_check_expire");
	hook_add_user_check_expire(check_expire);

	service_named_bind_command("nickserv", &ns_setpass);
	service_named_bind_command("nickserv", &ns_sendpass);
	add_conf_item("SENDPASS_ACCESS", &nicksvs.me->conf_table, c_ci_sendpass_access);
}

void _moddeinit(module_unload_intent_t intent)
{
	hook_del_user_identify(clear_setpass_key);
	hook_del_user_info(show_setpass);
	hook_del_user_check_expire(check_expire);
	service_named_unbind_command("nickserv", &ns_setpass);
	service_named_unbind_command("nickserv", &ns_sendpass);
	del_conf_item("SENDPASS_ACCESS", &nicksvs.me->conf_table);
}

enum specialoperation
{
	op_none,
	op_force,
	op_clear,
	op_paranoid
};

enum
{
	SENDPASS_ACCESS_STAFF,
	SENDPASS_ACCESS_USER,
	SENDPASS_ACCESS_USER_PARANOID
} sendpass_access = SENDPASS_ACCESS_STAFF;

static void ns_cmd_sendpass(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu = NULL;
	char *name    = parv[0];
	char *special = parv[1];
	enum specialoperation mode = op_none;
	bool has_sendpass_priv = has_priv(si, PRIV_USER_SENDPASS);
	bool email_visible = has_priv(si, PRIV_USER_AUSPEX);
	/* whether to leak things like MARK, SOPER etc. */
	bool leaky_checks  = (sendpass_access == SENDPASS_ACCESS_STAFF);
	bool ismarked = false;
	hook_user_needforce_t needforce_hdata;
	char *key;

	if (!name)
	{
		sendpass_param_error(si, fault_needmoreparams, has_sendpass_priv);
		return;
	}

	if (!(mu = myuser_find_by_nick(name)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), name);
		return;
	}

	/* no point hiding the email if we don't normally do that anyway */
	if (mu != NULL && !(mu->flags & MU_HIDEMAIL))
		email_visible = true;

	/* people with access: check for FORCE/CLEAR */
	if (sendpass_access == SENDPASS_ACCESS_STAFF || has_sendpass_priv)
	{
		if (special)
		{
			if (!strcasecmp(special, "FORCE"))
				mode = op_force;
			else if (!strcasecmp(special, "CLEAR"))
				mode = op_clear;
			else
			{
				sendpass_param_error(si, fault_badparams, has_sendpass_priv);
				return;
			}
		}
	}
	/* paranoid mode: check for email
	 * If the email is visible, we can safely ignore the parameter.
	 */
	else if (sendpass_access == SENDPASS_ACCESS_USER_PARANOID && !email_visible)
	{
		if (!special)
		{
			sendpass_param_error(si, fault_needmoreparams, has_sendpass_priv);
			return;
		}
		/* Don't tell them the specified address was wrong to avoid leaking info */
		else if (strcasecmp(special, mu->email))
		{
			sendpass_paranoid_success(si, mu);
			return;
		}
		mode = op_paranoid;
	}
	/* normal user mode: check for superfluous param */
	else if (sendpass_access == SENDPASS_ACCESS_USER && special)
	{
		sendpass_param_error(si, fault_badparams, has_sendpass_priv);
		return;
	}

	if (mu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not verified."), entity(mu)->name);
		return;
	}

	if (mode == op_clear)
	{
		if (metadata_find(mu, "private:setpass:key"))
		{
			metadata_delete(mu, "private:setpass:key");
			metadata_delete(mu, "private:sendpass:sender");
			metadata_delete(mu, "private:sendpass:timestamp");
			logcommand(si, CMDLOG_ADMIN, "SENDPASS:CLEAR: \2%s\2", entity(mu)->name);
			command_success_nodata(si, _("The password change key for \2%s\2 has been cleared."), entity(mu)->name);
		}
		else
			command_fail(si, fault_nochange, _("\2%s\2 did not have a password change key outstanding."), entity(mu)->name);
		return;
	}

	if (leaky_checks && is_soper(mu) && !has_priv(si, PRIV_ADMIN))
	{
		logcommand(si, CMDLOG_ADMIN, "failed SENDPASS \2%s\2 (is SOPER)", entity(mu)->name);
		command_fail(si, fault_badparams, _("\2%s\2 belongs to a services operator; you need %s privilege to send the password."), name, PRIV_ADMIN);
		return;
	}

	needforce_hdata.si = si;
	needforce_hdata.mu = mu;
	needforce_hdata.allowed = 1;

	hook_call_user_needforce(&needforce_hdata);

	if (metadata_find(mu, "private:mark:setter") || !needforce_hdata.allowed)
	{
		ismarked = true;
		if (leaky_checks)
		{
			if (mode != op_force)
			{
				logcommand(si, CMDLOG_ADMIN, "failed SENDPASS \2%s\2 (marked)", entity(mu)->name);
				command_fail(si, fault_badparams, _("This operation cannot be performed on %s, because the account has been marked."), entity(mu)->name);
				if (has_priv(si, PRIV_MARK))
				{
					char cmdtext[NICKLEN + 20];
					snprintf(cmdtext, sizeof cmdtext, "SENDPASS %s FORCE", entity(mu)->name);
					command_fail(si, fault_badparams, _("Use %s to override this restriction."), cmdtext);
				}
				return;
			}
			else if (!has_priv(si, PRIV_MARK))
			{
				logcommand(si, CMDLOG_ADMIN, "failed SENDPASS \2%s\2 (marked)", entity(mu)->name);
				command_fail(si, fault_noprivs, STR_NO_PRIVILEGE, PRIV_MARK);
				return;
			}
		}
	}

	if (metadata_find(mu, "private:freeze:freezer"))
	{
		command_fail(si, fault_noprivs, _("%s has been frozen by the %s administration."), entity(mu)->name, me.netname);
		return;
	}

	if (MOWGLI_LIST_LENGTH(&mu->logins) > 0)
	{
		command_fail(si, fault_noprivs, _("This operation cannot be performed on %s, because someone is logged in to it."), entity(mu)->name);
		return;
	}

	if (metadata_find(mu, "private:setpass:key"))
	{
		if (mode == op_paranoid)
			sendpass_paranoid_success(si, mu);
		else
		{
			command_fail(si, fault_alreadyexists, _("\2%s\2 already has a password change key outstanding."), entity(mu)->name);
			if (has_sendpass_priv)
				command_fail(si, fault_alreadyexists, _("Use SENDPASS %s CLEAR to clear it so that a new one can be sent."), entity(mu)->name);
		}
		return;
	}

	if (ismarked)
	{
		wallops("%s sent the password for the \2MARKED\2 account %s.", get_oper_name(si), entity(mu)->name);
		if (has_priv(si, PRIV_MARK) || has_priv(si, PRIV_USER_AUSPEX))
			command_success_nodata(si, _("Overriding MARK on the account %s."), entity(mu)->name);
	}
	logcommand(si, CMDLOG_ADMIN, "SENDPASS: \2%s\2", name);

	key = random_string(12);
	metadata_add(mu, "private:sendpass:sender", get_oper_name(si));
	metadata_add(mu, "private:sendpass:timestamp", number_to_string(time(NULL)));

	if (!sendemail(si->su != NULL ? si->su : si->service->me, mu, EMAIL_SETPASS, mu->email, key))
	{
		if (mode == op_paranoid)
			sendpass_paranoid_success(si, mu);
		else
			command_fail(si, fault_emailfail, _("Email send failed."));
		free(key);
		return;
	}

	metadata_add(mu, "private:setpass:key", crypt_string(key, gen_salt()));
	free(key);

	if (mode == op_paranoid)
		sendpass_paranoid_success(si, mu);
	else if (email_visible)
		command_success_nodata(si, _("The password change key for \2%s\2 has been sent to \2%s\2."), entity(mu)->name, mu->email);
	else
		command_success_nodata(si, _("The password change key for \2%s\2 has been sent to the corresponding email address."), entity(mu)->name);
}

static void sendpass_param_error(sourceinfo_t *si, cmd_faultcode_t fault, bool has_priv)
{
	if (fault == fault_needmoreparams)
		command_fail(si, fault, STR_INSUFFICIENT_PARAMS, "SENDPASS");
	else
		command_fail(si, fault, STR_INVALID_PARAMS, "SENDPASS");

	if (sendpass_access == SENDPASS_ACCESS_STAFF)
		command_fail(si, fault, _("Syntax: SENDPASS <account> [FORCE|CLEAR]"));
	else if (has_priv)
		command_fail(si, fault, _("Syntax: SENDPASS <account> [CLEAR]"));
	else if (sendpass_access == SENDPASS_ACCESS_USER_PARANOID)
		command_fail(si, fault, _("Syntax: SENDPASS <account> <email>"));
	else
		command_fail(si, fault, _("Syntax: SENDPASS <account>"));
}

static void sendpass_paranoid_success(sourceinfo_t *si, myuser_t *mu)
{
	command_success_nodata(si, _("The password change key for \2%s\2 has been sent to the email address provided, assuming it matched the user's recorded email address and a reset key wasn't already active."), entity(mu)->name);
}

static int c_ci_sendpass_access(mowgli_config_file_entry_t *ce)
{
	if (ce->vardata == NULL)
	{
		conf_report_warning(ce, "no parameter for configuration option");
		return 0;
	}

	if (!strcasecmp(ce->vardata, "STAFF"))
	{
		sendpass_access = SENDPASS_ACCESS_STAFF;
		ns_sendpass.access = PRIV_USER_SENDPASS;
	}
	else if (!strcasecmp(ce->vardata, "USER"))
	{
		sendpass_access = SENDPASS_ACCESS_USER;
		ns_sendpass.access = AC_NONE;
	}
	else if (!strcasecmp(ce->vardata, "USER_PARANOID"))
	{
		sendpass_access = SENDPASS_ACCESS_USER_PARANOID;
		ns_sendpass.access = AC_NONE;
	}

	return 0;
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
		{
			metadata_t *md;
			char strfbuf[BUFSIZE];
			char buf[BUFSIZE];
			size_t buflen = 0;

			buf[0] = '\0';

			if ((md = metadata_find(hdata->mu, "private:sendpass:sender")) != NULL)
				buflen += snprintf(buf + buflen, sizeof(buf) - buflen, " by %s", md->value);
			if ((md = metadata_find(hdata->mu, "private:sendpass:timestamp")) != NULL)
			{
				time_t ts;
				struct tm tm;

				ts = atoi(md->value);
				tm = *localtime(&ts);
				strftime(strfbuf, sizeof strfbuf, TIME_FORMAT, &tm);

				buflen += snprintf(buf + buflen, sizeof(buf) - buflen, " on %s (%s ago)", strfbuf, time_ago(ts));
			}
			if (buf[0] != '\0')
				command_success_nodata(hdata->si, _("%s has an active \2SETPASS\2 key (sent%s)"), entity(hdata->mu)->name, buf);
			else
				command_success_nodata(hdata->si, _("%s has an active \2SETPASS\2 key"), entity(hdata->mu)->name);
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
		slog(LG_VERBOSE, "get_setpass_key(): expiring setpass key for account %s", entity(mu)->name);
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

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
