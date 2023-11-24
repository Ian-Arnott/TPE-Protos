STANDARD= -std=c11
DISABLE_STYLE = -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -Wno-implicit-fallthrough -Wno-newline-eof  # disable style warnings like unused functions
DEBUG = -g # enable debugging symbols
STRICT = -Werror # treat warnings as errors
SANITIZERS = -fsanitize=address -fsanitize=undefined -fsanitize=leak
CFLAGS = $(STANDARD) $(DISABLE_STYLE) $(DEBUG) $(STRICT) $(SANITIZERS) -pedantic -pedantic-errors -Wall -Wextra -D_POSIX_C_SOURCE=200112L -Wuninitialized -Wunused-variable 
SRCDIR = src
OBJDIR = obj

SOURCES = $(wildcard $(SRCDIR)/*.c) $(wildcard $(SRCDIR)/*/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

TARGET = pop3d

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJECTS): | $(OBJDIR)
$(TARGET): | $(BINDIR)

$(OBJDIR) $(BINDIR):
	@mkdir -p $@


.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(BINDIR) 
	$(MAKE) -C client_src all

all: $(TARGET) 
	$(MAKE) -C client_src all


.DEFAULT_GOAL := all