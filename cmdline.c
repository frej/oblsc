/* -*- linux-c -*-
 *
 * Command line parsing functions.
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
#include "cmdline.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <unistd.h>


struct param {
	gchar *value;
	enum { CMDLINE, CONFIG, DEFAULT } where;
};

/* Stores given command line parameters */
struct cmd_line {
	gchar *conf_file;

	/* device */
	struct param device;
	struct param baudrate;

	/* clock */
	struct param sample_rate;
	struct param external_clock;
	struct param external_invert;

	/* capture */
	struct param filter;
	struct param trigger_split;

	gchar *outfile;
	gchar **signals;
	gchar *trigger;

};

typedef gchar *(*parse_fun_t)(gchar *value);

static void handle_command_line(struct cmd_line *cl, int argc, gchar *argv[])
{
	GError *error = NULL;
	GOptionContext *context;
	GOptionEntry entries[] = {
		{ .long_name = "config",
		  .short_name = 'C',
		  .flags = 0,
		  .arg = G_OPTION_ARG_FILENAME,
		  .arg_data = &cl->conf_file,
		  .description = "Configuration file",
		  .arg_description = "<filename>"},
		{ .long_name = "device",
		  .short_name = 'D',
		  .flags = 0,
		  .arg = G_OPTION_ARG_FILENAME,
		  .arg_data = &cl->device,
		  .description = "Device",
		  .arg_description = "<device>" },
		{ .long_name = "baudrate",
		  .short_name = 'B',
		  .flags = 0,
		  .arg = G_OPTION_ARG_STRING,
		  .arg_data = &cl->baudrate,
		  .description = "Baudrate",
		  .arg_description = "<baudrate>" },
		{ .long_name = "trigger",
		  .short_name = 't',
		  .flags = 0,
		  .arg = G_OPTION_ARG_STRING,
		  .arg_data = &cl->trigger,
		  .description = "Set trigger condition",
		  .arg_description = "<trigger-spec>"},
		{ .long_name = "trigger-split",
		  .short_name = 'r',
		  .flags = 0,
		  .arg = G_OPTION_ARG_STRING,
		  .arg_data = &cl->trigger_split,
		  .description = "The number of samples to keep before the" \
		                 " trigger-point",
		  .arg_description = "<percent>%/<number-of-samples>/time"},
		{ .long_name = "sample-rate",
		  .short_name = 'S',
		  .flags = 0,
		  .arg = G_OPTION_ARG_STRING,
		  .arg_data = &cl->sample_rate,
		  .description = "The sample rate",
		  .arg_description = "<Hz>" },
		{ .long_name = "filter",
		  .short_name = 'f',
		  .flags = 0,
		  .arg = G_OPTION_ARG_STRING,
		  .arg_data = &cl->filter,
		  .description = "Enables the filter input module",
		  .arg_description = "true/false (1/0)" },
		{ .long_name = "external-clock",
		  .short_name = 'e',
		  .flags = 0,
		  .arg = G_OPTION_ARG_STRING,
		  .arg_data = &cl->external_clock,
		  .description = "Use the external clock",
		  .arg_description = "true/false (1/0)" },
		{ .long_name = "invert-external-clock",
		  .short_name = 'i',
		  .flags = 0,
		  .arg = G_OPTION_ARG_STRING,
		  .arg_data = &cl->external_invert,
		  .description = "Invert the external clock",
		  .arg_description = "true/false (1/0)"},
		{ .long_name = "output",
		  .short_name = 'o',
		  .flags = 0,
		  .arg = G_OPTION_ARG_FILENAME,
		  .arg_data = &cl->outfile,
		  .description = "Output filename",
		  .arg_description = "<filename>" },
		{ .long_name = "signal",
		  .short_name = 's',
		  .flags = 0,
		  .arg = G_OPTION_ARG_STRING_ARRAY,
		  .arg_data = &cl->signals,
		  .description = "Define an input signal",
		  .arg_description = "<name>:<chlist>"},
		{ NULL }
	};

	memset(cl, 0, sizeof(*cl));
	context = g_option_context_new("- Open Bench Logic Sniffer");
	g_option_context_add_main_entries(context, entries, NULL);
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		fprintf(stderr, "option parsing failed: %s\n", error->message);
		exit(1);
	}
}

static void lookup_option(GKeyFile *keyfile,
			  gchar *group, gchar *key,
			  gchar *the_default,
			  struct param *p)
{
	if (p->value != NULL) {
		p->where = CMDLINE;
		return;
	}

