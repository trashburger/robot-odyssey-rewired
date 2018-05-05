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

# Remove modal "Insert disk 1" message before exiting back to menu
b.patch('09FF:3B0F', 'jmp 0x3B28')

# Break control flow at remaining locations that wait for keyboard
for call_site in [
    '09FF:405C',     # Menu for help "?" key
    '09FF:4069',     # Menu for "ESC" key
]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(1)
    subroutine = b.jumpTarget(call_site)
    b.patchAndHook(call_site, 'ret', 'proc->continue_from(r, &sub_%X);' % continue_at.linear)
    b.patch(continue_at, 'call 0x%04x' % subroutine.offset, length=2)
    b.exportSub(continue_at)

b.writeCodeToFile(os.path.join(basedir, 'bt_tutorial.cpp'), 'TutorialEXE')
