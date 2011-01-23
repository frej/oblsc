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

#ifndef _SUMP_H_
#define _SUMP_H_

#include <glib.h>

struct sump_trigger {
	guint trigger;
	guint32 mask;
	guint32 values;
	guint16 delay;
	guint8 level;
	guint8 channel;
	gboolean serial;
	gboolean start;
};

/* Drain the input buffer */
gboolean sump_drain_input(gint fd);

gboolean sump_read_buffer(gint fd, gsize size, gpointer buffer, gint timeout);

gboolean sump_cmd_reset(gint fd);
gboolean sump_cmd_id(gint fd, guint32 *ident);
gboolean sump_cmd_set_trigger(gint fd, struct sump_trigger *trigger);
gboolean sump_cmd_set_divider(gint fd, guint32 divider);
gboolean sump_cmd_set_size(gint fd, guint16 read_count, guint16 delay_count);
gboolean sump_cmd_set_flags(gint fd, guint32 flags);
gboolean sump_cmd_run(gint fd);
void sump_dump_buffer(gsize size, gpointer buffer);

#define SUMP_FLAG_DEMUX                    0x00000001
#define SUMP_FLAG_FILTER                   0x00000002
#define SUMP_FLAG_CHANNEL_GROUP_0_DISABLED 0x00000004
#define SUMP_FLAG_CHANNEL_GROUP_1_DISABLED 0x00000008
#define SUMP_FLAG_CHANNEL_GROUP_2_DISABLED 0x00000010
#define SUMP_FLAG_CHANNEL_GROUP_3_DISABLED 0x00000020
#define SUMP_FLAG_EXTERNAL_CLOCK           0x00000040
#define SUMP_FLAG_INVERT_EXTERNAL_CLOCK    0x00000080


#endif /* _SUMP_H_ */
