# hashcat-nes: an a-z brute-force cracker for the NES (NROM / mapper 0).
# Set LLVM_MOS to your llvm-mos toolchain (see README).

LLVM_MOS ?= ../keen-nes/third_party/llvm-mos
CC := $(LLVM_MOS)/bin/mos-nes-nrom-clang
PY := python3

ROM := build/hashcat.nes

SRCS := src/main.c src/scenes.c src/text.c src/sound.c src/brute.c \
        src/hash.c src/md.c src/sha1.c src/sha256.c src/crc32.c src/chr.c

CFLAGS := -Os -flto -Wall -Isrc

all: $(ROM)

# gen_chr.py emits both src/chr.c and src/gen_title.h (the logo tile map);
# src/gen_title.h is co-generated and follows from the same recipe.
src/chr.c src/gen_title.h: tools/gen_chr.py tools/font8x8_basic.h references/hashcatlogo.png
	$(PY) tools/gen_chr.py

$(ROM): $(SRCS)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $(SRCS) -lneslib -Wl,-Map=build/hashcat.map
	@ls -la $@

# Host-side correctness check for the hash cores (not part of the ROM build).
test:
	clang -I src -o build/hashtest tools/hashtest.c \
	  src/hash.c src/md.c src/sha1.c src/sha256.c src/crc32.c && ./build/hashtest

clean:
	rm -rf build

.PHONY: all clean test
