# Makefile
CC       ?= gcc
CFLAGS   ?= -std=c11 -Wall -Wextra
OPTFLAGS ?= -O2 -s
CPPFLAGS += -DVERSION=\"0.1.0\"

PREFIX   ?= /usr/local
BINDIR   ?= $(PREFIX)/bin

TARGET   := peek
SRC      := src/main.c src/utils/time.c src/utils/sys.c
OBJ      := $(SRC:.c=.o)

# Default build
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OPTFLAGS) $(LDFLAGS) -o $@ $^

# Generic compile rule
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OPTFLAGS) -c -o $@ $<

# Convenience targets
debug: CFLAGS += -O0 -g
debug: OPTFLAGS :=
debug: clean all

install: $(TARGET)
	mkdir -p "$(BINDIR)"
	install -m 0755 "$(TARGET)" "$(BINDIR)/$(TARGET)"

uninstall:
	rm -f "$(BINDIR)/$(TARGET)"

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all debug install uninstall clean
