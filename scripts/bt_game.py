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
bt_common.patchJoystick(b)
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

# Break up subroutines control flow at the landing point that we reach
# when switching levels. This isn't required, but it will make the generated
# code smaller as it avoids inlining many additional copies of the main loops.
# Since there isn't a jump we can easily replace here, we'll replace a
# "mov si, 8" instruction with a smaller variant and a tail-call.
b.patch('0E3B:2C04', 'mov si, 8', length=1)
b.patch('0E3B:2C05', 'call 0x2C07', length=1)
b.patch('0E3B:2C06', 'ret')

# Break control flow at remaining locations that wait for keyboard
for call_site in [
    '0E3B:3B8D',    # Menu for "?" key
    '0E3B:3B9A',    # Menu for "ESC" key
    '0E3B:6921',    # "To go back to the room you just left, press Enter"
    '0E3B:B5A7',    # Computer disk terminal in world 4
]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(1)
    subroutine = b.jumpTarget(call_site)
    b.patchAndHook(call_site, 'ret',
        'g.hw->output.pushDelay(20);'
        'g.proc->continueFrom(r, &sub_%X);' % continue_at.linear)
    b.patch(continue_at, 'call 0x%04x' % subroutine.offset, length=2)
    b.markSubroutine(continue_at)

# Break control flow in the transporter animation loop,
# it's too long to store in the output queue.
video_blit_frame = sbt86.Addr16(str='0E3B:1700')
for call_site in [
    '0E3B:6E77',  # Sewer -> Subway
    '0E3B:898B',  # Subway -> Town
    '0E3B:958A',  # Town -> Comp
    '0E3B:B3E6',  # Comp -> Street
    '0E3B:C8D2',  # Street -> End
]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(1)
    subroutine = b.jumpTarget(call_site)
    assert subroutine.linear == video_blit_frame.linear
    b.patchAndHook(call_site, 'ret', 'g.proc->continueFrom(r, &sub_%X);' % continue_at.linear)
    b.patch(continue_at, 'call 0x%04x' % subroutine.offset, length=2)
    b.markSubroutine(continue_at)

# Skip the disk prompts after the transporter animation
b.patch('0E3B:691C', 'jmp 0x692D')   # Sewer -> Subway
b.patch('0E3B:8AA0', 'jmp 0x8AAF')   # Subway -> Town
b.patch('0E3B:97A4', 'jmp 0x97B3')   # Town -> Comp
b.patch('0E3B:A61A', 'jmp 0xA629')   # Comp -> Street
b.patch('0E3B:B808', 'jmp 0xB817')   # Street -> End

# The computer terminal has a nested event loop we need to break.
# Inline it so we still run the rest of the event loop upon return, in case that matters.
b.patch('0E3B:A416', 'jmp 0xB492')   # Entry
b.patch('0E3B:B4E1', 'jmp 0xA419')   # Exit early (not showing menu)
b.patch('0E3B:B5B4', 'jmp 0xA419')   # Exit after informational disk (red/blue)
b.patch('0E3B:B5E7', 'jmp 0xA419')   # Exit after camera disk (white)

# To show the end cutscene, we return code 0xD5 to PLAY.EXE, which
# will then exec MENU2.EXE using the obscure parameter byte 0x17.
# This was always an unfortunate design, since starting the game directly
# by running GAME.EXE meant that you'd just get a DOS prompt instead of
# the victory screen. Let's improve on that by exec'ing the end screen directly.
b.hook('0E3B:2C96', 'g.hw->exec("menu2.exe", "\\x17");')

b.writeCodeToFile(os.path.join(basedir, 'bt_game.cpp'), 'GameEXE')
