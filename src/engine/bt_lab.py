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
b = sbt86.DOSBinary(os.path.join(basedir, "lab.exe"))

bt_common.patch(b)
bt_common.patchJoystick(b)
bt_common.patchChips(b)
bt_common.patchLoadSave(b)
bt_common.patchVideoHighLevel(b)

# Remove modal "Insert disk 1" message on save-file load failure
b.patch("0D63:5CFB", "jmp 0x5D14")

# Remove model "A disk error has occurred"
b.patch("0D63:5E3E", "ret")

# Remove modal "Insert disk 1" message before exiting back to menu
b.patch("0D63:5FFD", "jmp 0x6016")

# Break control flow at remaining locations that wait for keyboard
for call_site in [
    "0D63:26D7",  # Menu for "?" key
    "0D63:26E4",  # Menu for "ESC" key
    "0D63:7F78",  # Chip data editor (press "?" while holding a chip)
    "0D63:821D",  # Help for chip data editor ("?" in editor)
]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(1)
    subroutine = b.jumpTarget(call_site)
    b.patchAndHook(
        call_site,
        "ret",
        "g.clock += OutputInterface::msecToClocks(20);"
        "g.proc->continueFrom(r, &sub_%X);" % continue_at.linear,
    )
    b.patch(continue_at, "call 0x%04x" % subroutine.offset, length=2)
    b.markSubroutine(continue_at)

# Inline the single-caller chip data editor so we can insert continuations
# within the editor's help screen without losing control flow in the outer editor.
b.patch("0D63:7F5F", "jmp 0x7FB1")
b.patch("0D63:8061", "jmp 0x7F62")
b.patch("0D63:8232", "jmp 0x7F62")

# Inline check_main_game_keys too, since the chip editor's outer event loop ends
# up inlined into that function. It's single-caller, and this keeps control flow
# from being lost when coming back from the help screen in the chip editor.
b.patch("0D63:233B", "jmp 0x7EC5")
b.patch("0D63:7EE6", "jmp 0x233E")
b.patch("0D63:7FB0", "jmp 0x233E")

b.writeCodeToFile(os.path.join(basedir, "bt_lab.cpp"), "LabEXE")
