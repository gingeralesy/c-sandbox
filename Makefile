## Sandbox makefile

TARGET = sandbox
LINDEPS = freetype2 ncurses
WINDEPS = ncurses
CC = gcc
CXX = g++

IDIR = include
BDIR = bin
SDIR = src
ODIR = obj
LDIR = lib

ifeq ($(OS),Windows_NT)
  DEPS = $(WINDEPS)
  LIBDIRS = -L$(LDIR) -LC:/msys64/mingw64/lib
  LIBS = -l:libpcre.a -lWs2_32 -lpthread -lm $(shell pkg-config --libs $(DEPS))
else
  DEPS = $(LINDEPS)
  LIBDIRS = -L$(LDIR) -L/usr/lib -L/usr/lib64
  LIBS = -l:libpcre.a -lpthread -lm $(shell pkg-config --libs $(DEPS))
endif

CFLAGS = -g -Wall -std=c11 $(shell pkg-config --cflags $(DEPS))

SRC = $(wildcard $(SDIR)/*.c)

OBJ := $(SRC:$(SDIR)/%.c=$(ODIR)/%.o)

BIN = $(BDIR)/$(TARGET)

RM = rm -f

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< -I$(IDIR) $(CFLAGS)

build: $(OBJ)
	$(CC) -o $(BIN) $^ -I$(IDIR) $(CFLAGS) $(LIBDIRS) $(LIBS)

.PHONY: clean

clean:
	$(RM) $(ODIR)/*.o *~ */*~ $(BIN)

run:
	./$(BIN)
