#!/usr/bin/env python3
#
# Patches and hooks for the binary translation of TUT.EXE.
# Micah Elizabeth Scott <micah@scanlime.org>
#

import sbt86
import bt_common
import os
import sys

basedir = sys.argv[1]
b = sbt86.DOSBinary(os.path.join(basedir, 'tut.exe'))

bt_common.patch(b)
#bt_common.findSelfModifyingCode(b)

# Debug: Trace all IPs which access memory (for diagnosing hangs)
#b.trace('rw', 'return 1;', 'printf("%04x ", ip);')

b.writeCodeToFile(os.path.join(basedir, 'bt_tutorial.cpp'), 'TutorialEXE')
