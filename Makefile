PYTHON      := python3

GENERATED_BT := \
	build/bt_game.cpp \
	build/bt_lab.cpp \
	build/bt_menu.cpp \
	build/bt_menu2.cpp \
	build/bt_play.cpp \
	build/bt_tutorial.cpp

all: build/original build/fspack.cpp $(GENERATED_BT)

clean:
	rm -Rf build/ dist/ .cache/

build/%.cpp: scripts/%.py scripts/sbt86.py
	$(PYTHON) $< build

build/fspack.cpp: scripts/fs-packer.py build/fs/tut7.wor
	$(PYTHON) $< build/fs $@

# Pseudo-target to clean up and extract original Robot Odyssey data
build/original:
	mkdir -p build
	$(PYTHON) scripts/check-originals.py original build
	touch $@

.PHONY: all clean
