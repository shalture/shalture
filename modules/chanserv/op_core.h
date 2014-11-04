/*
 * Copyright (c) 2014 Janik Kleinhoff
 * Rights to this code are documented in doc/LICENSE.
 *
 * This is the interface for the abstracted op/voice/... multiple people
 * command as implemented in op_core.c, to be used by commands acting like
 * chanserv/op and similar.
 */

#ifndef CSOP_CORE_H
#define CSOP_CORE_H

typedef struct {
	unsigned int flag;
	unsigned int flag_self;
	bool respect_secure;
	char mode_letter;
	unsigned int mode_cstatus;
	const char *cmd_op;
	const char *cmd_deop;
	const char *notify_op;
	const char *notify_deop;
	const char *result_op;
	const char *result_deop;
} op_cmddesc_t;

#endif /* !CSOP_CORE_H */
