/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService PROTECT functions.
 *
 */

#include "atheme.h"
#include "chanserv.h"
#include "op_core.h"

DECLARE_MODULE_V1
(
	"chanserv/protect", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

void (*cmd_op_multiple)(sourceinfo_t *si, bool opping, const op_cmddesc_t *cmd, int parc, char *parv[]);

static void cs_cmd_protect(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_deprotect(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_protect = { "PROTECT", N_("Gives the channel protection flag to a user."),
                        AC_NONE, 2, cs_cmd_protect, { .path = "cservice/protect" } };
command_t cs_deprotect = { "DEPROTECT", N_("Removes channel protection flag from a user."),
                        AC_NONE, 2, cs_cmd_deprotect, { .path = "cservice/protect" } };

static op_cmddesc_t desc = {
	.flag      = CA_USEPROTECT,
	.flag_self = 0,
	.respect_secure = true,
	.mode_letter  = '\0',
	.mode_cstatus = CSTATUS_PROTECT,
	.cmd_op      = "PROTECT",
	.cmd_deop    = "DEPROTECT",
	.notify_op   = N_("You have been set as protected on %s by %s"),
	.notify_deop = N_("You have been unset as protected on %s by %s"),
	.result_op   = N_("\2%s\2 has been set as protected on \2%s\2."),
	.result_deop = N_("\2%s\2 has been unset as protected on \2%s\2.")
};

void _modinit(module_t *m)
{
	if (ircd != NULL && !ircd->uses_protect)
	{
		slog(LG_INFO, _("Module %s requires protect support, refusing to load."), m->name);
		m->mflags = MODTYPE_FAIL;
		return;
	}

	MODULE_TRY_REQUEST_SYMBOL(m, cmd_op_multiple, "chanserv/op_core", "cmd_op_multiple");

	desc.mode_letter = ircd->protect_mchar[1];

	service_named_bind_command("chanserv", &cs_protect);
	service_named_bind_command("chanserv", &cs_deprotect);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("chanserv", &cs_protect);
	service_named_unbind_command("chanserv", &cs_deprotect);
}

static void cs_cmd_protect(sourceinfo_t *si, int parc, char *parv[])
{
	cmd_op_multiple(si, true, &desc, parc, parv);
}

static void cs_cmd_deprotect(sourceinfo_t *si, int parc, char *parv[])
{
	cmd_op_multiple(si, false, &desc, parc, parv);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
