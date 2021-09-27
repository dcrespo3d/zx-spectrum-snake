# Makefile to cross-compile on Linux for ZXSpectrum/Z80 8bit executable
#
# Z80:ZCROSS Z88ROOT=/opt/z88dk Z88=$Z88ROOT/bin ZCFG=$Z88ROOT/lib/config
#     ^^^^^^ ^^^^^^^            ^^^              ^^^^
#     |||||| |||||||            |||              ++++-- config files
#     |||||| |||||||            +++-- crosscompiler tool path
#     |||||| +++++++----------------- crosscompiler root path
#     ++++++------------------------- toolchain prefix
#

ZCROSS=
Z88ROOT=/opt/z88dk
Z88=$(Z88ROOT)/bin
ZCFG=$(Z88ROOT)/lib/config

PATH:=$(Z88):$(PATH)
ZCCCFG=$(ZCFG)

CC=zcc
LD=zcc
CFLAGS=+zx -I./zxlib
LDFLAGS=-vn -lndos -lm -create-app
FILE = snake
SOURCE = $(FILE)
TARGET = $(FILE).ZX8

all: $(TARGET)

#$(TARGET): $(SOURCE).o screen.o params0.o keyb.o tile.o text.o menu.o frame.o
$(TARGET): $(SOURCE).o
	$(Z88)/$(LD) $(CFLAGS) $^ $(LDFLAGS) -o $@
	rm $(SOURCE)_BANK_7.bin

$(SOURCE).o: $(SOURCE).c
	$(Z88)/$(CC) $(CFLAGS) -c $< -o $@

#screen.o: zxlib/screen.c
#	$(Z88)/$(CC) $(CFLAGS) -c $< -o $@

#params0.o: zxlib/params0.c
#	$(Z88)/$(CC) $(CFLAGS) -c $< -o $@

#keyb.o: zxlib/keyb.c
#	$(Z88)/$(CC) $(CFLAGS) -c $< -o $@

#tile.o: tile.c
#	$(Z88)/$(CC) $(CFLAGS) -c $< -o $@

#text.o: text.c
#	$(Z88)/$(CC) $(CFLAGS) -c $< -o $@

#menu.o: menu.c
#	$(Z88)/$(CC) $(CFLAGS) -c $< -o $@

#frame.o: frame.c
#	$(Z88)/$(CC) $(CFLAGS) -c $< -o $@

cleanobj:
	rm *.o

clean:
	rm *.o $(TARGET)
