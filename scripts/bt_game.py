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

# Remove modal "Insert disk 1" message on world load failure
b.patchAndHook('0E3B:2CBE', 'ret', 'assert(0 && "World file load failure");')

# Remove modal "A disk error has occurred"
b.patch('0E3B:2E03', 'ret')

# Remove modal "Insert disk 1" message before exiting back to menu
b.patch('0E3B:2E83', 'jmp 0x2E9C')

# Remove modal "Insert storage disk" message for loading/saving chips
b.patch('0E3B:2EA9', 'jmp 0x2EDC')

# Break control flow at remaining locations that wait for keyboard
for call_site in [
    '0E3B:3B8D',    # Menu for "?" key
    '0E3B:3B9A',    # Menu for "ESC" key
    '0E3B:6921',    # "To go back to the room you just left, press Enter"
]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(1)
    subroutine = b.jumpTarget(call_site)
    b.patchAndHook(call_site, 'ret',
    	'hw->outputDelay(20);'
		'proc->continueFrom(r, &sub_%X);' % continue_at.linear)
    b.patch(continue_at, 'call 0x%04x' % subroutine.offset, length=2)
    b.exportSub(continue_at)

# Break control flow in the transporter animation loop,
# it's too long to store in the output queue.
for call_site in [
	'0E3B:6E77',  # Main transporter animation
]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(1)
    subroutine = b.jumpTarget(call_site)
    b.patchAndHook(call_site, 'ret', 'proc->continueFrom(r, &sub_%X);' % continue_at.linear)
    b.patch(continue_at, 'call 0x%04x' % subroutine.offset, length=2)
    b.exportSub(continue_at)

# Skip the disk prompts after the transporter animation
b.patch('0E3B:691C', 'jmp 0x692D')

b.writeCodeToFile(os.path.join(basedir, 'bt_game.cpp'), 'GameEXE')
