/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the GroupServ INVITE command.
 *
 */

/* TODO:
 * We should probably add a way for the target user to remove pending invites
 */

#include "atheme.h"
#include "groupserv.h"

DECLARE_MODULE_V1
(
	"groupserv/invite", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Shaltúre developers <https://github.com/shalture>"
);

static void gs_cmd_invite(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_invite = { "INVITE", N_("Invites a user to a group."), AC_AUTHENTICATED, 2, gs_cmd_invite, { .path = "groupserv/invite" } };

static void gs_cmd_invite(sourceinfo_t *si, int parc, char *parv[])
{
	user_t *tu;
	mygroup_t *mg;
	myuser_t *mu;
	groupacs_t *ga;
	groupinvite_t *gi;
	metadata_t *md;
	char *group = parv[0];
	char *user = parv[1];
	char buf[BUFSIZE];
	service_t *svs;

	if (!group || !user)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "INVITE");
		command_fail(si, fault_needmoreparams, _("Syntax: INVITE <!group> <user>"));
		return;
	}

	if ((mg = mygroup_find(group)) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("The group \2%s\2 does not exist."), group);
		return;
	}

	if (!groupacs_sourceinfo_has_flag(mg, si, GA_INVITE))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	if ((mu = myuser_find_ext(user)) == NULL)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a registered account."), user);
		return;
	}

	if ((ga = groupacs_find(mg, entity(mu), 0, false)) != NULL)
	{
		command_fail(si, fault_badparams, _("\2%s\2 is already a member of \2%s\2."), user, group);
		return;
	}

	if (MU_NEVERGROUP & mu->flags)
	{
		command_fail(si, fault_noprivs, _("\2%s\2 does not wish to belong to any groups."), user);
		return;
	}

	/* Convert legacy invites */
	groupinvite_convert(mu, strshare_ref(entity(si->smu)->name), CURRTIME);

	if ((gi = groupinvite_find(mg, entity(mu))) != NULL)
	{
		command_fail(si, fault_badparams, _("\2%s\2 is already invited to this group."), user);
		return;
	}

	gi = groupinvite_add(mg, entity(mu), strshare_ref(entity(si->smu)->name), CURRTIME);

	tu = user_find_named(user);
	if (tu != NULL && tu->myuser == mu) {
		myuser_notice(si->service->nick, mu, "You have been invited to the group \2%s\2.", group);
	}
	else {
		if ((svs = service_find("memoserv")) != NULL)
		{
			snprintf(buf, BUFSIZE, "%s [auto memo] You have been invited to the group \2%s\2.", user, group);

			command_exec_split(svs, si, "SEND", buf, svs->commands);
		}
		else
		{
			myuser_notice(si->service->nick, mu, "You have been invited to the group \2%s\2.", group);
		}
	}

	logcommand(si, CMDLOG_SET, "INVITE: \2%s\2 \2%s\2", group, user);
	command_success_nodata(si, _("\2%s\2 has been invited to \2%s\2"), user, group);
}

void _modinit(module_t *m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_invite);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("groupserv", &gs_invite);
}

