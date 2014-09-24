#---
# Makefile for http_server project
#---
TARGET = http_server

CC = gcc
CFLAGS  = -Wall -I. -ggdb
LINKER  = gcc -o
LFLAGS  = -Wall -I. -lm
SRCDIR  = src
OBJDIR  = obj
BINDIR  = bin
TESTDIR = tests

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

TESTSRC  := $(wildcard $(TESTDIR)/*.c)
TESTOBJ  := $(TESTSRC:$(TESTDIR)/%.c=$(OBJDIR)/%.o)
TESTS    := $(TESTSRC:$(TESTDIR)/%.c=$(BINDIR)/%.test)
rm  = rm -f

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(LINKER) $@ $(LFLAGS) $(OBJECTS)

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

$(TESTS): $(TESTOBJ)
	@$(LINKER) $@ $(LFLAGS) $(TESTOBJ)

$(TESTOBJ): $(OBJDIR)/%.o : $(TESTDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

tests: $(TESTS) $(TESTOBJ)
	./$(TESTS)

.PHONEY: clean
clean:
	@$(rm) $(OBJECTS)
	@$(rm) $(TESTS)
	@$(rm) $(TESTOBJ)

.PHONEY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET)
