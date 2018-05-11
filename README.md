Robot Odyssey Rewired
=====================

Modernized port of a classic electronics adventure game.

Kinda working, but still much to do. See below.

This repository contains open source modifications which can be applied to the original game. This project was not created by nor endorsed by the game's original authors. Robot Odyssey is Copyright 1984 The Learning Company. The game itself is NOT part of this repository.


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

- cutscenes
  - intro cutscene timing and sound are somewhat off
  - end-game is broken
  - Final cutscene still needs patching and testing

- Needs play testing

- load/save
  - package thumbnails plus game data into PNG/JPEG/GIF for mobile+social
  - local browser storage
  - picker for chip load

- output
  - nicer video scaling

- input
  - keyboard event queueing
  - nicer touch controls
  - controls help

- offline support
  - service workers? how does this even work nowadays.
  - home screen icon, favicon

- enhancements
  - generalized touch/mouse controls?
    - servo moves player to cursor without violating game logic
    - zoomed detail loupe for soldering
    - context sensitive soldering actions
  - context sensitive keyboard controls
  - built-in chip decompiler
  - enhanced graphics modes

