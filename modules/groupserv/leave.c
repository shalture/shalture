/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the GroupServ HELP command.
 *
 */

#include "atheme.h"
#include "groupserv.h"

DECLARE_MODULE_V1
(
	"groupserv/leave", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Shaltúre developers <https://github.com/shalture>"
);

static void gs_cmd_leave(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_leave = { "LEAVE", N_("Leave a group."), AC_AUTHENTICATED, 2, gs_cmd_leave, { .path = "groupserv/leave" } };

static void gs_cmd_leave(sourceinfo_t *si, int parc, char *parv[])
{
	mygroup_t *mg;

	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "LEAVE");
		command_fail(si, fault_needmoreparams, _("Syntax: LEAVE <!groupname>"));
		return;
	}

	if (!(mg = mygroup_find(parv[0])))
	{
		command_fail(si, fault_alreadyexists, _("Group \2%s\2 does not exist."), parv[0]);
		return;
	}

	if (!groupacs_sourceinfo_has_flag(mg, si, 0))
	{
		command_fail(si, fault_nosuch_target, _("You are not a member of group \2%s\2."), parv[0]);
		return;
	}

	groupacs_delete(mg, entity(si->smu));
	command_success_nodata(si, _("You are not longer a member of group \2%s\2."), entity(mg)->name);
}

void _modinit(module_t *m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_leave);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("groupserv", &gs_leave);
}
