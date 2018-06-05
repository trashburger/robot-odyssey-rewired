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
	-I src \
	-I library/circular_buffer/include \
	-I library/zstd/lib

# C++ modules, including generated code
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

WEBPACK_DEPS := \
	build/engine.js \
	src/*.js \
	src/*.html \
	src/*.css

CPP_DEPS := \
	src/*.h

all: dist

dist: $(WEBPACK_DEPS)
	mkdir -p build/
	mkdir -p dist/
	npx webpack --config ./webpack.config.js

clean:
	rm -Rf build/ dist/ .cache/
	make -C library/zstd clean

serve: $(WEBPACK_DEPS)
	mkdir -p build/
	npx webpack-serve --config ./webpack.config.js --host 0.0.0.0 --port 8000

.PHONY: all clean dist serve

# WASM build from bitcode
build/engine.js: $(OBJS)
	$(CC) $(CCFLAGS) $(WASMFLAGS) -o build/engine.js $(OBJS)

# Build normal C++ code to LLVM bitcode
build/%.bc: src/%.cpp $(CPP_DEPS)
	@mkdir -p build/
	$(CC) $(CCFLAGS) $(INCLUDES) -c -o $@ $<

# Build generated C++ code to LLVM bitcode
build/%.bc: build/%.cpp $(CPP_DEPS)
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
