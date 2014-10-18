/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2008 Atheme Development Group
 * Copyright (c) 2014 Max Teufel
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for neephi.
 *
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/charybdis.h"
#include "protocol/neephi.h"

DECLARE_MODULE_V1("protocol/neephi", true, _modinit, NULL, PACKAGE_STRING, "Zohlai Development Group");

ircd_t neephi = {
	"neephi",					/* IRCd name */
	"$$",						/* TLD Prefix, used by Global. */
	true,						/* Whether or not we use IRCNet/TS6 UID */
	false,						/* Whether or not we use RCOMMAND */
	false,						/* Whether or not we support channel owners. */
	false,						/* Whether or not we support channel protection. */
	false,						/* Whether or not we support halfops. */
	false,						/* Whether or not we use P10 */
	false,						/* Whether or not we use vHosts. */
	CMODE_EXLIMIT | CMODE_PERM,			/* Oper-only cmodes */
	0,						/* Integer flag for owner channel flag. */
	0,						/* Integer flag for protect channel flag. */
	0,						/* Integer flag for halfops. */
	"+",						/* Mode we set for owner. */
	"+",						/* Mode we set for protect. */
	"+",						/* Mode we set for halfops. */
	PROTOCOL_CHARYBDIS,				/* Protocol type */
	CMODE_PERM,					/* Permanent cmodes */
	0,						/* Oper-immune cmode */
	"beIq",						/* Ban-like cmodes */
	'e',						/* Except mchar */
	'I',						/* Invex mchar */
	IRCD_CIDR_BANS | IRCD_HOLDNICK			/* Flags */
};

struct cmode_ neephi_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'r', CMODE_REGONLY},
  { 'z', CMODE_OPMOD  },
  { 'g', CMODE_FINVITE},
  { 'L', CMODE_EXLIMIT},
  { 'P', CMODE_PERM   },
  { 'F', CMODE_FTARGET},
  { 'Q', CMODE_DISFWD },

  /* following modes are added as extensions */
  { 'N', CMODE_NPC	},
  { 'S', CMODE_SSLONLY	},
  { 'O', CMODE_OPERONLY	},
  { 'A', CMODE_ADMINONLY},
  { 'c', CMODE_NOCOLOR	},
  { 'C', CMODE_NOCTCP	},
  { 'E', CMODE_NOKICKS	},
  { '\0', 0 }
};

struct cmode_ neephi_user_mode_list[] = {
  { 'I', UF_IMMUNE   },
  { 'a', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { 'D', UF_DEAF     },
  { 'S', UF_SERVICE  },
  { '\0', 0 }
};

void _modinit(module_t * m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/charybdis");

	mode_list = neephi_mode_list;
	user_mode_list = neephi_user_mode_list;

	ircd = &neephi;

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
