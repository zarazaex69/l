CC       = tcc
CFLAGS   = -Wall -Ivendor/raygui/src -Isrc
LDFLAGS  = -lraylib -lm -lGL -lpthread -ldl -lrt

BUILDDIR = build
TARGET   = $(BUILDDIR)/l

SRCS     = main.c src/helloworld.c src/autorun.c src/wallcreate.c src/wallsee.c src/keybind.c src/calc.c src/clock.c src/menu.c

all: $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(SRCS) src/core.h src/helloworld.h src/autorun.h src/wallcreate.h src/wallsee.h src/keybind.h src/calc.h src/clock.h src/menu.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean
