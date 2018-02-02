/* -*- linux-c -*-
 *
 * VCD dumper
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
#include "time.h"
#include "vcd.h"

struct vcd_state {
	struct state *state;
	FILE *out;
	guint8 *samples;
	gdouble timescale;
};

static void signal_def(struct vcd_state *state, struct signal_def* signal)
{
	fprintf(state->out, "$var wire %d %c %s $end\n",
		signal->noof_bits, signal->index + '!', signal->name);
}

static gboolean write_header(struct vcd_state *state)
{
	time_t t = time(NULL);
	double sample_time = 1.0/state->state->sample_rate;

	fprintf(state->out, "$date\n  %s$end\n", ctime(&t));
	fprintf(state->out,
		"$version\n  Open bench logic sniffer capture tool v"
		VERSION_STRING"\n$end\n");

	/* Dump triggers and arguments in a comment */
	fprintf(state->out, "$comment\n");
	fprintf(state->out, "  Sample rate %ld Hz\n",
		state->state->sample_rate);
	fprintf(state->out, "  Number of samples %d\n",
		state_buffer_capacity(state->state));
	fprintf(state->out, "$end\n");

	/* We want at least three decimals for each sample */
	if (state->state->sample_rate > 1000000)
		fprintf(state->out, "$timescale %dps $end\n",
			(int)(1e12*sample_time));
	else if (state->state->sample_rate > 1000)
		fprintf(state->out, "$timescale %dns $end\n",
			(int)(1e9*sample_time));
	else
		fprintf(state->out, "$timescale %dus $end\n",
			(int)(1e6*sample_time));

	fprintf(state->out, "$scope module logic $end\n");
	/* Wires here */
	for (GList *i = g_list_first(state->state->signals);
	     i != NULL;
	     i = g_list_next(i)) {
		signal_def(state, (struct signal_def*) i->data);
	}
	if (state->state->trigger_spec != NULL)
		fprintf(state->out, "$var event 1 trigg obls_trigger $end\n");
	fprintf(state->out, "$upscope $end\n");
	fprintf(state->out, "$enddefinitions $end\n");

	return TRUE;
}

static guint32 make_channels_mask(struct signal_def *s)
{
	guint32 r = 0;

	for (GList *i = g_list_first(s->channels);
	     i != NULL;
	     i = g_list_next(i)) {
		r |= (1 << GPOINTER_TO_INT(i->data));
	}
	return r;
}

static guint32 unpack_sample(guint32 channels_in_use, guint8 *samples)
{
	guint32 v = 0;

	if (channels_in_use & 0x000000FF)
		v |= *samples--;
	if (channels_in_use & 0x0000FF00)
		v |= (*samples-- << 8);
	if (channels_in_use & 0x00FF0000) {
		v |= (*samples-- << 16);
		samples--;
	}
	if (channels_in_use & 0xFF000000)
		v |= (*samples-- << 24);
	return v;
}

static void dump_value(struct vcd_state *state,
		       guint32 sample,
		       struct signal_def *signal)
{
	guint32 v = 0;
	gint index = 0;

	for (GList *i = g_list_first(signal->channels);
	     i != NULL;
	     i = g_list_next(i), index++) {
		if (sample & (1 << GPOINTER_TO_INT(i->data)))
			v |= (1 << index);
	}
	if (index == 1)
		fprintf(state->out, "%d%c\n", v, '!' + signal->index);
	else {
		fprintf(state->out, "b");
		for (; index >= 0; index--) {
			if (v & (1 << index))
				fprintf(state->out, "1");
			else
				fprintf(state->out, "0");
		}
		fprintf(state->out, " %c\n", '!' + signal->index);
	}
}

static void dump_values(struct vcd_state *state)
{
	gint noof_samples = state_buffer_capacity(state->state);
	/* A bit set to '1' in the word at index 'n' means that the
	   signal with index 'n' depends on the channel */
	guint32 channels_mask[state->state->noof_signals];
	guint32 current_sample, previous_sample;
	gint noof_groups = state_noof_channel_groups_in_use(state->state);

	current_sample = unpack_sample(
		state->state->channels_in_use,
		state->samples + noof_samples * noof_groups);
	fprintf(state->out, "$dumpvars\n");
	/* Initial values here */
	for (GList *i = g_list_first(state->state->signals);
	     i != NULL;
	     i = g_list_next(i)) {
		struct signal_def *s = i->data;

		channels_mask[s->index] = make_channels_mask(s);
		dump_value(state, current_sample, s);
	}
	fprintf(state->out, "$end\n");

	for (gint i = 1; i < noof_samples; i++) {
		guint32 diff;

		previous_sample = current_sample;
		current_sample = unpack_sample(
			state->state->channels_in_use,
			state->samples + (noof_samples - i) * noof_groups);
		diff = previous_sample ^ current_sample;

		if (!(diff & state->state->channels_in_use) &&
		    i != state->state->trigger_holdoff &&
		    i != (noof_samples - 1))
			continue;
		fprintf(state->out, "#%d\n", i);
		if (i == state->state->trigger_holdoff)
			fprintf(state->out, "1trigg\n");
		for (GList *sig = g_list_first(state->state->signals);
		     sig != NULL;
		     sig = g_list_next(sig)) {
			struct signal_def *s = sig->data;

			if (channels_mask[s->index] & diff)
				dump_value(state, current_sample, s);
		}

	}

}

gboolean vcd_dump(struct state *state, guint8 *samples)
{
	struct vcd_state s = {
		.state = state,
		.samples = samples
	};

	if (state->outfile == NULL)
		s.out = stdout;
	else {
		if ((s.out = fopen(state->outfile, "w")) == NULL) {
			perror(state->outfile);
			return FALSE;
		}
	}

	if (!write_header(&s)) {
		fclose(s.out);
		return FALSE;
	}

	dump_values(&s);
	fclose(s.out);
	return TRUE;
}
