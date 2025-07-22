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
b = sbt86.DOSBinary(os.path.join(basedir, "tut.exe"))

bt_common.patch(b)
bt_common.patchJoystick(b)
bt_common.patchVideoHighLevel(b)

# Remove modal "Insert disk 1" message before exiting back to menu
b.patch("0A8F:3B0F", "jmp 0x3B28")

# Break control flow at remaining locations that wait for keyboard
for call_site in [
    "0A8F:405C",  # Menu for help "?" key
    "0A8F:4069",  # Menu for "ESC" key
]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(1)
    subroutine = b.jumpTarget(call_site)
    b.patchAndHook(
        call_site,
        "ret",
        "g.hw->output.pushDelay(g.clock, 20);"
        "g.proc->continueFrom(r, &sub_%X);" % continue_at.linear,
    )
    b.patch(continue_at, "call 0x%04x" % subroutine.offset, length=2)
    b.markSubroutine(continue_at)

# Break control flow in the transporter animation loop; after animating
# it will change rooms (to demonstrate the teleporter) then we can safely restart the main loop.
for call_site in ["0A8F:54CE"]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(1)
    subroutine = b.jumpTarget(call_site)
    b.patchAndHook(
        call_site,
        "ret",
        "g.hw->output.pushDelay(g.clock, 0);"
        "g.proc->continueFrom(r, &sub_%X);" % continue_at.linear,
    )
    b.patch(continue_at, "call 0x%04x" % subroutine.offset, length=2)
    b.markSubroutine(continue_at)

b.writeCodeToFile(os.path.join(basedir, "bt_tutorial.cpp"), "TutorialEXE")
