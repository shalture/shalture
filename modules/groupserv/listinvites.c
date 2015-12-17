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
	"groupserv/listinvites", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Shaltúre developers <https://github.com/shalture>"
);

static void gs_cmd_listinvites(sourceinfo_t *si, int parc, char *parv[]);

command_t gs_listinvites = { "LISTINVITES", N_("List groups with pending invitations."), AC_AUTHENTICATED, 2, gs_cmd_listinvites, { .path = "groupserv/listinvites" } };

static void gs_cmd_listinvites(sourceinfo_t *si, int parc, char *parv[])
{
	myentity_t *mt;
	mowgli_node_t *n;
	myentity_iteration_state_t state;
	char buf[BUFSIZE];
	struct tm tm;
	mygroup_t *mg;
	metadata_t *md;
	groupinvite_t *gi;

	/* Convert legacy invites */
	groupinvite_convert(si->smu, strshare_get(si->service->nick), 0);

	command_success_nodata(si, _("Groups you are invited to:"));

	MYENTITY_FOREACH_T(mt, &state, ENT_GROUP)
	{
		mg = group(mt);
		continue_if_fail(mt != NULL);
		continue_if_fail(mg != NULL);

		MOWGLI_ITER_FOREACH(n, mg->invites.head)
		{
			gi = n->data;

			if (gi->mg == mg && gi->mt == entity(si->smu)) {
				tm = *localtime(&gi->invite_ts);
				strftime(buf, BUFSIZE, TIME_FORMAT, &tm);

				command_success_nodata(si, "group:\2%s\2 inviter:\2%s\2 (%s)",
						entity(gi->mg)->name, gi->inviter, buf);
			}
		}
	}

	command_success_nodata(si, "End of list.");
	logcommand(si, CMDLOG_GET, "LISTINVITES");

}

void _modinit(module_t *m)
{
	use_groupserv_main_symbols(m);

	service_named_bind_command("groupserv", &gs_listinvites);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("groupserv", &gs_listinvites);
}
