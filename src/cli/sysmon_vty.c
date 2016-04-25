/* CLI commands
 *
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP
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
 * File: top_cpu_vty.c
 *
 * Purpose: To add top_cpu CLI commands.
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

VLOG_DEFINE_THIS_MODULE(top_cpu_vty);

/*Global variables*/
extern struct ovsdb_idl *idl;
#define AVG_1_MIN "avg_for_last_1_min"
#define AVG_5_MIN "avg_for_last_5_min"
#define AVG_15_MIN "avg_for_last_15_min"
#define USER_PROCESS_RUN "user_process_run"
#define KERNEL_PROCESS_RUN "kernel_process_run"
#define WAIT_FOR_IO_COMPLETION "waiting_on_io_completion"
#define SERVICING_HW_INTERRUPTS "servicing_hw_interrupts"
#define SERVICING_SW_INTERRUPTS "servicing_sw_interrupts"
#define PHYS_TOTAL_MEMORY_SIZE "phys_total_memory_size"
#define PHYS_FREE_MEMORY_SIZE "phys_free_memory_size"
#define PHYS_USED_MEMORY_SIZE "phys_used_memory_size"
#define PHYS_BUFFER_SIZE "phys_buffer_size"
#define VIRT_TOTAL_MEMORY_SIZE "virt_total_memory_size"
#define VIRT_FREE_MEMORY_SIZE "virt_free_memory_size"
#define VIRT_USED_MEMORY_SIZE "virt_used_memory_size"
#define VIRT_BUFFER_SIZE "virt_buffer_size"
#define PNAME "pname"
#define USER "user"
#define VIRTUAL_MEMORY_SIZE "virtual_memory_size"
#define RESIDENT_MEMORY_SIZE "resident_memory_size"
#define SHARED_MEMORY_SIZE "shared_memory_size"
#define PROCESS_STATUS "process_status"
#define MEMORY_USAGE "memory_usage"
#define LAST_CPU_USAGE "last_cpu_usage"
#define DEFAULT_UTIL "0.0"

/*================================================================================================*/
/* TOP CLI Implementations */
static void
vtysh_top_memory_handler()
{
    char *argv[] = { "-b", "-n", "1", "-s", "-c", "-o", "%MEM", "-w", "110"};
    int argc = 9;
    execute_command("top", argc, (const char **) argv);
}

static void
vtysh_top_cpu_handler()
{
    char *argv[] = { "-b", "-n", "1", "-s", "-c", "-o", "%CPU", "-w", "110"};
    int argc = 9;
    execute_command("top", argc, (const char **) argv);
}


/*================================================================================================*/
/* SHOW CLI Implementations */

