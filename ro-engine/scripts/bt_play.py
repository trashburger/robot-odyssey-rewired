#!/usr/bin/env python3
#
# Patches and hooks for the binary translation of PLAY.EXE.
# Micah Elizabeth Scott <micah@scanlime.org>
#

import sys
import os
import sbt86

basedir = sys.argv[1]
b = sbt86.DOSBinary(os.path.join(basedir, 'play.exe'))

b.writeCodeToFile(os.path.join(basedir, 'bt_play.rs'), 'PlayEXE')
