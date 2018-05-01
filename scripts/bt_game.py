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

b.decl("#include <stdio.h>")
b.patchAndHook(b.findCode('2c01 :2f a2____ a2____ b12c 32ed'),
               'nop', length=1, cCode='''
   sassert(false, "Unimplemented DAS instruction\\n");
''')

b.writeCodeToFile(os.path.join(basedir, 'bt_game.rs'), 'GameEXE')