/**
 * This file is part of Hercules.
 * http://herc.ws - http://github.com/HerculesWS/Hercules
 *
 * Copyright (C) 2012-2020 Hercules Dev Team
 * Copyright (C) Athena Dev Teams
 *
 * Hercules is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define HERCULES_CORE

#include "config/core.h" // CELL_NOSTACK, CIRCULAR_AREA, CONSOLE_INPUT, DBPATH, RENEWAL
#include "api.h"

#include "api/HPMapi.h"
#include "api/aclif.h"
#include "common/HPM.h"
#include "common/cbasetypes.h"
#include "common/conf.h"
#include "common/console.h"
#include "common/core.h"
#include "common/ers.h"
#include "common/grfio.h"
#include "common/md5calc.h"
#include "common/memmgr.h"
#include "common/nullpo.h"
#include "common/random.h"
#include "common/showmsg.h"
#include "common/socket.h"
#include "common/sql.h"
#include "common/strlib.h"
#include "common/timer.h"
#include "common/utils.h"
#include "api/handlers.h"
#include "api/httpparser.h"
#include "api/httpsender.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif

static struct api_interface api_s;

struct api_interface *api;


int do_final(void)
{
	HPM->event(HPET_FINAL);

	aclif->final();

	HPM_api_do_final();

	HPM->event(HPET_POST_FINAL);

	ShowStatus("Finished.\n");
	return api->retval;
}

//------------------------------
// Function called when the server
// has received a crash signal.
//------------------------------
void do_abort(void)
{
	static int run = 0;
	//Save all characters and then flush the inter-connection.
	if (run) {
		ShowFatalError("Server has crashed while trying to save characters. Character data can't be saved!\n");
		return;
	}
	run = 1;
	ShowError("Server received crash signal! Attempting to save all online characters!\n");
}

void set_server_type(void)
{
	SERVER_TYPE = SERVER_TYPE_MAP;
}

/// Called when a terminate signal is received.
static void do_shutdown(void)
{
	if (core->runflag != APISERVER_ST_SHUTDOWN)
	{
		core->runflag = APISERVER_ST_SHUTDOWN;
		ShowStatus("Shutting down...\n");
		sockt->flush_fifos();
		core->runflag = CORE_ST_STOP;
	}
}

static void api_load_defaults(void)
{
	api_defaults();
	aclif_defaults();
	httpparser_defaults();
	httpsender_defaults();
}

/**
 * Defines the local command line arguments
 */
void cmdline_args_init_local(void)
{
}

int do_init(int argc, char *argv[])
{
	bool minimal = false;

#ifdef GCOLLECT
	GC_enable_incremental();
#endif

	api_load_defaults();
	handlers_defaults();

	HPM_api_do_init();
//	cmdline->exec(argc, argv, CMDLINE_OPT_PREINIT);

	HPM->event(HPET_PRE_INIT);

	minimal = api->minimal;/* temp (perhaps make minimal a mask with options of what to load? e.g. plugin 1 does minimal |= mob_db; */
	if (!minimal) {

		if (!api->ip_set || !api->char_ip_set) {
			char ip_str[16];
			sockt->ip2str(sockt->addr_[0], ip_str);

			ShowInfo("Defaulting to %s as our IP address\n", ip_str);

			if (!api->ip_set)
				aclif->setip(ip_str);
		}
	}

	handlers->init(minimal);
	aclif->init(minimal);
	httpparser->init(minimal);

	if( minimal ) {
		HPM->event(HPET_READY);
		exit(EXIT_SUCCESS);
	}

	Sql_HerculesUpdateCheck(api->mysql_handle);

	ShowStatus("Server is '"CL_GREEN"ready"CL_RESET"' and listening on port '"CL_WHITE"%d"CL_RESET"'.\n\n", api->port);

	if( core->runflag != CORE_ST_STOP ) {
		core->shutdown_callback = api->do_shutdown;
		core->runflag = APISERVER_ST_RUNNING;
	}

	HPM->event(HPET_READY);

	return 0;
}

/*=====================================
 * Default Functions : api.h
 * Generated by HerculesInterfaceMaker
 * created by Susu
 *-------------------------------------*/
void api_defaults(void)
{
	api = &api_s;

	/* */
	api->minimal = false;
	api->count = 0;
	api->retval = EXIT_SUCCESS;

	api->server_port = 3306;
	sprintf(api->server_ip,"127.0.0.1");
	sprintf(api->server_id,"ragnarok");
	sprintf(api->server_pw,"ragnarok");
	sprintf(api->server_db,"ragnarok");
	api->mysql_handle = NULL;

	api->port = 7000;
	api->ip_set = 0;
	api->char_ip_set = 0;

	api->do_shutdown = do_shutdown;
}
