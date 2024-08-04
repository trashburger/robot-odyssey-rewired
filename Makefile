PYTHON      := $(shell which python3)
CC          := $(shell which em++)
NODE        := $(shell which node)
NPX         := $(shell which npx)

CCFLAGS := -std=c++11 -Oz -flto

ZSTD_OPTS := ZSTD_LEGACY_SUPPORT=0 CFLAGS=-Oz

# Our emscripten configuration is pretty minimal. We need its library
# support for embind and memset and an event loop, but for the most part
# we want the emscripten runtime disabled to save space.

WASMFLAGS := \
	-s WASM=1 \
	-s MODULARIZE=1 \
	-s VERBOSE=1 \
	-s ASSERTIONS=0 \
	-s STRICT=1 \
	-s MALLOC=emmalloc \
	-s NO_FILESYSTEM=1 \
	-s ALLOW_MEMORY_GROWTH=0 \
	-s NO_DYNAMIC_EXECUTION=1 \
	--emit-symbol-map \
	--bind

INCLUDES := \
	-I src/engine \
	-I library/circular_buffer/include \
	-I library/zstd/lib

# C++ modules, including generated code
OBJS := \
	build/bt_game.bc \
	build/bt_lab.bc \
	build/bt_show.bc \
	build/bt_show2.bc \
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

WEBPACK_DEPS := \
	build/engine.js \
	build/font/rofont.woff \
	src/*.js \
	src/*/*.js \
	src/*.html \
	src/style/*.css

CPP_DEPS := \
	src/engine/*.h \
	src/engine/*.py

all: dist

dist: $(WEBPACK_DEPS)
	mkdir -p build/
	mkdir -p dist/
	rm -f dist/*
	$(NPX) webpack --config ./webpack.config.js
	@echo Done.

clean:
	rm -Rf build/ dist/ .cache/
	make -C library/zstd clean

# Hot-reload server
hotserve: $(WEBPACK_DEPS)
	mkdir -p build/
	$(NPX) webpack-serve --config ./webpack.config.js --host 0.0.0.0 --port 8000

# Static server
distserve: dist
	(cd dist; $(PYTHON) -m http.server)

.PHONY: all clean dist hotserve distserve

# WASM build from bitcode
build/engine.js: $(OBJS)
	$(CC) $(CCFLAGS) $(WASMFLAGS) -o build/engine.js $(OBJS)

# Build normal C++ code to LLVM bitcode
build/%.bc: src/engine/%.cpp $(CPP_DEPS)
	@mkdir -p build/
	$(CC) $(CCFLAGS) $(INCLUDES) -c -o $@ $<

# Build generated C++ code to LLVM bitcode
build/%.bc: build/%.cpp $(CPP_DEPS)
	$(CC) $(CCFLAGS) $(INCLUDES) -c -o $@ $<

# Generate C++ code from 8086 EXEs by running Python translation scripts
build/%.cpp: src/engine/%.py src/engine/sbt86.py build/original
	$(PYTHON) $< build

# Tell gmake not to delete the intermediate .cpp files we generate
.PRECIOUS: build/%.cpp

# Pack all files in the build/fs/* directory, including a subset of
# the original game files, and the repacked show files.
build/fspack.cpp: src/assets/fs-packer.py build/original build/fs/show.shw
	$(PYTHON) $< build/fs $@

# Pseudo-target to clean up and extract original Robot Odyssey data
build/original: src/assets/check-originals.py
	@mkdir -p build/
	$(PYTHON) $< original build
	@touch $@

# Re-pack the game's show files, and generate additional outputs based on them
build/fs/show.shw: src/assets/showfile-repacker.py build/original
	$(PYTHON) $< build

# Generate SVGs from the game font
build/font/glyph-00.svg: src/assets/font2svg.js build/original
	@mkdir -p build/font/
	$(NODE) $<

# Generate web font from SVGs
build/font/rofont.woff: build/font/glyph-00.svg src/assets/font.config.json
	$(NPX) webfont build/font/glyph-*.svg --config ./src/assets/font.config.json

# Compile libzstd from source to LLVM bitcode
library/zstd/lib/libzstd.a:
	emmake make -C library/zstd/lib $(ZSTD_OPTS) libzstd.a
