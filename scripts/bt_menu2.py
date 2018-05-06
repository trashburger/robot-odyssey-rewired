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

bt_common.patchFramebufferTrace(b)

# Dynamic branch for cutscene sound effects
b.patchDynamicBranch('019E:068B', [
    sbt86.Addr16(str='019E:069B'),
    sbt86.Addr16(str='019E:06E3'), 
    sbt86.Addr16(str='019E:06BE')
])

# This isn't actually self-modifying code, but some data stored in the code segment
# will trigger some warnings that we can silence by marking the area explicitly as data.
b.patchDynamicLiteral('019E:0434', length=2)
b.patchDynamicLiteral('019E:0436', length=2)

# The main menu is a subroutine with a single caller; inline it to make cutting the control flow easier
b.patch('019E:0108', 'jmp 0x22d')
b.patch('019E:0267', 'jmp 0x10b')

b.writeCodeToFile(os.path.join(basedir, 'bt_menu2.cpp'), 'Menu2EXE')