static void
vtysh_show_system_memory_handler()
{
    const struct ovsrec_system *ovs_system = NULL;
    const struct ovsrec_system_health *ovs_system_health = NULL;
    const struct ovsrec_process_info *ovs_process_info= NULL;
    const char *buf = NULL;
    const char *pname_buf = NULL;
    const char *user_buf = NULL;
    const char *virt_mem_size_buf = NULL;
    const char *res_mem_size_buf = NULL;
    const char *shar_mem_size_buf = NULL;
    const char *process_status_buf = NULL;
    const char *mem_usage_buf = NULL;

    /* Get access to the System Table */
    ovs_system = ovsrec_system_first(idl);
    if (NULL == ovs_system) {
         vty_out(vty, "Could not access the System Table\n");
         return;
    }

    OVSREC_SYSTEM_HEALTH_FOR_EACH(ovs_system_health, idl) {
      vty_out(vty, "Physical memory (in KiB)\n");
      vty_out(vty, "-------------------------\n");
      buf = smap_get(&ovs_system_health->memory_usage, PHYS_TOTAL_MEMORY_SIZE);
      vty_out(vty, "Total memory        : %s\n", ((buf) ? buf : DEFAULT_UTIL));
      buf = smap_get(&ovs_system_health->memory_usage, PHYS_FREE_MEMORY_SIZE);
      vty_out(vty, "Total free memory   : %s\n", ((buf) ? buf : DEFAULT_UTIL));
      buf = smap_get(&ovs_system_health->memory_usage, PHYS_USED_MEMORY_SIZE);
      vty_out(vty, "Total used memory   : %s\n", ((buf) ? buf : DEFAULT_UTIL));
      buf = smap_get(&ovs_system_health->memory_usage, PHYS_BUFFER_SIZE);
      vty_out(vty, "Total buffers       : %s\n", ((buf) ? buf : DEFAULT_UTIL));

      vty_out(vty, "Virtual memory (in KiB)\n");
      vty_out(vty, "-------------------------\n");
      buf = smap_get(&ovs_system_health->memory_usage, VIRT_TOTAL_MEMORY_SIZE);
      vty_out(vty, "Total memory        : %s\n", ((buf) ? buf : DEFAULT_UTIL));
      buf = smap_get(&ovs_system_health->memory_usage, VIRT_FREE_MEMORY_SIZE);
      vty_out(vty, "Total free memory   : %s\n", ((buf) ? buf : DEFAULT_UTIL));
      buf = smap_get(&ovs_system_health->memory_usage, VIRT_USED_MEMORY_SIZE);
      vty_out(vty, "Total used memory   : %s\n", ((buf) ? buf : DEFAULT_UTIL));
      buf = smap_get(&ovs_system_health->memory_usage, VIRT_BUFFER_SIZE);
      vty_out(vty, "Total buffers       : %s\n", ((buf) ? buf : DEFAULT_UTIL));
    }

    vty_out(vty, "\n\n");
    vty_out(vty, "%9s %20s %10s %13s %12s %10s %10s %15s\n","PID", "PROCESS_NAME",\
        "USER", "VIRT_MEM_SIZE", "RES_MEM_SIZE", "STATUS", "MEM_USAGE", "SHARED_MEM_SIZE");
    vty_out(vty, "------------------------------------------------------------\
        ----------------------------------------------\n");

    OVSREC_PROCESS_INFO_FOR_EACH(ovs_process_info, idl) {
      pname_buf = smap_get(&ovs_process_info->details, PNAME);
      user_buf = smap_get(&ovs_process_info->details, USER);
      virt_mem_size_buf = smap_get(&ovs_process_info->details, VIRTUAL_MEMORY_SIZE);
      res_mem_size_buf = smap_get(&ovs_process_info->details, RESIDENT_MEMORY_SIZE);
      shar_mem_size_buf = smap_get(&ovs_process_info->details, SHARED_MEMORY_SIZE);
      process_status_buf = smap_get(&ovs_process_info->details, PROCESS_STATUS);
      mem_usage_buf = smap_get(&ovs_process_info->details, MEMORY_USAGE);
      vty_out(vty, "%9ld %20s %10s %13s %12s %10s %10s %15s\n",ovs_process_info->pid, \
          pname_buf,user_buf,\
          virt_mem_size_buf,res_mem_size_buf,\
          process_status_buf,mem_usage_buf,\
          shar_mem_size_buf);
    }

}


