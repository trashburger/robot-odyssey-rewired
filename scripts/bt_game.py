#!/usr/bin/env python3
#
# Patches and hooks for the binary translation of GAME.EXE.
# Micah Elizabeth Scott <micah@scanlime.org>
#

import sys
import os
import sbt86
import bt_common

basedir = sys.argv[1]
b = sbt86.DOSBinary(os.path.join(basedir, 'game.exe'))

bt_common.patch(b)
bt_common.patchChips(b)
bt_common.patchLoadSave(b)

b.writeCodeToFile(os.path.join(basedir, 'bt_game.cpp'), 'GameEXE')