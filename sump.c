/* -*- linux-c -*-
 *
 * SUMP protocol
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

#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include "sump.h"
#include "serial.h"

#define WRITE_TIMEOUT_MS 1000
#define CMD_TIMEOUT_MS    200
#define DRAIN_TIMEOUT_MS  100
#define DRAIN_BUFFER_SIZE 256

enum sump_commands {
	CMD_RESET = 0x00,
	CMD_RUN = 0x01,
	CMD_ID = 0x02,
	CMD_XON = 0x11,
	CMD_XOFF = 0x13,
	CMD_SET_TRIGGER_0_MASK = 0xc0,
	CMD_SET_TRIGGER_1_MASK = 0xc4,
	CMD_SET_TRIGGER_2_MASK = 0xc8,
	CMD_SET_TRIGGER_3_MASK = 0xcc,

	CMD_SET_TRIGGER_0_VALUES = 0xc1,
	CMD_SET_TRIGGER_1_VALUES = 0xc5,
	CMD_SET_TRIGGER_2_VALUES = 0xc9,
	CMD_SET_TRIGGER_3_VALUES = 0xcd,

	CMD_SET_TRIGGER_0_CONF = 0xc2,
	CMD_SET_TRIGGER_1_CONF = 0xc6,
	CMD_SET_TRIGGER_2_CONF = 0xca,
	CMD_SET_TRIGGER_3_CONF = 0xce,

	CMD_SET_DIVIDER = 0x80,
	CMD_SET_READ_AND_DELAY_COUNT = 0x81,
	CMD_SET_FLAGS = 0x82
};

gboolean sump_drain_input(gint fd)
{
	guint8 buffer[DRAIN_BUFFER_SIZE];
	fd_set fds;

	FD_ZERO(&fds);

	while (TRUE) {
		ssize_t r;
		int sel_result;
		struct timeval timeout = {
			.tv_sec = 0,
			.tv_usec = DRAIN_TIMEOUT_MS * 1000
		};

		FD_SET(fd, &fds);
		sel_result = select(fd + 1, &fds, NULL, NULL, &timeout);
		if (sel_result == 1 && FD_ISSET(fd, &fds)) {
			FD_CLR(fd, &fds);
			r = read(fd, buffer, DRAIN_BUFFER_SIZE);
			if (r == -1) {
				if (errno == EAGAIN)
					continue;
				perror("read");
				return FALSE;
			}
		} else if (sel_result == 0)
			return TRUE;
		else {
			perror("select in sump_drain");
			return FALSE;
		}
	}
}

void sump_dump_buffer(gsize size, gpointer buffer)
{
	guint8 *b = buffer;

	fprintf(stderr, "buffer %p size=%d\n", buffer, (gint)size);
	for (gsize i = 0; i < size;) {
		if (i % 16 == 0)
			fprintf(stderr, "%08x  ", (unsigned int)i);
		fprintf(stderr, "%02x", b[i++]);
		if (i % 16 == 0) {
			fprintf(stderr, "\n");
			continue;
		}
		if (i % 8 == 0)
			fprintf(stderr, "  ");
		else
			fprintf(stderr, " ");
	}
	fprintf(stderr, "\n");
}

static gboolean send_buffer(gint fd, gsize size, gpointer buffer)
{
	ssize_t r;
	guint8 *b = buffer;
	fd_set fds;

	FD_ZERO(&fds);

	while (size > 0) {
		int sel_result;
		struct timeval timeout = {
			.tv_sec = 0,
			.tv_usec = WRITE_TIMEOUT_MS * 1000
		};

		FD_SET(fd, &fds);
		sel_result = select(fd + 1, NULL, &fds, NULL, &timeout);
		if (sel_result == 0)
			return FALSE;
		else if (sel_result < 0) {
			perror("select in sump_send_buffer");
			return FALSE;
		} else {
			r = write(fd, b, size);
			if (r == -1) {
				if (errno == EAGAIN)
					continue;
				perror("write in sump_send_buffer");
				return FALSE;
			}
			size -= r;
			b += r;
		}
	}
	return TRUE;
}



gboolean sump_read_buffer(gint fd, gsize size, gpointer buffer, gint timeout)
{
	guint8 *b = buffer;
	ssize_t r;
	fd_set fds;

	FD_ZERO(&fds);

	while (size > 0) {
		int sel_result;
		struct timeval to = {
			.tv_sec = 0,
			.tv_usec = timeout * 1000
		};

		FD_SET(fd, &fds);
		sel_result = select(fd + 1, &fds, NULL, NULL,
				    timeout == -1 ? NULL: &to);
		if (sel_result == 1 && FD_ISSET(fd, &fds)) {
			FD_CLR(fd, &fds);
			r = read(fd, b, size);
			if (r == -1) {
				if (errno == EAGAIN)
					continue;
				perror("write in sump_read_buffer");
				return FALSE;
			} else if (r == 0) {
				return FALSE;
			}
			size -= r;
			b += r;
		} else if (sel_result == 0)
			return FALSE;
		else if (sel_result < 0) {
			perror("select in sump_read_buffer");
			return FALSE;
		}
	}
	return TRUE;
}

gboolean sump_cmd_reset(gint fd)
{
	guint8 buff[5] = {
		CMD_RESET, CMD_RESET, CMD_RESET, CMD_RESET, CMD_RESET
	};

	return send_buffer(fd, sizeof(buff), buff);
}

gboolean sump_cmd_id(gint fd, guint32 *ident)
{
	guint8 buff[1] = {
		CMD_ID
	};

	if (!send_buffer(fd, sizeof(buff), buff))
		return FALSE;
	return sump_read_buffer(fd, sizeof(*ident), ident, CMD_TIMEOUT_MS);
}

gboolean sump_cmd_set_trigger(gint fd, struct sump_trigger *trigger)
{
	guint8 buffer[] = {
		CMD_SET_TRIGGER_0_MASK,
		0, 0, 0, 0,
		CMD_SET_TRIGGER_0_VALUES,
		0, 0, 0, 0,
		CMD_SET_TRIGGER_0_CONF,
		0, 0, 0, 0
	};
	guint32 *mask_ptr = (guint32 *)(buffer + 1);
	guint32 *values_ptr = (guint32 *)(buffer + 6);
	guint32 *delay_ptr = (guint32 *)(buffer + 11);
	guint32 *conf_ptr = (guint32 *)(buffer + 13);

	if (trigger->trigger > 3 || trigger->level > 3 || trigger->channel > 31)
		return FALSE;
	for (gint i = 0; i < 3; i++)
		buffer[i * 5] += trigger->trigger * 4;
	*mask_ptr = trigger->mask;
	*values_ptr = trigger->values;
	*delay_ptr = trigger->level;
	*conf_ptr = (trigger->start ? 0x800 : 0)
		| (trigger->serial ? 4 : 0)
		| (trigger->level << 8)
		| ((trigger->channel & 0xf) << 12)
		| ((trigger->channel & 0x10) >> 4);
	return send_buffer(fd, sizeof(buffer), buffer);
}

gboolean sump_cmd_set_divider(gint fd, guint32 divider)
{
	guint8 buff[] = {
		CMD_SET_DIVIDER,
		(divider & 0xFF),
		((divider >> 8) & 0xFF),
		((divider >> 16) & 0xFF), 0
	};

	return send_buffer(fd, sizeof(buff), buff);
}

gboolean sump_cmd_set_size(gint fd,
			   guint16 read_count, guint16 delay_count)
{
	guint8 buffer[] = {
		CMD_SET_READ_AND_DELAY_COUNT,
		0, 0, 0, 0
	};
	guint32 *read_ptr = (guint32 *)(buffer + 1);
	guint32 *delay_ptr = (guint32 *)(buffer + 3);

	*read_ptr = read_count;
	*delay_ptr = delay_count;
	return send_buffer(fd, sizeof(buffer), buffer);
}

gboolean sump_cmd_set_flags(gint fd, guint32 flags)
{
	guint8 buffer[] = {
		CMD_SET_FLAGS,
		0, 0, 0, 0
	};
	guint32 *ptr = (guint32 *)(buffer + 1);

	*ptr = flags;
	return send_buffer(fd, sizeof(buffer), buffer);
}

gboolean sump_cmd_run(gint fd)
{
	guint8 buff[] = {
		CMD_RUN
	};

	return send_buffer(fd, sizeof(buff), buff);
}
