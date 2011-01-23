/* -*- linux-c -*-
 *
 * Pattern handling functions
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
#include <math.h>
#include <string.h>
#include "trigger_type.h"

struct trigger_pattern trigger_pattern_merge(
	struct trigger_pattern a,
	struct trigger_pattern b)
{
	struct trigger_pattern r;

	r.value = a.value | b.value;
	r.mask = a.mask | b.mask;
	return r;
}

struct trigger_pattern trigger_pattern_make(struct signal_def *signal,
					    guint32 value)
{
	struct trigger_pattern r;

	r.value = state_signal_value(signal, value);
	r.mask = state_signal_mask(signal);
	return r;
}

static struct sump_trigger *trigger_allocate_trigger(
	struct trigger_state *state)
{
	if (state->noof_available == 0)
		return NULL;

	return &state->state->triggers[state->noof_available-- - 1];
}

struct trigger *trigger_make_pattern_trigger(
	struct trigger_state *trigger_state,
	struct trigger_pattern pattern)
{
	struct trigger *r = g_malloc(sizeof(*r));

	r->noof_triggers = 1;
 	r->triggers = g_malloc(r->noof_triggers * sizeof(*r->triggers));
	r->triggers[0] = trigger_allocate_trigger(trigger_state);
	if (r->triggers[0] == NULL) {
			fprintf(stderr,	"Error: Hardware triggers exhausted\n");
			trigger_state->success = FALSE;
			return r;
	}

	r->triggers[0]->mask = pattern.mask;
	r->triggers[0]->values = pattern.value;
	r->triggers[0]->channel = 0;
	r->triggers[0]->serial = FALSE;
	return r;
}

struct trigger *trigger_add_time_delay(struct state *state,
				       struct trigger_state *trigger_state,
				       struct trigger *trigger,
				       gdouble delay)
{
	gdouble sample_period = 1.0 / (double)state->sample_rate;

	trigger->delay = state->sample_rate * delay;

	if (trigger->delay > MAX_SAMPLE_DELAY) {
		fprintf(stderr,
			"Error: Time delay of %es exceeds hardware"
			" capabilities.\n",
			delay);
		trigger_state->success = FALSE;
		trigger->delay = MAX_SAMPLE_DELAY;
	} else if (fabs(trigger->delay * sample_period - delay) / delay > 0.1) {
		fprintf(stderr,
			"Warning: The time delay of %es, when converted to "
			"samples as used by the hardware, differs from the "
			"desired delay by more than 10%%\n",
			delay);
	}

	return trigger;
}

struct trigger *trigger_add_sample_delay(struct trigger_state *trigger_state,
					 struct trigger *trigger, gint delay)
{
	if (delay > MAX_SAMPLE_DELAY) {
		fprintf(stderr,
			"Error: Sample delay of %d exceeds hardware "
			" capabilities\n",
			delay);
		delay = MAX_SAMPLE_DELAY;
		trigger_state->success = FALSE;
	}
	trigger->delay = delay;
	return trigger;
}

struct trigger_timed_value *trigger_make_timed_value(
	struct state *state,
	guint32 value, gdouble delay)
{
	struct trigger_timed_value *r = g_malloc(sizeof(*r));
	gdouble sample_period = 1.0 / (double)state->sample_rate;

	r->delay = state->sample_rate * delay;
	r->value = value;
	if (fabs(r->delay * sample_period - delay) / delay > 0.1) {
		fprintf(stderr,
			"Warning: The time delay of %es, when converted to "
			"samples as used by the hardware, differs from the "
			"desired delay by more than 10%%\n",
			delay);
	}

	return r;
}

struct trigger_timed_value *trigger_make_delayed_value(
	guint32 value, gint delay)
{
	struct trigger_timed_value *r = g_malloc(sizeof(*r));

	r->delay = delay;
	r->value = value;
	return r;
}

struct trigger *trigger_make_timed_trigger(struct trigger_state *trigger_state,
					   struct signal_def *signal,
					   struct trigger_timed_value *values)
{
	struct trigger *r = g_malloc(sizeof(*r));
	gint bit = 0;

	r->noof_triggers = signal->noof_bits;
 	r->triggers = g_malloc(r->noof_triggers * sizeof(*r->triggers));

	for (GList *i = signal->channels;
	     i != NULL;
	     i = g_list_next(i), bit++) {
		gint channel = GPOINTER_TO_INT(i->data);
		struct sump_trigger *trigger =
			trigger_allocate_trigger(trigger_state);
		gint sample = 0;

		r->triggers[bit] = trigger;
		if (trigger == NULL) {
			fprintf(stderr,
				"Error: Pattern sequence trigger for signal %s "
				"requires more than the available number of "
				"hardware triggers\n",
				signal->name);
			trigger_state->success = FALSE;
			return r;
		}
		trigger->channel = channel;
		trigger->serial = TRUE;
		for (struct trigger_timed_value *value = values;
		     value != NULL;
		     value = value->next) {
			for (gint index = 0; index < value->delay; index++) {
				trigger->mask |= (1 << (sample + index));
				if (value->value & (1 << bit))
					trigger->values |= (1 << (sample + index));
			}
			sample += value->delay;
		}
		if (sample > 32) {
			fprintf(stderr,
				"Error: Pattern sequence trigger for signal "
				"%s describes a sequence of patterns extending"
				" for more than 32 samples\n",
				signal->name);
			trigger_state->success = FALSE;
			return r;
		}
	}
	return r;
}

void trigger_state_init(struct state *state,
			struct trigger_state *trigger_state)
{
	trigger_state->state = state;
	trigger_state->noof_available = NOOF_TRIGGERS;
	trigger_state->success = TRUE;
	memset(state->triggers, 0, sizeof(*state->triggers) * NOOF_TRIGGERS);

}

struct trigger *trigger_activate(struct trigger *trigger,
				 gint level, gboolean start)
{
	for (gint i = 0; i < trigger->noof_triggers; i++) {
		trigger->triggers[i]->level = level;
		trigger->triggers[i]->start = start;
		trigger->triggers[i]->delay = trigger->delay;
	}
	return trigger;
}


/* triggers is a list of struct trigger * */
void trigger_activate_sequential_list(GList *triggers)
{
	gint level = 0;
	for (GList *i = triggers; i != NULL; i = g_list_next(i), level++) {
		struct trigger *trigger = i->data;

		trigger_activate(trigger, level, g_list_next(i) == NULL);
	}

}

/* triggers is a list of struct trigger * */
void trigger_activate_parallel_list(GList *triggers)
{
	for (GList *i = triggers; i != NULL; i = g_list_next(i)) {
		struct trigger *trigger = i->data;

		trigger_activate(trigger, 0, g_list_next(i) == NULL);
	}
}

