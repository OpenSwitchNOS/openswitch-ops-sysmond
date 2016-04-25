#!/usr/bin/env python
# (C) Copyright 2015 Hewlett Packard Enterprise Development LP
# All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License"); you may
#    not use this file except in compliance with the License. You may obtain
#    a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#    License for the specific language governing permissions and limitations
#    under the License..

import argparse
import sys
from time import sleep
import os
import ovs.dirs
import ovs.daemon
import ovs.db.idl
import ovs.unixctl
import ovs.unixctl.server
from pprint import pformat
from pprint import pprint

# OVS definitions
idl = None
POLL_TIMER_SLEEP_INTERVAL = 2
POLL_TIMER_SLEEP_INTERVAL_FOR_UPDATES = 10

# Tables definitions
SYSTEM_HEALTH_TABLE = 'System_Health'
SYSTEM_TABLE = 'System'
PROCESS_INFO_TABLE = 'Process_Info'

SYSTEM_TBL_CUR_CFG_COL = "cur_cfg"
SYSTEM_TBL_SYSTEM_HEALTH_COL = "system_health"
SYSTEM_HEALTH_TBL_CPU_USAGE_COL = 'cpu_usage'
SYSTEM_HEALTH_TBL_MEMORY_USAGE_COL = 'memory_usage'

# Default DB path
def_db = 'unix:/var/run/openvswitch/db.sock'

# OPS_TODO: Need to pull these from the build env
ovs_schema = '/usr/share/openvswitch/vswitch.ovsschema'

vlog = ovs.vlog.Vlog("ops-sysmond")
exiting = False
seqno = 0

global pid, ovsrec_system_health
global system_info
ovsrec_system_health = None
pid = 1


def unixctl_exit(conn, unused_argv, unused_aux):
    global exiting
    exiting = True
    conn.reply(None)

# ------------------ db_get_system_status() ----------------


def db_get_system_status(data):
    '''
    Checks if the system initialization is completed.
    If System:cur_cfg > 0:
        configuration completed: return True
    else:
        return False
    '''
    for ovs_rec in data[SYSTEM_TABLE].rows.itervalues():
        if ovs_rec.cur_cfg:
            if ovs_rec.cur_cfg == 0:
                return False
            else:
                return True
    return False

# ------------------ system_is_configured() ----------------


def system_is_configured():
    global idl

    # Check the OVS-DB/File status to see if initialization has completed.
    if not db_get_system_status(idl.tables):
        return False

    return True

# ------------------ terminate() ----------------


def terminate():
    global exiting
    # Exiting daemon
    exiting = True

# ------------------ ops_sysmond_init() ----------------


def ops_sysmond_registration(remote):
    '''
    Initializes the OVS-DB connection
    '''

    global idl

    schema_helper = ovs.db.idl.SchemaHelper(location=ovs_schema)
    schema_helper.register_columns(SYSTEM_TABLE,
                                   [SYSTEM_TBL_CUR_CFG_COL,
                                    SYSTEM_TBL_SYSTEM_HEALTH_COL])
    schema_helper.register_table(SYSTEM_HEALTH_TABLE)
    schema_helper.register_table(PROCESS_INFO_TABLE)
    idl = ovs.db.idl.Idl(remote, schema_helper)


def sync_updates():

    pass

global p_dict
p_dict = {}


def get_process_info(x):
    y = x.split()

    p_dict[int(y[0])] = {
        "pname": y[11],
        "user": y[1],
        "virtual_memory_size": y[4],
        "resident_memory_size": y[5],
        "shared_memory_size": y[6],
        "process_status": y[7],
        "memory_usage": y[9],
        "last_cpu_usage": y[8]
    }
    return True


def get_system_info(usage_data):
    u = usage_data[0].split()
    v = usage_data[2].split()
    cpu_usage = {
        "avg_for_last_1_min": u[-3].split(",")[0],
        "avg_for_last_5_min": u[-2].split(",")[0],
        "avg_for_last_15_min": u[-1].split(",")[0],
        "user_process_run": v[1],
        "kernel_process_run": v[3],
        "waiting_on_io_completion": v[9],
        "servicing_hw_interrupts": v[11],
        "servicing_sw_interrupts": v[13]
    }
    u = usage_data[3].split()
    v = usage_data[4].split()
    memory_usage = {
        "phys_total_memory_size": u[3],
        "phys_free_memory_size": u[5],
        "phys_used_memory_size": u[7],
        "phys_buffer_size": u[9],
        "virt_total_memory_size": v[2],
        "virt_free_memory_size": v[4],
        "virt_used_memory_size": v[6],
        "virt_buffer_size": v[8]
    }
    return cpu_usage, memory_usage


