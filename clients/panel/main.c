/* LASH Control Panel
 * Copyright(C) 2006 Dave Robillarrd <dave@drobilla.net>
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 *(at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include "lash/lash.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "panel.h"

static void
print_help()
{
	printf("LASH Control Panel version %s\n", PACKAGE_VERSION);
	printf
		("Copyright(C) 2006 Dave Robillard <dave@drobilla.net>\n");
	printf("\n");
	printf
		("This program comes with ABSOLUTELY NO WARRANTY.  You are licensed to use it\n");
	printf
		("under the terms of the GNU General Public License, version 2 or later.  See\n");
	printf("the COPYING file that came with this software for details.\n");
	printf("\n");
	printf(" -h, --help                  Display this help info\n");
	printf("     --lash-port=<port>    Connect to server on port <port>\n");
	printf("\n");
}

int
main(int argc, char **argv)
{
	lash_client_t *lash_client;
	panel_t *panel;
	int opt;
	const char *options = "h:";
	struct option long_options[] = {
		{"help", 0, NULL, 'h'},
		{0, 0, 0, 0}
	};

	gtk_set_locale();
	gtk_init(&argc, &argv);

	while ((opt = getopt_long(argc, argv, options, long_options, NULL)) != -1) {
		switch (opt) {
		case 'h':
			print_help();
			exit(0);
			break;

		case ':':
		case '?':
			print_help();
			exit(1);
			break;
		}
	}

	lash_client = lash_client_open_controller();

	if (!lash_client) {
		fprintf(stderr, "%s: could not initialise LASH\n", __FUNCTION__);
		exit(1);
	}

	panel = panel_create(lash_client);

	if (!lash_set_control_callback(lash_client, deal_with_event,
	                               (void *) panel)) {
		// TODO: Is there really no destructor for panel_t?
		fprintf(stderr, "%s: could not initialise LASH\n", __FUNCTION__);
		exit(1);
	}

	gtk_main();

	return 0;
}
