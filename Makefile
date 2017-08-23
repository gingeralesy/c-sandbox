## Sandbox makefile

TARGET = sandbox
DEPS = freetype2 ncurses
CC = gcc
CXX = g++
CFLAGS = -g -Wall $(shell pkg-config --cflags $(DEPS))

IDIR = include
BDIR = bin
SDIR = src
ODIR = obj
LDIR = lib

LIBS = -l:libpcre.a -lpthread -lm $(shell pkg-config --libs $(DEPS))

SRC = $(wildcard $(SDIR)/*.c)

OBJ := $(SRC:$(SDIR)/%.c=$(ODIR)/%.o)

BIN = $(BDIR)/$(TARGET)

RM = rm -f

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< -I$(IDIR) $(CFLAGS)

build: $(OBJ)
	$(CC) -o $(BIN) $^ -I$(IDIR) $(CFLAGS) -L$(LDIR) -L/usr/lib -L/usr/lib64 $(LIBS)

.PHONY: clean

clean:
	$(RM) $(ODIR)/*.o *~ */*~ $(BIN)

run:
	./$(BIN)