static void
vtysh_show_system_cpu_handler()
{
    const struct ovsrec_system *ovs_system = NULL;
    const struct ovsrec_system_health *ovs_system_health = NULL;
    const struct ovsrec_process_info *ovs_process_info= NULL;
    const char *buf = NULL;
    const char *pname_buf = NULL;
    const char *user_buf = NULL;
    const char *process_status_buf = NULL;
    const char *last_cpu_usage_buf = NULL;

    /* Get access to the System Table */
    ovs_system = ovsrec_system_first(idl);
    if (NULL == ovs_system) {
         vty_out(vty, "Could not access the System Table\n");
         return;
    }

    OVSREC_SYSTEM_HEALTH_FOR_EACH(ovs_system_health, idl) {
      vty_out(vty, "CPU Average (in percentage)\n");
      vty_out(vty, "--------------------------\n");
      buf = smap_get(&ovs_system_health->cpu_usage, AVG_1_MIN);
      vty_out(vty, "Average for last 1  min                      : %s\n", ((buf) ? buf : DEFAULT_UTIL));
      buf = smap_get(&ovs_system_health->cpu_usage, AVG_5_MIN);
      vty_out(vty, "Average for last 5  min                      : %s\n", ((buf) ? buf : DEFAULT_UTIL));
      buf = smap_get(&ovs_system_health->cpu_usage, AVG_15_MIN);
      vty_out(vty, "Average for last 15 min                      : %s\n", ((buf) ? buf : DEFAULT_UTIL));
      buf = smap_get(&ovs_system_health->cpu_usage, USER_PROCESS_RUN);
      vty_out(vty, "CPU util to run user applications            : %s\n", ((buf) ? buf : DEFAULT_UTIL));
      buf = smap_get(&ovs_system_health->cpu_usage, KERNEL_PROCESS_RUN);
      vty_out(vty, "CPU util to run kernel processes             : %s\n", ((buf) ? buf : DEFAULT_UTIL));
      buf = smap_get(&ovs_system_health->cpu_usage, WAIT_FOR_IO_COMPLETION);
      vty_out(vty, "CPU idle time waiting for I/O completion     : %s\n", ((buf) ? buf : DEFAULT_UTIL));
      buf = smap_get(&ovs_system_health->cpu_usage, SERVICING_HW_INTERRUPTS);
      vty_out(vty, "CPU util for servicing h/w interrupts        : %s\n", ((buf) ? buf : DEFAULT_UTIL));
      buf = smap_get(&ovs_system_health->cpu_usage, SERVICING_SW_INTERRUPTS);
      vty_out(vty, "CPU util for servicing s/w interrupts        : %s\n", ((buf) ? buf : DEFAULT_UTIL));
    }

    vty_out(vty, "\n\n");
    vty_out(vty, "%9s %20s %10s %15s %14s\n","PID", "PROCESS_NAME",\
        "USER", "STATUS","LAST_CPU_USAGE");
    vty_out(vty, "--------------------------------------------------------------------n");

    OVSREC_PROCESS_INFO_FOR_EACH(ovs_process_info, idl) {
      pname_buf = smap_get(&ovs_process_info->details, PNAME);
      user_buf = smap_get(&ovs_process_info->details, USER);
      process_status_buf = smap_get(&ovs_process_info->details, PROCESS_STATUS);
      last_cpu_usage_buf = smap_get(&ovs_process_info->details, LAST_CPU_USAGE);
      vty_out(vty, "%9ld %20s %10s %15s %14s\n",ovs_process_info->pid,pname_buf,user_buf,\
          process_status_buf,\
          last_cpu_usage_buf);
    }

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

DEFUN ( vtysh_set_monitor_poll_timer,
        vtysh_set_monitor_poll_timer_cmd,
        "monitor poll-timer <5-15>",
        MONITOR_DISPLAY_STR
        POLL_TIMER_DISPLAY_STR
        POLL_TIMER_RANGE_DISPLAY_STR
      )
{
    /*
     * int ret_code = CMD_SUCCESS;
     */
    /*
     * ntp_cli_ntp_auth_enable_params_t ntp_auth_enable_params;
     * ntp_auth_enable_get_default_parameters(&ntp_auth_enable_params);
     *
     * if (vty_flags & CMD_FLAG_NO_CMD) {
     *     ntp_auth_enable_params.no_form = 1;
     * }
     *
     * ret_code = vtysh_ovsdb_ntp_auth_enable_set(&ntp_auth_enable_params);
     */
    /*
     * return ret_code;
     */
    return CMD_SUCCESS;
}


DEFUN_NO_FORM ( vtysh_set_monitor_poll_timer,
        vtysh_set_monitor_poll_timer_cmd,
        "monitor poll-timer <5-15>",
        MONITOR_DISPLAY_STR
        POLL_TIMER_DISPLAY_STR
        POLL_TIMER_RANGE_DISPLAY_STR
      );


DEFUN_NOLOCK ( vtysh_top_cpu,
        vtysh_top_cpu_cmd,
        "top cpu",
        TOP_DISPLAY_STR
        CPU_DISPLAY_STR
      )
{
    vtysh_top_cpu_handler();
    return CMD_SUCCESS;
}

DEFUN_NOLOCK ( vtysh_top_memory,
        vtysh_top_memory_cmd,
        "top memory",
        TOP_DISPLAY_STR
        MEMORY_DISPLAY_STR
      )
{
    vtysh_top_memory_handler();
    return CMD_SUCCESS;
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
    install_element (VIEW_NODE, &vtysh_top_cpu_cmd);
    install_element (ENABLE_NODE, &vtysh_top_cpu_cmd);

    install_element (VIEW_NODE, &vtysh_top_memory_cmd);
    install_element (ENABLE_NODE, &vtysh_top_memory_cmd);

    install_element (VIEW_NODE, &vtysh_show_system_cpu_cmd);
    install_element (ENABLE_NODE, &vtysh_show_system_cpu_cmd);

    install_element (VIEW_NODE, &vtysh_show_system_memory_cmd);
    install_element (ENABLE_NODE, &vtysh_show_system_memory_cmd);

    install_element (CONFIG_NODE, &vtysh_set_monitor_poll_timer_cmd);
    install_element (CONFIG_NODE, &no_vtysh_set_monitor_poll_timer_cmd);

}

/*================================================================================================*/
