Robot Odyssey Rewired
=====================

Modernized port of a classic electronics adventure game.

Status: Playable, but needs testing and polishing. See below.

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

- load/save
  - local browser storage
  - picker for chip load
  - package thumbnails plus game data into PNG/JPEG/GIF for mobile+social?

- output
  - nicer video scaling

- input
  - nicer touch controls
  - context sensitive keyboard controls with help
  - context sensitive soldering actions / zooming

- offline support
  - service workers?

- enhancements
  - built-in chip decompiler
  - level editor
  
