Robot Odyssey Rewired
=====================

Modernized port of a classic electronics adventure game.

Work in progress, not ready yet.

This repository contains open source modifications which can be applied to the original game. This project was not created by nor endorsed by the game's original authors. Robot Odyssey is Copyright 1984 The Learning Company. The original game is not part of this repository.


Dependencies
------------

- python3
- emscripten
- nasm (with ndisasm)
- node (with npm)


Build Instructions
------------------

- Place original game files in the "original" directory
- npm install
- npm run build


To do
-----

- end-game is broken
  - Final cutscene still needs patching and testing
  - Needs play testing

- load/save
  - package thumbnails plus game data into PNG/JPEG/GIF for mobile+social
  - local browser storage
  - picker for chip load

- output
  - sound
  - nicer video scaling

- input
  - keyboard event queueing
  - nicer touch controls
  - controls help

- enhancements
  - touch/mouse controls for schematic editing
  - built-in chip decompiler
  - enhanced graphics modes
  - input UI / mobile experience
    - player-follows-finger?
    - zoomed detail loupe for soldering?
