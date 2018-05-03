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

# Break control flow in any loop that's waiting for keyboard input

for call_site in [
    '09FF:3B1E',
    '09FF:405C',
    '09FF:4069',
]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(1)
    subroutine = b.jumpTarget(call_site)
    b.patchAndHook(call_site, 'ret', 'proc->continue_from(r, &sub_%X);' % continue_at.linear)
    b.patch(continue_at, 'call 0x%04x' % subroutine.offset, length=2)
    b.exportSub(continue_at)

b.writeCodeToFile(os.path.join(basedir, 'bt_tutorial.cpp'), 'TutorialEXE')
