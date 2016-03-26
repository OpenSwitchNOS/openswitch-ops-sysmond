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

import ovs.dirs
import ovs.daemon
import ovs.db.idl
import ovs.unixctl
import ovs.unixctl.server

# OVS definitions
idl = None
POLL_TIMER_SLEEP_INTERVAL = 5

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


def check_config_updates():
    pass


def prov_process_info():
    global idl
    global seqno
    global ovsrec_system_health, pid

    # add system health table row
    txn = ovs.db.idl.Transaction(idl)
    for shrow in idl.tables[SYSTEM_HEALTH_TABLE].rows.itervalues():
        shrow.__setattr__("process_info", [])
    for i in xrange(100):
        pid += 1
        ovsrec_sys_health = txn.insert(idl.tables[PROCESS_INFO_TABLE])
        ovsrec_sys_health.__setattr__("pid", pid)
        ovsrec_system_health = ovsrec_sys_health
        for shrow in idl.tables[SYSTEM_HEALTH_TABLE].rows.itervalues():
            e_sys_health_ref = shrow.__getattr__("process_info")
            e_sys_health_ref.append(ovsrec_sys_health.uuid)
            shrow.__setattr__("process_info", e_sys_health_ref)


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
        prov_process_info()
        sleep(POLL_TIMER_SLEEP_INTERVAL)

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
