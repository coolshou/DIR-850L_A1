/*
 * Copyright (C) 2000 Lennert Buytenhek
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _BRCTL_H
#define _BRCTL_H

struct command
{
	int	needs_bridge_argument;
	int	num_string_arguments;
	char * name;
	union
	{
		void (*func)(struct bridge *br, char *arg0, char *arg1);	/* The original handler, takes 2 args. */
		void (*func3)(struct bridge *br, char *arg0, char *arg1, char *arg2); /* This one takes 3 args. */
		void (*func_main)(struct bridge *br, int argc, char *argv[]); /* This one takes 3 args. */
	};
};

struct command *br_command_lookup(char *cmd);
void br_dump_bridge_id(unsigned char *x);
void br_show_timer(struct timeval *tv);
void br_dump_interface_list(struct bridge *br);
void br_dump_port_info(struct port *p);
void br_dump_info(struct bridge *br);

#endif