def check_config_updates():
    global idl
    ovs_rec = None
    for ovs_rec in idl.tables[SYSTEM_HEALTH_TABLE].rows.itervalues():
        if ovs_rec.poll_timer and ovs_rec.poll_timer is not None:
            vlog.info("Poll timer value is %d " % (int(ovs_rec.poll_timer)))
    vlog.info("check_config_updates executed")
    pass


def generate_top_output():
    os.system("top -n 1 -b > /etc/top_output.txt")


def parse_top_output():
    global system_info
    with open("/etc/top_output.txt") as f:
        data = f.readlines()
        usage_data = data[:6]
        p_dict = {}
        cpu_usage, memory_usage = get_system_info(usage_data)
        system_info = (cpu_usage, memory_usage)
        process_listing = data[7:]
        map(get_process_info, process_listing)


def prov_process_info():
    global idl
    global seqno
    global ovsrec_system_health, pid
    global p_dict
    global system_info
    generate_top_output()
    parse_top_output()
    shrow = None
    # add system health table row
    txn = ovs.db.idl.Transaction(idl)
    for shrow in idl.tables[SYSTEM_HEALTH_TABLE].rows.itervalues():
        shrow.__setattr__("process_info", [])
    list_of_process_info_rows = []

    for ovs_process_info_row in idl.tables[PROCESS_INFO_TABLE].rows.itervalues():
        if ovs_process_info_row.pid and ovs_process_info_row is not None:
            if int(ovs_process_info_row.pid) in p_dict.keys():
                list_of_process_info_rows.append(ovs_process_info_row.uuid)
                setattr(ovs_process_info_row, 'details',
                        p_dict[int(ovs_process_info_row.pid)])
                del p_dict[int(ovs_process_info_row.pid)]

    for pid in p_dict.keys():
        ovs_process_info_row = txn.insert(idl.tables[PROCESS_INFO_TABLE])
        ovs_process_info_row.__setattr__("pid", pid)
        setattr(ovs_process_info_row, 'details', p_dict[pid])
        list_of_process_info_rows.append(ovs_process_info_row.uuid)

    if shrow is not None:
        # Set process info rows with system_health table
        e_sys_health_ref = shrow.__getattr__("process_info")
        for row in list_of_process_info_rows:
            e_sys_health_ref.append(row)
        shrow.__setattr__("process_info", e_sys_health_ref)
        setattr(shrow, 'cpu_usage', system_info[0])
        setattr(shrow, 'memory_usage', system_info[1])

    status = txn.commit_block()
    vlog.info("status for process_info, %s" % status)


# ------------------ main() ----------------
def ops_sysmond_init():

    global exiting
    global idl
    global seqno

    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--database', metavar="DATABASE",
                        help="A socket on which ovsdb-server is listening.",
                        dest='database')

    ovs.vlog.add_args(parser)
    ovs.daemon.add_args(parser)
    args = parser.parse_args()
    ovs.vlog.handle_args(args)
    ovs.daemon.handle_args(args)

    if args.database is None:
        remote = def_db
    else:
        remote = args.database

    ops_sysmond_registration(remote)

    ovs.daemon.daemonize()

    ovs.unixctl.command_register("exit", "", 0, 0, unixctl_exit, None)
    error, unixctl_server = ovs.unixctl.server.UnixctlServer.create(None)

    if error:
        ovs.util.ovs_fatal(error, "ops-sysmond : could not create "
                                  "unix-ctl server", vlog)
    idl.run()
    seqno = idl.change_seqno    # Sequence number when we last processed the db
    exiting = False
    time_since_last_sync = 0
    while not exiting:
        unixctl_server.run()
        if exiting:
            break
        idl.run()
        if seqno == idl.change_seqno:
            sync_updates()
        else:
            vlog.info("ops-sysmond main - seqno change from %d to %d "
                      % (seqno, idl.change_seqno))
            check_config_updates()
            seqno = idl.change_seqno
        if POLL_TIMER_SLEEP_INTERVAL_FOR_UPDATES < time_since_last_sync:
            prov_process_info()
            time_since_last_sync = 0
        sleep(POLL_TIMER_SLEEP_INTERVAL)
        time_since_last_sync += POLL_TIMER_SLEEP_INTERVAL

    # Daemon exit
    unixctl_server.close()
    idl.close()


if __name__ == '__main__':
    try:
        ops_sysmond_init()
    except SystemExit:
        # Let system.exit() calls complete normally
        raise
    except:
        vlog.exception("traceback")
        sys.exit(ovs.daemon.RESTART_EXIT_CODE)
