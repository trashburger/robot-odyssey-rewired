PYTHON      := python3
CC          := emcc

CCFLAGS := \
	-std=c++11 -Oz --bind \
	-I src \
	-I library/circular_buffer/include \
	-I library/zstd/lib

CCSETTINGS := \
	-s WASM=1 \
	-s MODULARIZE=1 \
	-s NO_FILESYSTEM=1

OBJS := \
	build/engine.bc \
	build/sbt86.bc \
	build/hardware.bc \
	build/roData.bc \
	build/tinySave.bc \
	build/filesystem.bc \
	build/output.bc \
	build/fspack.bc \
	build/bt_game.bc \
	build/bt_lab.bc \
	build/bt_menu.bc \
	build/bt_menu2.bc \
	build/bt_play.bc \
	build/bt_tutorial.bc \
	library/zstd/lib/libzstd.a

all: dist

dist: dist/index.html dist/main.css dist/bundle.js dist/engine.wasm

clean:
	rm -Rf build/ dist/
	make -C library/zstd clean

serve: all
	(cd dist; $(PYTHON) -m http.server)

.PHONY: all clean dist serve

dist/index.html: src/index.html
	@mkdir -p dist/
	cp $< $@

dist/main.css: src/main.css
	@mkdir -p dist/
	cp $< $@

dist/bundle.js: build/engine.js src/*.js
	npx webpack --config ./webpack.config.js

build/engine.js: $(OBJS)
	$(CC) $(CCFLAGS) $(CCSETTINGS) -o $@ $(OBJS)
	cp build/engine.wasm dist/engine.wasm

build/%.bc: src/%.cpp
	@mkdir -p build/
	$(CC) $(CCFLAGS) -c -o $@ $<

build/%.bc: build/%.cpp
	$(CC) $(CCFLAGS) -c -o $@ $<

build/%.cpp: scripts/%.py scripts/sbt86.py build/original
	$(PYTHON) $< build

build/fspack.cpp: scripts/fs-packer.py build/original
	$(PYTHON) $< build/fs $@

# Pseudo-target to clean up and extract original Robot Odyssey data
build/original:
	@mkdir -p build/
	$(PYTHON) scripts/check-originals.py original build
	@touch $@

library/zstd/lib/libzstd.a:
	emmake make -C library/zstd/lib libzstd.a
