# (C) Copyright 2016 Hewlett Packard Enterprise Development LP
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
#    under the License.
#
##########################################################################

"""
OpenSwitch Test for verifying output of top commands
"""

TOPOLOGY = """
# +-------+
# |  ops1 |
# +-------+

# Nodes
[type=openswitch name="OpenSwitch 1"] ops1

# Links
"""


def test_top_command(topology, step):
    ops1 = topology.get('ops1')
#   Maximum cpu usage in terms of percentage
    max_cpu_usage_of_a_process = 30
#   Maximum memory usage in terms of percentage
    max_memory_usage_of_a_process = 10

    assert ops1 is not None

    step("Step 1: Test top cpu command")

    output = ops1("top cpu")
    lines = output.split('\n')
    assert len(lines) > 0,\
        'Test top cpu command - Failed'

    step("Test top cpu output to spot core processes available")
    check_point = 0
    for line in lines:
        if "systemd-journald" in line or "dbus-daemon" in line or \
           "rsyslogd" in line or "system-logind" in line or \
           "ovsdb-server" in line:
            check_point += 1
            print(line)
    print("Number of check points achieved %d" % (check_point))
    assert check_point == 5,\
        'Test top cpu command output to spot core processes - Failed'

    step("Test top cpu and spot high cpu processes")
    top_process_cpu_usage = int(lines[7].split()[8].split(".")[0])
    if top_process_cpu_usage >= max_cpu_usage_of_a_process:
        check_point += 1
    print("Number of check points achieved %d" % (check_point))
    assert check_point == 5,\
        'Test top cpu command output to spot core processes - Failed'

    step("Step 2: Test top memory command")

    output = ops1("top memory")
    lines = output.split('\n')
    assert len(lines) > 0,\
        'Test top memory command - Failed'

    step("Test top memory output to spot core processes available")
    check_point = 0
    for line in lines:
        if "systemd-journald" in line or "dbus-daemon" in line or \
           "rsyslogd" in line or "system-logind" in line or \
           "ovsdb-server" in line:
            check_point += 1
            print(line)
    print("Number of check points achieved %d" % (check_point))
    assert check_point == 5,\
        'Test top memory command output to spot core processes - Failed'

    step("Test top memory and spot high memory usage processes")
    top_process_memory_usage = int(lines[7].split()[8].split(".")[0])
    if top_process_memory_usage >= max_memory_usage_of_a_process:
        check_point += 1
    print("Number of check points achieved %d" % (check_point))
    assert check_point == 5,\
        'Test top memory command output to spot \
        high memory usage processes - Failed'
