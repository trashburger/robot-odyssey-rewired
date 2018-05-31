PYTHON      := python3
CC          := emcc

CCFLAGS := -std=c++11 -Oz --bind

ZSTD_OPTS := ZSTD_LEGACY_SUPPORT=0 CFLAGS=-Oz

WASMFLAGS := \
	-s WASM=1 \
	-s MODULARIZE=1 \
	-s NO_FILESYSTEM=1 \
	-s ALLOW_MEMORY_GROWTH=0 \
	-s NO_DYNAMIC_EXECUTION=1

INCLUDES := \
	-I src \
	-I library/circular_buffer/include \
	-I library/zstd/lib

OBJS := \
	build/bt_game.bc \
	build/bt_lab.bc \
	build/bt_menu.bc \
	build/bt_menu2.bc \
	build/bt_play.bc \
	build/bt_tutorial.bc \
	build/engine.bc \
	build/sbt86.bc \
	build/input.bc \
	build/roData.bc \
	build/tinySave.bc \
	build/filesystem.bc \
	build/output.bc \
	build/draw.bc \
	build/fspack.bc \
	build/hardware.bc \
	library/zstd/lib/libzstd.a

all: dist

dist: dist/index.html dist/engine.wasm

clean:
	rm -Rf build/ dist/ .cache/
	make -C library/zstd clean

serve: all
	(cd dist; $(PYTHON) -m http.server)

.PHONY: all clean dist serve

# Static files
dist/%: src/%
	@mkdir -p dist/
	cp $< $@

# Javascript build with webpack
dist/index.html: build/engine.js src/*.js src/*.html src/*.css
	npx webpack --config ./webpack.config.js

# WASM build from bitcode
build/engine.js dist/engine.wasm: $(OBJS)
	@mkdir -p dist/
	$(CC) $(CCFLAGS) $(WASMFLAGS) -o build/engine.js $(OBJS)
	cp build/engine.wasm dist/

# Build normal C++ code to LLVM bitcode
build/%.bc: src/%.cpp
	@mkdir -p build/
	$(CC) $(CCFLAGS) $(INCLUDES) -c -o $@ $<

# Build generated C++ code to LLVM bitcode
build/%.bc: build/%.cpp
	$(CC) $(CCFLAGS) $(INCLUDES) -c -o $@ $<

# Generate C++ code from 8086 EXEs by running Python translation scripts
build/%.cpp: src/%.py src/sbt86.py build/original
	$(PYTHON) $< build

# Tell gmake not to delete the intermediate .cpp files we generate
.PRECIOUS: build/%.cpp

build/fspack.cpp: src/fs-packer.py build/original
	$(PYTHON) $< build/fs $@

# Pseudo-target to clean up and extract original Robot Odyssey data
build/original:
	@mkdir -p build/
	$(PYTHON) src/check-originals.py original build
	@touch $@

# Compile libzstd from source to LLVM bitcode
library/zstd/lib/libzstd.a:
	emmake make -C library/zstd/lib $(ZSTD_OPTS) libzstd.a
