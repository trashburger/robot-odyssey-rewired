Robot Odyssey Rewired
=====================

Modernized port of a classic electronics adventure game.

Kinda working, but still much to do. See below.

This repository contains open source modifications which can be applied to the original game. This project was not created by nor endorsed by the game's original authors. Robot Odyssey is Copyright 1984 The Learning Company. The game itself is NOT part of this repository.


Dependencies
------------

- make
- git
- nasm
- node
- python3
- emscripten


Build Instructions
------------------

- Place original game files in the "original" directory
- git submodule update --init --recursive
- npm install
- make

The built web site will be in `dist`, or start a development server with `make serve`

To do
-----

- Needs play testing

- cutscenes
  - intro cutscene timing and sound are somewhat off
  - end-game is broken
  - Final cutscene still needs patching and testing
  - State of "PLAY.EXE" won't always be correct when returning from a subprocess, consider rewriting play.exe entirely

- load/save
  - package thumbnails plus game data into PNG/JPEG/GIF for mobile+social?
  - local browser storage
  - picker for chip load

- output
  - nicer video scaling

- input
  - nicer touch controls
  - controls help

- offline support
  - service workers? how does this even work nowadays.

- enhancements
  - zoomed detail loupe for soldering
  - context sensitive soldering actions
  - context sensitive keyboard controls
  - built-in chip decompiler
  - enhanced graphics modes

