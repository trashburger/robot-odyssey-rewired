PYTHON      := python3

GENERATED_BT := \
	build/bt_game.cpp \
	build/bt_lab.cpp \
	build/bt_menu.cpp \
	build/bt_play.cpp \
	build/bt_renderer.cpp \
	build/bt_tutorial.cpp

all: build/original $(GENERATED_BT)

build/%.cpp: scripts/%.py scripts/sbt86.py
	@echo "[SBT86  ]" $@
	@$(PYTHON) $< build

# Pseudo-target to clean up and extract original Robot Odyssey data
build/original:
	@echo "[UNPACK ] Unpacking original Robot Odyssey data..."
	@mkdir -p build
	@$(PYTHON) scripts/check-originals.py original build
	@touch $@

.PHONY: all
