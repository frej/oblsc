/* -*- linux-c -*-
 *
 * Open Bench Logic Sniffer Client
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
#include <glib.h>
#include <glib-object.h>
#include "sump.h"
#include "serial.h"
#include "state.h"
#include "cmdline.h"
#include "vcd.h"
#include "trigger.h"

static gboolean setup_hardware(int port, struct state *state)
{
	gboolean success = FALSE;
	guint32 ident;

	if (!sump_drain_input(port)) {
		fprintf(stderr, "Failed to drain input\n");
		goto error;
	}

	if (!sump_cmd_reset(port)) {
		fprintf(stderr, "Reset failed\n");
		goto error;
	}

	if (!sump_cmd_id(port, &ident)) {
		fprintf(stderr, "Ident failed\n");
		goto error;
	}
	if (ident != 0x534c4131) {
		fprintf(stderr, "Ident failed, device returned 0x%x\n", ident);
		goto error;
	}
	success = TRUE;
error:
	return success;
}

static gboolean setup_triggers(int port, struct state *state)
{
	gboolean success = FALSE;
	if (!trigger_compile(state))
		goto error;
	for (gint i = 0; i < NOOF_TRIGGERS; i++)
		if (!sump_cmd_set_trigger(port, state->triggers + i)) {
			fprintf(stderr, "Failed to set up trigger\n");
			goto error;
		}
	success = TRUE;
error:
	return success;
}

static gboolean setup_capture(int port, struct state *state)
{
	gboolean success = FALSE;
	guint32 divider, flags = 0;
	guint32 buffer_capacity;

	if (state->sample_rate > CLOCK_FREQ &&
	    state_noof_channel_groups_in_use(state) > 2) {
		fprintf(stderr,
			"Cannot handle clock frequency of %ld when %d "
			"channel groups are used.\n",
			state->sample_rate,
			state_noof_channel_groups_in_use(state));
		goto error;
	}

	if (state->sample_rate > CLOCK_FREQ)
		divider = (2 * CLOCK_FREQ / state->sample_rate) - 1;
	else
		divider = (CLOCK_FREQ / state->sample_rate) - 1;
	if (!sump_cmd_set_divider(port, divider)) {
		fprintf(stderr, "Failed to set divider\n");
		goto error;
	}
	if (state->sample_rate > CLOCK_FREQ) {
		flags |= SUMP_FLAG_DEMUX;
	}
	if ((flags & SUMP_FLAG_DEMUX) && state->filter) {
		fprintf(stderr,
			"Cannot use the filter at the current sample rate\n");
		goto error;
	}
	if (state->filter)
		flags |= SUMP_FLAG_FILTER;
	if (state->external_clock)
		flags |= SUMP_FLAG_EXTERNAL_CLOCK;
	if (state->external_invert)
		flags |= SUMP_FLAG_INVERT_EXTERNAL_CLOCK;
	if ((state->channels_in_use & 0x000000FF) == 0)
		flags |= SUMP_FLAG_CHANNEL_GROUP_0_DISABLED;
	if ((state->channels_in_use & 0x0000FF00) == 0)
		flags |= SUMP_FLAG_CHANNEL_GROUP_1_DISABLED;
	if ((state->channels_in_use & 0x00FF0000) == 0)
		flags |= SUMP_FLAG_CHANNEL_GROUP_2_DISABLED;
	if ((state->channels_in_use & 0xFF000000) == 0)
		flags |= SUMP_FLAG_CHANNEL_GROUP_3_DISABLED;

	if (!sump_cmd_set_flags(port, flags)) {
		fprintf(stderr, "Failed to set flags\n");
		goto error;
	}

	if (!setup_triggers(port, state)) {
		fprintf(stderr, "Failed to set up triggers\n");
		goto error;
	}

	buffer_capacity = state_buffer_capacity(state);
	if (!sump_cmd_set_size(
		    port, (buffer_capacity >> 2) - 1,
		    ((buffer_capacity - state->trigger_holdoff) >> 2) - 1)) {
	 	fprintf(stderr, "Failed to set size\n");
		goto error;
	}

	success = TRUE;
error:
	return success;
}

static guint8 *do_capture(struct state *state)
{
	guint8 *buffer = NULL;
	gint port;
	guint32 buffer_size;

	port = open_serial(state->device, state->baudrate);

	if (port == -1) {
		perror("open_serial");
		return NULL;
	}

	if (!setup_hardware(port, state)) {
		fprintf(stderr, "Failed to set up hardware\n");
		goto error;
	}

	if (!setup_capture(port, state)) {
		fprintf(stderr, "Failed to set up capture\n");
		goto error;
	}

	if (!sump_cmd_run(port)) {
		fprintf(stderr, "Failed to run\n");
		goto error;
	}

	buffer_size = state_buffer_capacity(state)
		* state_noof_channel_groups_in_use(state);
	buffer = g_malloc(buffer_size);

	if (!sump_read_buffer(port, buffer_size, buffer, -1)) {
		fprintf(stderr, "Failed to read result\n");
		g_free(buffer);
		goto error;
	}
	return buffer;
error:
	close(port);
	return NULL;
}

gint main(int argc, gchar *argv[])
{
	struct state state;
	guint8 *samples;

	setup_configuration(argc, argv, &state);

	samples = do_capture(&state);
	if (samples == NULL) {
		fprintf(stderr, "Failed to obtain capture\n");
		exit(1);
	}
	if (!vcd_dump(&state, samples)) {
		fprintf(stderr, "Failed to dump capture to VCD\n");
		exit(1);
	}

	return 0;
}
