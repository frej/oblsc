/* -*- linux-c -*-
 *
 * Main state definition
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

#ifndef _STATE_H_
#define _STATE_H_

#include <glib.h>
#include <termios.h>
#include <unistd.h>
#include "sump.h"

#define MEMORY_SIZE (24*1024) /* bytes */
#define CLOCK_FREQ  100000000 /* Hz */
#define NOOF_TRIGGERS 4
#define MAX_SAMPLE_DELAY 0xFFFF

struct signal_def {
	gint index;
	gchar *name;
	gint noof_bits;
	guint32 mask;
	GList *channels; /* gints */
};

struct state {
	/* Command line parameters */
	gchar *device;
	gchar *outfile;
	speed_t baudrate;
	glong sample_rate;
	gboolean external_clock;
	gboolean external_invert;
	gboolean filter;
	gchar *trigger_spec;
	gint trigger_holdoff;

	guint32 channels_in_use; /* Bit-vector of used channels */
	gint noof_signals;
	GList *signals; /* struct signal_def* */
	struct sump_trigger triggers[NOOF_TRIGGERS];
};

/* channels is a list of gints */
void state_add_signal(struct state *state, gchar *name, GList *channels);

gint state_noof_channel_groups_in_use(struct state *state);

/* Return the size of the buffer in number of samples */
gint state_buffer_capacity(struct state *state);

/* Return NULL if no such signal is defined */
struct signal_def *state_lookup_signal(struct state *state, gchar *name);

guint32 state_signal_value(struct signal_def *signal,
			   guint32 value);

guint32 state_signal_mask(struct signal_def *signal);

#endif /* _STATE_H_ */
