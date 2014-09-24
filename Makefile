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
#remove an object that already has a main method
TESTDEPS := $(filter-out obj/main.o, $(OBJECTS))
TESTS    := $(TESTSRC:$(TESTDIR)/%.c=$(BINDIR)/%.test)
rm        = rm -f
create_dir=@mkdir -p $(@D)

$(BINDIR)/$(TARGET): $(OBJECTS)
	$(create_dir)
	@$(LINKER) $@ $(LFLAGS) $(OBJECTS)

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(create_dir)
	@$(CC) $(CFLAGS) -c $< -o $@

$(TESTS): $(TESTOBJ) $(OBJECTS)
	$(create_dir)
	@$(LINKER) $@ $(LFLAGS) $(TESTOBJ) $(TESTDEPS)

$(TESTOBJ): $(OBJDIR)/%.o : $(TESTDIR)/%.c
	$(create_dir)
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
