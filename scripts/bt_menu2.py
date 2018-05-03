#!/usr/bin/env python3
#
# Patches and hooks for the binary translation of MENU2.EXE.
# Micah Elizabeth Scott <micah@scanlime.org>
#

import os
import sys
import sbt86
import bt_common

basedir = sys.argv[1]
b = sbt86.DOSBinary(os.path.join(basedir, 'menu2.exe'))

bt_common.patchMenu(b)

# XXX: Dynamic branch, looks sound related.
b.patch('010E:068B', 'nop', 2)

b.writeCodeToFile(os.path.join(basedir, 'bt_menu2.cpp'), 'Menu2EXE')
