/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService OP functions.
 *
 */

#include "atheme.h"
#include "chanserv.h"
#include "op_core.h"

DECLARE_MODULE_V1
(
	"chanserv/op", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

void (*cmd_op_multiple)(sourceinfo_t *si, bool opping, const op_cmddesc_t *cmd, int parc, char *parv[]);

static void cs_cmd_op(sourceinfo_t *si, int parc, char *parv[]);
static void cs_cmd_deop(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_op = { "OP", N_("Gives channel ops to a user."),
                        AC_NONE, 2, cs_cmd_op, { .path = "cservice/op_voice" } };
command_t cs_deop = { "DEOP", N_("Removes channel ops from a user."),
                        AC_NONE, 2, cs_cmd_deop, { .path = "cservice/op_voice" } };

static op_cmddesc_t desc = {
	.flag      = CA_OP,
	.flag_self = CA_AUTOOP,
	.respect_secure = true,
	.mode_letter  = 'o',
	.mode_cstatus = CSTATUS_OP,
	.cmd_op      = "OP",
	.cmd_deop    = "DEOP",
	.notify_op   = N_("You have been opped on %s by %s"),
	.notify_deop = N_("You have been deopped on %s by %s"),
	.result_op   = N_("\2%s\2 has been opped on \2%s\2."),
	.result_deop = N_("\2%s\2 has been deopped on \2%s\2.")
};

void _modinit(module_t *m)
{
	MODULE_TRY_REQUEST_SYMBOL(m, cmd_op_multiple, "chanserv/op_core", "cmd_op_multiple");

	service_named_bind_command("chanserv", &cs_op);
	service_named_bind_command("chanserv", &cs_deop);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_named_unbind_command("chanserv", &cs_op);
	service_named_unbind_command("chanserv", &cs_deop);
}

static void cs_cmd_op(sourceinfo_t *si, int parc, char *parv[])
{
	cmd_op_multiple(si, true, &desc, parc, parv);
}

static void cs_cmd_deop(sourceinfo_t *si, int parc, char *parv[])
{
	cmd_op_multiple(si, false, &desc, parc, parv);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
