#!/usr/bin/env python
#
# Patches and hooks for the binary translation of PLAY.EXE.
# Micah Elizabeth Scott <micah@misc.name>
#

import sbt86

b = sbt86.DOSBinary('PLAY.EXE')

b.writeCodeToFile("bt_play.c", "play_main")
