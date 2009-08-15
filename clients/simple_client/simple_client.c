/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *   Copyright (C) 2002 Robert Ham <rah@bash.sh>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#include <jack/jack.h>
#include <lash/lash.h>

#define info(fmt, args...) printf(fmt "\n", ## args)
#define error(fmt, args...) fprintf(stderr, "%s: " fmt "\n", __FUNCTION__, ## args)

/* Callback function prototype */
static bool
callback(enum LashEvent  type,
         void           *user_data);

int
main(int    argc,
     char **argv)
{
	lash_client_t *client;
	jack_client_t *jack_client;
	char client_name[64];
	int ret = EXIT_FAILURE;

	sprintf(client_name, "lsec_%d", getpid());
	info("Client name: '%s'", client_name);

	/* LASH pt. 1 */
	info("Connecting to LASH server");
	if (!(client = lash_client_open("LASH Simple Client", 0, argc, argv))) {
		error("Failed to open LASH client");
		return EXIT_FAILURE;
	}
	info("Connected to LASH server as %s client",
	     lash_client_is_being_restored(client) ? "restoring" : "new");

	/* JACK */
	info("Connecting to JACK server");
	jack_client = jack_client_open(client_name, JackNullOption, NULL);
	if (!jack_client) {
		error("Failed to open JACK client");
		goto end;
	}
	info("Connected to JACK with client name '%s'", client_name);

	/* LASH pt. 2 */
	lash_set_client_callback(client, callback, client);
	if (!lash_activate(client)) {
		error("Failed to activate LASH client");
		goto end2;
	}
	info("Client '%s' is associated with project '%s'",
	     lash_get_client_name(client), lash_get_project_name(client));

	pause();

	info("Bye!");

end2:
	jack_client_close(jack_client);
end:
	lash_client_close(client);
	return ret;
}

static bool
save(lash_client_t *client)
{
	const char *name = "lash_simple_client";
	double version = 1.0;
	uint32_t revision = 1;
	const char *raw_data = "some raw data";

	lash_write(client, "name", &name, LASH_TYPE_STRING, 0);
	lash_write(client, "version", &version, LASH_TYPE_DOUBLE, 0);
	lash_write(client, "revision", &revision, LASH_TYPE_INTEGER, 0);

	/* Write a string as raw data without the terminating NUL */
	lash_write(client, "raw data", &raw_data, LASH_TYPE_RAW, strlen(raw_data));

	return true;
}

static bool
load(lash_client_t *client)
{
	const char *key;
	int ret;
	enum LashType type;

	union {
		double      d;
		uint32_t    u;
		const char *s;
	} value;

	while ((ret = lash_read(client, &key, &value, &type))) {
		if (ret == -1) {
			error("Failed to read data set");
			return false;
		}

		if (type == LASH_TYPE_DOUBLE)
			info("  \"%s\" : %f", key, value.d);
		else if (type == LASH_TYPE_INTEGER)
			info("  \"%s\" : %u", key, value.u);
		else if (type == LASH_TYPE_STRING)
			info("  \"%s\" : \"%s\"", key, value.s);
		else if (type == LASH_TYPE_RAW)
			info("  \"%s\" : <raw data>", key);
		else
			error("Unknown config type");
	}

	return true;
}

static bool
callback(enum LashEvent  type,
         void           *user_data)
{
	lash_client_t *client = user_data;

	switch (type) {
	/* We are always interruptible */
	case LASH_EVENT_INTERRUPT:
		return true;
	case LASH_EVENT_SAVE:
		info("Client saving");
		return save(client);
	case LASH_EVENT_LOAD:
		info("Client loading");
		return load(client);
	case LASH_EVENT_QUIT:
		info("Client quitting");
		kill(getpid(), SIGQUIT);
		return true;
	case LASH_EVENT_CHANGE_NAME:
		info("Client name changed to '%s'", lash_get_client_name(client));
		return true;
	case LASH_EVENT_CHANGE_PROJECT:
		info("Project changed to '%s'", lash_get_project_name(client));
		return true;
	case LASH_EVENT_CHANGE_PROJECT_NAME:
		info("Project name changed to '%s'", lash_get_project_name(client));
		return true;
	case LASH_EVENT_INVALID:
	default:
		error("Received invalid event #%d", type);
		return false;
	}
}
