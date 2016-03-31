/* CLI commands
 *
 * Copyright (C) 2015-2016 Hewlett Packard Enterprise Development LP
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * File: show_system_cpu_vty.c
 *
 * Purpose: To add show_system_cpu CLI commands.
 */
#include <sys/wait.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "vtysh/command.h"
#include "memory.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "sysmond_vty.h"
#include "smap.h"
#include "openvswitch/vlog.h"
#include "openswitch-idl.h"
#include "ovsdb-data.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "vtysh/vtysh_ovsdb_if.h"

VLOG_DEFINE_THIS_MODULE(show_system_cpu_vty);
/*-----------------------------------------------------------------------------
| Name : printshow_system_cpuOutput
| Responsibility : Display show_system_cpu output
| Parameters : char* buff : pointer to char
| Return : void
-----------------------------------------------------------------------------*/
void print_output(char * buff)
{
    vty_out (vty, "%s", buff);
}

/*================================================================================================*/
/* SHOW CLI Implementations */
static void
vtysh_show_system_memory_handler()
{
    char output[VTY_BUFSIZ];
    FILE *fp = NULL;

    fp = popen("top -b -n 1 -s -c -o %MEM -w 110","w");
    if (fp)
    {
        while ( fgets( output, VTY_BUFSIZ, fp ) != NULL )
            vty_out (vty, "%s", output);
    }
    else
    {
        VLOG_ERR("Failed to open pipe stream");
        return;
    }
    pclose(fp);
}

static void
vtysh_show_system_cpu_handler()
{
    char output[VTY_BUFSIZ];
    FILE *fp = NULL;

    fp = popen("top -b -n 1 -s -c -o %CPU -w 110","w");
    if (fp)
    {
        while ( fgets( output, VTY_BUFSIZ, fp ) != NULL )
            vty_out (vty, "%s", output);
    }
    else
    {
        VLOG_ERR("Failed to open pipe stream");
        return;
    }
    pclose(fp);

}
/*================================================================================================*/
/* CLI Definitions */

DEFUN ( vtysh_show_system_cpu,
        vtysh_show_system_cpu_cmd,
        "show system cpu",
        SHOW_STR
        SYSTEM_DISPLAY_STR
        CPU_DISPLAY_STR
      )
{
    vtysh_show_system_cpu_handler();
    return CMD_SUCCESS;
}

DEFUN ( vtysh_show_system_memory,
        vtysh_show_system_memory_cmd,
        "show system memory",
        SHOW_STR
        SYSTEM_DISPLAY_STR
        MEMORY_DISPLAY_STR
      )
{
    vtysh_show_system_memory_handler();
    return CMD_SUCCESS;
}

/*================================================================================================*/

/*
 * Function       : sysmond_vty_init
 * Responsibility : install all the CLIs in the respective contexts.
 */

void
sysmond_vty_init (void)
{
    install_element (VIEW_NODE, &vtysh_show_system_cpu_cmd);
    install_element (ENABLE_NODE, &vtysh_show_system_cpu_cmd);

    install_element (VIEW_NODE, &vtysh_show_system_memory_cmd);
    install_element (ENABLE_NODE, &vtysh_show_system_memory_cmd);

}

/*================================================================================================*/

/* Initialize ops-sysmond cli node.
 */
void cli_pre_init(void)
{
    /* ops-sysmond doesn't have any context level cli commands.
     * To load ops-sysmond cli shared libraries at runtime, this function is required.
     */
}

/* Initialize ops-sysmond cli element.
 */
void cli_post_init(void)
{
    install_element (VIEW_NODE, &vtysh_show_system_cpu_cmd);
    install_element (ENABLE_NODE, &vtysh_show_system_cpu_cmd);

    install_element (VIEW_NODE, &vtysh_show_system_memory_cmd);
    install_element (ENABLE_NODE, &vtysh_show_system_memory_cmd);

}

/*================================================================================================*/
