LIBRARY := ../../library
ORIGINAL := ../../original

NPM := $(shell which npm)
PYTHON := $(shell which python3)

ifdef CLANG_VER
  CXX := $(shell which clang++-$(CLANG_VER))
  CC := $(shell which clang-$(CLANG_VER))
else
  CXX := $(shell which clang++)
  CC := $(shell which clang)
endif

ifdef DEBUG
  OPTFLAGS := -g
else
  OPTFLAGS := -Oz
endif

INCLUDES := \
	-I src/wasm \
	-I $(LIBRARY)/zstd/lib

TARGET := --target=wasm32 -mbulk-memory
CCFLAGS := $(INCLUDES) $(OPTFLAGS) $(TARGET) -flto -fstrict-aliasing -Wall -Wextra -Werror
CXXFLAGS := $(CCFLAGS) -std=c++11
LDFLAGS := $(OPTFLAGS) $(TARGET) -nostdlib -Wl,--no-entry

# C and C++ modules, including generated code
OBJS := \
	build/engine.bc \
	build/filesystem.bc \
	build/fs_pack.bc \
	build/joystick.bc \
	build/zcontexts.bc \
	build/zstd_nostdlib.bc \
	build/bt_game.bc \
	build/bt_lab.bc \
	build/bt_show.bc \
	build/bt_show2.bc \
	build/bt_tutorial.bc

CPP_DEPS := \
	src/wasm/*.h \
	src/wasm/*.py

TSC_DEPS := \
	src/*.ts \
	src/*/*.ts

ALL := \
	dist/index.js \
	dist/images/rofont.woff \
	dist/web/display.css \
	dist/wasm

all: $(ALL)

clean:
	rm -Rf build/ dist/
	make -C $(LIBRARY)/zstd clean

.PHONY: all clean

# WASM build from bitcode
dist/wasm: $(OBJS)
	@mkdir -p dist/
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

# Compile source C++ code to LLVM bitcode
build/%.bc: src/wasm/%.cpp $(CPP_DEPS)
	@mkdir -p build/
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compile source C code to LLVM bitcode
build/%.bc: src/wasm/%.c
	@mkdir -p build/
	$(CC) $(CCFLAGS) -c -o $@ $<

# Compile generated C++ code to LLVM bitcode
build/%.bc: build/%.cpp $(CPP_DEPS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Generate C++ code from 8086 EXEs by running Python translation scripts
build/%.cpp: src/wasm/%.py src/wasm/bt_common.py src/wasm/sbt86.py build/original.log
	$(PYTHON) $< > $(@:.cpp=.log)

# Tell gmake not to delete the intermediate .cpp files we generate
.PRECIOUS: build/%.cpp

# Pack all files in the build/fs/* directory, including a subset of
# the original game files, and the repacked show files.
build/fs_pack.cpp: src/wasm/fs_packer.py build/original.log build/fs/show.shw
	$(PYTHON) $< build/fs $@ > build/fs.log

# Clean up and extract original Robot Odyssey data
build/original.log: src/wasm/check_originals.py
	@mkdir -p build/
	$(PYTHON) $< $(ORIGINAL) build/ > build/original.log

# Re-pack the game's show files, and generate additional outputs based on them
build/fs/show.shw: src/images/showfile_repack.py build/original.log
	@mkdir -p dist/images/ build/fs/
	$(PYTHON) $< > build/show.log

# Generate SVGs from the game font
build/font/glyph_00.svg: dist/index.js build/original.log
	@mkdir -p build/font/
	$(NPM) exec -- node dist/images/font2svg.js build/font/font.fon build/font/glyph_

# Generate web font from SVGs
dist/images/rofont.woff: build/font/glyph_00.svg src/images/font.config.json
	@mkdir -p dist/images/
	$(NPM) exec -- webfont build/font/glyph_*.svg --config ./src/images/font.config.json

# Compile all Typescript
dist/index.js: $(TSC_DEPS)
	@mkdir -p build/ dist/
	$(NPM) exec -- tsc

# Compile Stylus to CSS
dist/web/%.css: src/web/%.styl
	@mkdir -p dist/web/
	$(NPM) exec -- stylus --compress < $< > $@

