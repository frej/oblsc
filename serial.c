/* -*- linux-c -*-
 *
 * Functions for communicating with a serial port on linux.
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
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "serial.h"

/*
 * Open the tty and set it up.
 * 9600 8N1
 *
 * Return -1 on error or the FD for the tty.
 */
int open_serial(char *name, speed_t baudrate)
{
	int serial;
	struct termios serial_struct;

	if((serial = open(name, O_RDWR | O_NONBLOCK)) < 0) {
		perror(name);
		return -1;
	}

	tcgetattr(serial, &serial_struct);

	/* Clear the flags we don't want */
	serial_struct.c_iflag &= ~(INPCK | ISTRIP | IGNCR | ICRNL |
				   INLCR | IXOFF | IXON);
	serial_struct.c_oflag &= ~(OPOST);
	serial_struct.c_lflag &= ~(ISIG | ICANON | ECHO);
	serial_struct.c_cflag &= ~(CSTOPB | PARENB | CSIZE);

	/* Set the flags we want */
	serial_struct.c_iflag |= IGNBRK;
	serial_struct.c_cflag |= (CS8 | CREAD) ;

	cfsetospeed(&serial_struct, B115200);
	cfsetispeed(&serial_struct, B115200);

	if(tcsetattr(serial, TCSANOW, &serial_struct) < 0)
		perror ("tcsetattr");

	return serial;
}
