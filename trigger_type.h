/* -*- linux-c -*-
 *
 * Definition of a trigger
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

#ifndef _TRIGGER_TYPE_H_
#define _TRIGGER_TYPE_H_

#include <glib.h>
#include "state.h"

struct trigger_state {
	gboolean success;
	gint noof_available;
	struct state *state;
};

struct trigger_pattern {
	guint32 value;
	guint32 mask;
};

struct trigger_timed_value {
	struct trigger_timed_value *next;

	gint delay;
	guint32 value;
};

struct trigger {
	gint delay; /* In samples */
	gint noof_triggers;
	struct sump_trigger **triggers;
};

struct trigger_pattern trigger_pattern_make(struct signal_def *signal,
					    guint32 value);

struct trigger_pattern trigger_pattern_merge(
	struct trigger_pattern a,
	struct trigger_pattern b);

struct trigger *trigger_make_pattern_trigger(
	struct trigger_state *trigger_state,
	struct trigger_pattern pattern);

struct trigger *trigger_add_time_delay(struct state *state,
				       struct trigger_state *trigger_state,
				       struct trigger *trigger,
				       gdouble delay);

struct trigger *trigger_add_sample_delay(struct trigger_state *trigger_state,
					 struct trigger *trigger,
					 gint delay);

struct trigger_timed_value *trigger_make_timed_value(
	struct state *state,
	guint32 value, gdouble delay);

struct trigger_timed_value *trigger_make_delayed_value(
	guint32 value, gint delay);

struct trigger *trigger_make_timed_trigger(struct trigger_state *trigger_state,
					   struct signal_def *signal,
					   struct trigger_timed_value *values);

void trigger_state_init(struct state *state,
			struct trigger_state *trigger_state);

struct trigger *trigger_activate(struct trigger *trigger,
				 gint level, gboolean start);

/* triggers is a list of struct trigger * */
void trigger_activate_sequential_list(GList *triggers);
void trigger_activate_parallel_list(GList *triggers);

#endif /* _TRIGGER_TYPE_H_ */
