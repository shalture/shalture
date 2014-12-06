/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains common code for the CService OP, VOICE, etc. functions.
 *
 */

#include "atheme.h"
#include "chanserv.h"
#include "op_core.h"

DECLARE_MODULE_V1
(
	"chanserv/op_core", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

void _modinit(module_t *m)
{
}

void _moddeinit(module_unload_intent_t intent)
{
}

void cmd_op_multiple(sourceinfo_t *si, bool opping, const op_cmddesc_t *cmd, int parc, char *parv[])
{
	char *chan = parv[0];
	char *nick = parv[1];
	mychan_t *mc;
	user_t *tu;
	chanuser_t *cu;
	char *nicks;
	bool op;
	mowgli_list_t op_actions = {0};
	mowgli_node_t *n;

	if (!parv[0])
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, opping ? cmd->cmd_op : cmd->cmd_deop);
		command_fail(si, fault_needmoreparams, _("Syntax: %s <#channel> [nickname] [...]"), opping ? cmd->cmd_op : cmd->cmd_deop);
		return;
	}

	mc = mychan_find(chan);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), chan);
		return;
	}

	if (metadata_find(mc, "private:close:closer"))
	{
		command_fail(si, fault_noprivs, _("\2%s\2 is closed."), chan);
		return;
	}

	nicks = (!nick ? strdup(si->su->nick) : strdup(nick));
	prefix_action_set_all(&op_actions, opping, nicks);
	free(nicks);

	if (cmd->flag_self && MOWGLI_LIST_LENGTH(&op_actions) == 1)
	{
		/* allow opping only self if the user has the necessary flag */
		struct prefix_action *act = op_actions.head->data;
		nick = act->nick;
		tu = user_find_named(nick);

		if (!(chanacs_source_has_flag(mc, si, cmd->flag) ||
					(tu && tu == si->su && chanacs_source_has_flag(mc, si, cmd->flag_self))))
		{
			command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
			return;
		}
	}
	else if (!chanacs_source_has_flag(mc, si, cmd->flag))
	{
		command_fail(si, fault_noprivs, STR_NOT_AUTHORIZED);
		return;
	}

	MOWGLI_LIST_FOREACH(n, op_actions.head)
	{
		struct prefix_action *act = n->data;
		nick = act->nick;
		op = act->en;

		/* figure out who we're going to op */
		if (!(tu = user_find_named(nick)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not online."), nick);
			continue;
		}

		if (!op && is_service(tu))
		{
			command_fail(si, fault_noprivs, _("\2%s\2 is a network service; you cannot reduce their channel status."), tu->nick);
			continue;
		}

		/* SECURE check; we can skip this if deopping or sender == target, because we already verified */
		if (cmd->respect_secure && op && (si->su != tu) && (mc->flags & MC_SECURE) && !chanacs_user_has_flag(mc, tu, cmd->flag) && !chanacs_user_has_flag(mc, tu, cmd->flag_self))
		{
			command_fail(si, fault_noprivs, _("\2%s\2 has the SECURE option enabled, and \2%s\2 does not have appropriate access."), mc->name, tu->nick);
			continue;
		}

		cu = chanuser_find(mc->chan, tu);
		if (!cu)
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not on \2%s\2."), tu->nick, mc->name);
			continue;
		}

		modestack_mode_param(chansvs.nick, mc->chan, op ? MTYPE_ADD : MTYPE_DEL, cmd->mode_letter, CLIENT_NAME(tu));
		if (op)
			cu->modes |= cmd->mode_cstatus;
		else
			cu->modes &= ~(cmd->mode_cstatus);

		if (si->c == NULL && tu != si->su)
			change_notify(chansvs.nick, tu, _(op ? cmd->notify_op : cmd->notify_deop), mc->name, get_source_name(si));

		logcommand(si, CMDLOG_DO, "%s: \2%s!%s@%s\2 on \2%s\2", op ? cmd->cmd_op : cmd->cmd_deop, tu->nick, tu->user, tu->vhost, mc->name);
		if (si->su == NULL || !chanuser_find(mc->chan, si->su))
			command_success_nodata(si, _(op ? cmd->result_op : cmd->result_deop), tu->nick, mc->name);
	}

	prefix_action_clear(&op_actions);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
