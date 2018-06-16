PYTHON      := python3
CC          := emcc

CCFLAGS := -std=c++11 -Oz --bind

ZSTD_OPTS := ZSTD_LEGACY_SUPPORT=0 CFLAGS=-Oz

WASMFLAGS := \
	-s WASM=1 \
	-s MODULARIZE=1 \
	-s NO_FILESYSTEM=1 \
	-s ALLOW_MEMORY_GROWTH=0 \
	-s NO_DYNAMIC_EXECUTION=1 \
	--emit-symbol-map

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
	build/font/glyph-00.svg \
	src/*.js \
	src/*.html \
	src/*.css

CPP_DEPS := \
	src/engine/*.h

all: dist

dist: $(WEBPACK_DEPS)
	mkdir -p build/
	mkdir -p dist/
	npx webpack --config ./webpack.config.js

clean:
	rm -Rf build/ dist/ .cache/
	make -C library/zstd clean

# Hot-reload server
hotserve: $(WEBPACK_DEPS)
	mkdir -p build/
	npx webpack-serve --config ./webpack.config.js --host 0.0.0.0 --port 8000

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
build/fspack.cpp: src/fs-packer.py build/original build/fs/show.shw
	$(PYTHON) $< build/fs $@

# Pseudo-target to clean up and extract original Robot Odyssey data
build/original:
	@mkdir -p build/
	$(PYTHON) src/check-originals.py original build
	@touch $@

# Re-pack the game's show files, and generate additional outputs based on them
build/fs/show.shw: build/original src/showfile-repacker.py
	$(PYTHON) src/showfile-repacker.py build

# Generate SVGs from the game font
build/font/glyph-00.svg: build/original
	@mkdir -p build/font/
	node src/font2svg.js

# Compile libzstd from source to LLVM bitcode
library/zstd/lib/libzstd.a:
	emmake make -C library/zstd/lib $(ZSTD_OPTS) libzstd.a
