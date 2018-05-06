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

# Remove modal "Insert disk 1" message on save-file load failure
b.patch('0D63:5CFB', 'jmp 0x5D14')

# Remove model "A disk error has occurred"
b.patch('0D63:5E3E', 'ret')

# Remove modal "Insert disk 1" message before exiting back to menu
b.patch('0D63:5FFD', 'jmp 0x6016')

# Break control flow at remaining locations that wait for keyboard
for call_site in [
    '0D63:820B',    # Related to "?" key?
    '0D63:26D7',    # Menu for "?" key
    '0D63:26E4',    # Menu for "ESC" key
]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(1)
    subroutine = b.jumpTarget(call_site)
    b.patchAndHook(call_site, 'ret', 'proc->continue_from(r, &sub_%X);' % continue_at.linear)
    b.patch(continue_at, 'call 0x%04x' % subroutine.offset, length=2)
    b.exportSub(continue_at)

b.writeCodeToFile(os.path.join(basedir, 'bt_lab.cpp'), 'LabEXE')
