TARGET = crashing-http-server
CFLAGS = -Wall -pedantic -static

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	rm -f $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) -o $@ $(CFLAGS) $^
