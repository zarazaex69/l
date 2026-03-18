CC      = tcc
CFLAGS  = -Wall -Ivendor/raygui/src
LDFLAGS = -lraylib -lm -lGL -lpthread -ldl -lrt

TARGET  = hello

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
