CC      = tcc
CFLAGS  = -Wall -Ivendor/raygui/src
LDFLAGS = -lraylib -lm -lGL -lpthread -ldl -lrt

BUILDDIR = build
TARGET   = $(BUILDDIR)/hello

all: $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): main.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean
