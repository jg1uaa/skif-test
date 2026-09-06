TARGET = skif-test
OBJ = queue.o input-libinput.o input-skif.o table.o main.o
CFLAGS = -O2 -Wall -c -fdata-sections -ffunction-sections -pthread
LFLAGS = -Wl,--gc-sections -pthread
LDLIBS = 

CFLAGS += $(shell pkg-config --cflags libinput libudev)
LDLIBS += $(shell pkg-config --libs libinput libudev)

ifeq ($(DEBUG), true)
	CFLAGS += -DDEBUG
endif

all: $(TARGET)

queue.o: queue.c
	$(CC) $(CFLAGS) $< -o $@

input-libinput.o: input-libinput.c
	$(CC) $(CFLAGS) $< -o $@

input-skif.o: input-skif.c
	$(CC) $(CFLAGS) $< -o $@

table.o: table.c
	$(CC) $(CFLAGS) $< -o $@

main.o: main.c
	$(CC) $(CFLAGS) $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(LFLAGS) $(OBJ) $(LDLIBS) -o $@

clean:
	rm -f $(TARGET) $(OBJ)
