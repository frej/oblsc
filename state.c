/* -*- linux-c -*-
 *
 * Functions for updating and querying the state
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
#include <stdlib.h>
#include "state.h"

void state_add_signal(struct state *state, gchar *name, GList *channels)
{
	struct signal_def *d = g_malloc(sizeof(*d));

	d->name = g_strdup(name);
	d->channels = channels;
	d->index = state->noof_signals++;
	d->noof_bits = g_list_length(channels);
	d->mask = 0;

	for (GList *i = d->channels; i != NULL; i = g_list_next(i)) {
		int channel = GPOINTER_TO_INT(i->data);

		if (channel < 0 || channel > 31) {
			fprintf(stderr, "Unsupported channel number %d\n",
				channel);
			exit(1);
		}
		d->mask |= (1 << channel);
		state->channels_in_use |= (1 << channel);
	}
	state->signals = g_list_append(state->signals, d);
}

gint state_noof_channel_groups_in_use(struct state *state)
{
	gint r = 0;

	if (state->channels_in_use & 0x000000FF)
		r++;
	if (state->channels_in_use & 0x0000FF00)
		r++;
	if (state->channels_in_use & 0x00FF0000)
		r++;
	if (state->channels_in_use & 0xFF000000)
		r++;
	return r;
}

/* Return the size of the buffer in number of samples */
gint state_buffer_capacity(struct state *state)
{
	return MEMORY_SIZE / state_noof_channel_groups_in_use(state);
}

/* Return NULL if no such signal is defined */
struct signal_def *state_lookup_signal(struct state *state, gchar *name)
{
	for (GList *i = state->signals; i != NULL; i = g_list_next(i)) {
		struct signal_def *s = (struct signal_def *)i->data;

		if (g_strcmp0(s->name, name) == 0)
			return s;
	}
	return NULL;
}

guint32 state_signal_value(struct signal_def *signal, guint32 value)
{
	gint index = 0;
	guint32 r = 0;

	for (GList *i = signal->channels;
	     i != NULL;
	     i = g_list_next(i), index++) {
		int channel = GPOINTER_TO_INT(i->data);

		if (value & (1 << index))
			r |= (1 << channel);
	}
	return r;
}

guint32 state_signal_mask(struct signal_def *signal)
{
	return signal->mask;
}