	/* The value was not given, look in the configuration file */
	GError *e = NULL;
	gchar *v = g_key_file_get_value(keyfile, group, key, &e);

	if (v != NULL) {
		p->value = v; /* Don't care about leaked memory */
		p->where = CONFIG;
		return;
	}
	p->value = the_default;
	p->where = DEFAULT;
}

static void parse_baudrate(struct param *value, speed_t *baudrate)
{
	int speed;

	if (sscanf(value->value, "%d", &speed) != 1)
		speed = 0;
	switch (speed) {
	case 1152000:
		*baudrate = B115200;
		break;
	case 57600:
		*baudrate = B57600;
		break;
	case 38400:
		*baudrate = B38400;
		break;
	case 19200:
		*baudrate = B19200;
		break;
	default:
		fprintf(stderr,
			"The only supported baudrates are 1152000, 57600,"
			" 38400 and 19200 baud. "
			"\"%s\" as specified %s is not supported\n",
			value->value,
			value->where == CMDLINE ? "on the command line" :
			"in the configuration file");
		exit(1);
	}
}

static void parse_sample_rate(struct param *value, glong *sample_rate)
{
	long v;
	char *tail;

	v = strtol(value->value, &tail, 0);
	if (tail == value->value) {
		fprintf(stderr,
			"Cannot parse \"%s\" as specified %s as a sample rate\n",
			value->value,
			value->where == CMDLINE ? "on the command line" :
			"in the configuration file");
		exit(1);
	}
	if (strcmp(tail, "M") == 0)
		v *= 1000000;
	else if (strcmp(tail, "k") == 0)
		v *= 1000;
	else if (*tail != 0) {
		fprintf(stderr,
			"Cannot parse \"%s\" as specified %s as a "
			"sample rate. The suffix \"%s\" is not supported\n",
			value->value,
			value->where == CMDLINE ? "on the command line" :
			"in the configuration file",
			tail);
		exit(1);
	}
	*sample_rate = v;
	if (*sample_rate > 2*CLOCK_FREQ) {
		fprintf(stderr,
			"Specified sample rate %ld Hz as specified %s is"
			" outside the supported range.\n",
			v,
			value->where == CMDLINE ? "on the command line" :
			"in the configuration file");
		exit(1);
	}

}

static void parse_boolean(gchar *desc, struct param *value, gboolean *flag)
{
	if (*value->value == '1' ||
	    strcasecmp(value->value, "true") == 0)
		*flag = TRUE;
	else if (*value->value == '0' ||
		 strcasecmp(value->value, "false") == 0)
		*flag = FALSE;
	else
		fprintf(stderr,
			"Cannot parse boolean flag %s as specified %s."
			" \"%s\" is not a boolean value\n",
			desc,
			value->where == CMDLINE ? "on the command line" :
			"in the configuration file",
			value->value);
}

static GList *parse_channel_list(gchar *channels)
{
	gchar *tail;
	long channel = strtol(channels, &tail, 0);
	long channel2;
	gchar *tail2;
	GList *r = NULL;
	int i;

	if (tail == channels) {
		fprintf(stderr, "Expected channel number at '%s'\n",
			channels);
		exit(1);
	}
	if (*tail == 0)
		return g_list_prepend(NULL, GINT_TO_POINTER(channel));
	if (*tail == ',')
		return g_list_prepend(parse_channel_list(tail + 1),
				      GINT_TO_POINTER(channel));
	if (*tail != '-') {
		fprintf(stderr, "Unexpected separator in channel list '%s'\n",
			channels);
		exit(1);
	}

	channel2 = strtol(tail + 1, &tail2, 0);
	if (tail == tail2) {
		fprintf(stderr,
			"Expected channel number following '-' in '%s'\n",
			channels);
		exit(1);
	}

	i = channel;
	while (TRUE) {
		r = g_list_append(r, GINT_TO_POINTER(i));
		if (i == channel2)
			break;
		if (channel < channel2)
			i++;
		else
			i--;
	}
	if (*tail2 == ',')
		return g_list_concat(r, parse_channel_list(tail2 + 1));
	if (*tail2 == 0)
		return r;
	fprintf(stderr,
		"Expected end of channel list following '%s'\n",
		tail);
	exit(1);
}

