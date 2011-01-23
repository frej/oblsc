/* -*- linux-c -*-
 *
 * Functions for opening a serial port on linux
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

#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <termios.h>

/*
 * Open the tty and set it up for communicating with the OLS.
 * 115200bps 8N1
 *
 * Return -1 on error or the FD for the tty.
 */
int open_serial(char *name, speed_t baudrate);


#endif /* _SERIAL_H_ */
