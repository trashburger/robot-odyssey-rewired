#!/usr/bin/env python3
#
# Patches and hooks for the binary translation of PLAY.EXE.
# Micah Elizabeth Scott <micah@scanlime.org>
#

import sys
import sbt86

b = sbt86.DOSBinary('build/play.exe')

b.writeCodeToFile('build/bt_play.cpp', "play_main")
