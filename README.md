OPS-SYSMOND
===========

What is ops-sysmond?
--------------------
The `ops-sysmond` python module provides system monitoring updates to OpenvSwitchDatabase on a periodic basisc. 

What utilities are used to capture process information ?
--------------------------------------------------------
We are using linux commands like `ps` and `top` to display global system CPU and memory usage.

What is the structure of the repository?
----------------------------------------
- `ops-sysmond` The Python source files are under this subdirectory.
- `./tests/` - This directory contains the component tests of `ops-sysmond` based on the topology framework.

What is the license?
--------------------
Apache 2.0 license. For more details refer to [COPYING](https://git.openswitch.net/cgit/openswitch/ops-sysmond/tree/COPYING).