static void parse_signals(gchar **signals, struct state *state)
{
	if (signals == NULL) {
		fprintf(stderr,
			"No signals defined. "
			"Cowardly refusing to perform empty capture.\n");
		exit(1);
	}
	for (int i = 0; signals[i] != NULL; i++) {
		gchar *tmp = g_strdup(signals[i]);
		gchar *name = strtok(tmp, ":");
		gchar *channels = strtok(NULL, ":");

		if (channels == NULL) {
			fprintf(stderr,
				"Expected channel list in signal definition %s\n",
				signals[i]);
			exit(1);
		}
		state_add_signal(state, name, parse_channel_list(channels));
		g_free(tmp);
	}
}

/* Trigger split must be called last when the state is populated */
static void parse_trigger_split(struct param *value, struct state *state)
{
	double v;
	char *tail;
	gint samples = -1;
	gint buffer_size = state_buffer_capacity(state);

	v = strtod(value->value, &tail);
	if (value->value == tail) {
		fprintf(stderr,
			"Cannot parse \"%s\" as specified %s "
			"as a valid trigger split.\n",
			value->value,
			value->where == CMDLINE ? "on the command line" :
			"in the configuration file");
		exit(1);
	}
	if (*tail == '%') {
		samples = v * 0.01 * buffer_size;
	} else if (*tail == 0) {
		samples = v;
	} else if (strcasecmp(tail, "s") == 0) {
		samples = state->sample_rate;
	} else if (strcasecmp(tail, "ms") == 0) {
		samples = state->sample_rate * v * 0.001;
	} else if (strcasecmp(tail, "us") == 0) {
		samples = state->sample_rate * v * 0.000001;
	} else if (strcasecmp(tail, "ns") == 0) {
		samples = state->sample_rate * v * 0.000000001;
	} else if (strcasecmp(tail, "ps") == 0) {
		samples = state->sample_rate * v * 0.000000000001;
	} else {
		fprintf(stderr,
			"Cannot parse \"%s\" as specified %s as a "
			"valid trigger split. "
			"The suffix \"%s\" is not supported\n",
			value->value,
			value->where == CMDLINE ? "on the command line" :
			"in the configuration file",
			tail);
		exit(1);
	}
	/* Do sanity check */
	if (samples > buffer_size) {
		fprintf(stderr,
			"With the current configuration there "
			"are %d samples available. The given trigger "
			"split would use %d samples.\n",
			buffer_size, samples);
	}
	state->trigger_holdoff = samples;
}

static void include_config_and_defaults(
	struct cmd_line *cl, gchar *filename)
{
	GKeyFile *f;
	GError *error = NULL;

	f = g_key_file_new();
	if (!g_key_file_load_from_file(f, filename, G_KEY_FILE_NONE, &error)) {
		fprintf(stderr,
			"Failed to read configuration file %s: %s. "
			"Will continue using defaults\n",
			filename, error->message);
	}

	lookup_option(f, "device", "device", "/dev/ttyACM0", &cl->device);
	lookup_option(f, "device", "baudrate", "115200", &cl->baudrate);
	lookup_option(f, "clock", "sample-rate", "100M", &cl->sample_rate);
	lookup_option(f, "clock", "external-clock", "false",
		      &cl->external_clock);
	lookup_option(f, "clock", "invert-external-clock", "false",
		      &cl->external_invert);
	lookup_option(f, "capture", "filter", "true", &cl->filter);
	lookup_option(f, "capture", "split", "0%", &cl->trigger_split);

	g_key_file_free(f);
}

static void setup_state(struct cmd_line *cl, struct state *state)
{
	state->signals = NULL;
	state->channels_in_use = 0;
	state->device = cl->device.value;
	state->outfile = cl->outfile;
	state->noof_signals = 0;
	state->trigger_spec = cl->trigger;
	parse_baudrate(&cl->baudrate, &state->baudrate);
	parse_sample_rate(&cl->sample_rate, &state->sample_rate);
	parse_boolean("external clock",
		      &cl->external_clock, &state->external_clock);
	parse_boolean("invert external clock",
		      &cl->external_invert, &state->external_invert);
	parse_boolean("filter input module", &cl->filter, &state->filter);
	parse_signals(cl->signals, state);
	parse_trigger_split(&cl->trigger_split, state);
}

void setup_configuration(int argc, gchar *argv[], struct state *state)
{
	struct cmd_line cl;

	handle_command_line(&cl, argc, argv);

	if (cl.conf_file == NULL) {
		gchar *file = g_strconcat(g_get_user_config_dir(),
					  "/oblsc.rc", NULL);
		include_config_and_defaults(&cl, file);
		g_free(file);
	} else
		include_config_and_defaults(&cl, cl.conf_file);
	setup_state(&cl, state);
}

