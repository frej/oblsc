/* -*- linux-c -*-
 *
 * Trigger compiler
 *
 * This file is part of oblsc.
 *
 * Copyright (C) 2010-2011 Frej Drejhammar <frej.drejhammar@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#include <stdio.h>
#include <string.h>
#include "trigger.h"
#include "trigger_type.h"
#include "trigger_parse.h"
#include "trigger_lex.h"

extern void *trigger_lex_setup_string(char *string);
extern void trigger_lex_release_string(void *buffer);

int yyparse(yyscan_t scanner,
	    struct state *state,
	    struct trigger_state *trigger_state);

/* Will clear the triggers not used */
gboolean trigger_compile(struct state *state)
{
	gboolean status = FALSE;
	yyscan_t scanner;
	struct trigger_state trigger_state;
	YY_BUFFER_STATE buffer;

	trigger_state_init(state, &trigger_state);

	if (state->trigger_spec == NULL) {
		for (gint i = 0; i < NOOF_TRIGGERS; i++)
			state->triggers[i].start = TRUE;
		return TRUE;
	}

	yylex_init_extra(state, &scanner);
	buffer = yy_scan_string(state->trigger_spec, scanner);

	if (yyparse(scanner, state, &trigger_state) || !trigger_state.success)
		goto error;
	status = TRUE;
error:
	yy_delete_buffer(buffer, scanner);
	yylex_destroy(scanner);
	return status;
}

