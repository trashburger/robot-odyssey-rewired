#!/usr/bin/env python3
#
# Patches and hooks for the binary translation of LAB.EXE.
# Micah Elizabeth Scott <micah@scanlime.org>
#

import os
import sys
import sbt86
import bt_common

basedir = sys.argv[1]
b = sbt86.DOSBinary(os.path.join(basedir, 'lab.exe'))

bt_common.patch(b)
bt_common.patchChips(b)
bt_common.patchLoadSave(b)

b.writeCodeToFile(os.path.join(basedir, 'bt_lab.rs'), 'LabEXE')
