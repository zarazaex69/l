CC       = tcc
CFLAGS   = -Wall -Ivendor/raygui/src -Isrc
LDFLAGS  = -lraylib -lm -lGL -lpthread -ldl -lrt

BUILDDIR = build
TARGET   = $(BUILDDIR)/l

SRCS     = main.c src/helloworld.c src/autorun.c src/wallcreate.c

all: $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(SRCS) src/core.h src/helloworld.h src/autorun.h src/wallcreate.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean
