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

LIBDIRS = -L$(LDIR)
LIBS = -l:libpcre.a -lpthread -lm

ifeq ($(OS),Windows_NT)
  DEPS = $(WINDEPS)
  LIBDIRS := $(LIBDIRS) -LC:/msys64/mingw64/lib
  LIBS := $(LIBS) -lWs2_32
else
  DEPS = $(LINDEPS)
  LIBDIRS := $(LIBDIRS) -L/usr/lib -L/usr/lib64
endif

CFLAGS = -g -Wall -std=c11 $(shell pkg-config --cflags $(DEPS)) -iquote $(IDIR)
LDFLAGS := $(LIBDIRS) $(LIBS) $(shell pkg-config --libs $(DEPS))

SPATTERN = $(SDIR)/%.c
OPATTERN = $(ODIR)/%.o

INC = $(wildcard $(IDIR)/*.h)
SRC = $(wildcard $(SDIR)/*.c)
OBJ = $(patsubst $(SPATTERN),$(OPATTERN),$(SRC))
BIN = $(BDIR)/$(TARGET)

RM = rm -f

$(BIN): $(OBJ)
	@$(CC) -o $@ $^ $(LDFLAGS)
	@echo " *** Linked "$(BIN)

$(OBJ): $(OPATTERN) : $(SPATTERN)
	@$(CC) $(CFLAGS) -c $^ -o $@
	@echo " *** Compiled "$@

TAGS: $(SRC) $(INC)
	@ctags -e $^
	@echo " *** TAGS file created"

.PHONY: tags
tags: TAGS

.PHONY: clean
clean:
	@$(RM) $(OBJ) $(BIN) *~ */*~
	@echo " *** Cleanup complete"

.PHONY: run
run:
	./$(BIN)

.PHONY: all
all: clean $(BIN)
	@echo " *** All done"
