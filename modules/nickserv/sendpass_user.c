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
	"nickserv/sendpass_user", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void ns_cmd_sendpass(sourceinfo_t *si, int parc, char *parv[]);

command_t ns_sendpass = { "SENDPASS", N_("Email registration passwords."), AC_NONE, 2, ns_cmd_sendpass, { .path = "nickserv/sendpass_user" } };

void _modinit(module_t *m)
{
	MODULE_CONFLICT(m, "nickserv/sendpass")
	MODULE_TRY_REQUEST_DEPENDENCY(m, "nickserv/setpass");

	service_named_bind_command("nickserv", &ns_sendpass);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("nickserv", &ns_sendpass);
}

enum specialoperation
{
	op_none,
	op_clear
};

static void ns_cmd_sendpass(sourceinfo_t *si, int parc, char *parv[])
{
	myuser_t *mu;
	char *name = parv[0];
	char *email = parv[1];
	char *key;
	enum specialoperation op = op_none;
	bool ismarked = false;
	int is_valid_email = email ? validemail(email) : 0;
	int has_sendpass_priv = has_priv(si, PRIV_USER_SENDPASS);

	if (!name)
	{
wrong_syntax:
		if (has_sendpass_priv)
		{
			command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SENDPASS");
			command_fail(si, fault_needmoreparams, _("Syntax: SENDPASS <account> <email>"));
		}
		else
		{
			command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SENDPASS");
			command_fail(si, fault_badparams, _("Syntax: SENDPASS <account> [CLEAR]"));
		}
		return;
	}

	if (has_sendpass_priv && parc > 1)
	{
		if (!strcasecmp(parv[1], "CLEAR"))
			op = op_clear;
		else
			goto wrong_syntax;
	}
	else if (!has_sendpass_priv && (parc < 2 || !is_valid_email))
		goto wrong_syntax;

	if (!(mu = myuser_find_by_nick(name)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), name);
		return;
	}

	if (mu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_badparams, _("\2%s\2 is not verified."), entity(mu)->name);
		return;
	}

	if (metadata_find(mu, "private:mark:setter"))
	{
		ismarked = true;
		/* don't want to disclose this, so just go ahead... */
	}

	if (op == op_clear)
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

	if (MOWGLI_LIST_LENGTH(&mu->logins) > 0)
	{
		command_fail(si, fault_noprivs, _("This operation cannot be performed on %s, because someone is logged in to it."), entity(mu)->name);
		return;
	}

	if (metadata_find(mu, "private:freeze:freezer"))
	{
		command_success_nodata(si, _("%s has been frozen by the %s administration."), entity(mu)->name, me.netname);
		return;
	}

	if (metadata_find(mu, "private:setpass:key"))
	{
		logcommand(si, CMDLOG_ADMIN, "SENDPASS: \2%s\2 already has an active reset key. Ignoring request.", name);
		if (has_sendpass_priv)
		{
			command_fail(si, fault_alreadyexists, _("\2%s\2 already has a password change key outstanding."), entity(mu)->name);
			command_fail(si, fault_alreadyexists, _("Use SENDPASS %s CLEAR to clear it so that a new one can be sent."), entity(mu)->name);
			return;
		}
		/* The user gets the generic success message if they don't have PRIV_USER_SENDPASS */
	}
	else if (has_sendpass_priv || (is_valid_email && !strcasecmp(email, mu->email)))
	{
		key = random_string(12);
		if (sendemail(si->su != NULL ? si->su : si->service->me, mu, EMAIL_SETPASS, mu->email, key))
		{
			metadata_add(mu, "private:setpass:key", crypt_string(key, gen_salt()));
			logcommand(si, CMDLOG_ADMIN, "SENDPASS: \2%s\2 (change key)", name);
			if (ismarked)
				wallops("%s sent the password for the \2MARKED\2 account %s.", get_oper_name(si), entity(mu)->name);

			metadata_add(mu, "private:sendpass:sender", get_oper_name(si));
			metadata_add(mu, "private:sendpass:timestamp", number_to_string(time(NULL)));
		}
		else
			logcommand(si, CMDLOG_ADMIN, "SENDPASS: Could not send email for \2%s\2 to \2%s\2", entity(mu)->name, email);
		free(key);
	}

	command_success_nodata(si, _("The password change key for \2%s\2 has been sent to the email address provided, if it matched the one stored with the user and a reset key isn't already active."), entity(mu)->name);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
