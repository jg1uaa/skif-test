TARGET = skif-test
OBJ = serial.o main.o
CFLAGS = -O2 -Wall -c -fdata-sections -ffunction-sections
LFLAGS = -Wl,--gc-sections
LDLIBS = 

ifeq ($(DEBUG), true)
	CFLAGS += -DDEBUG
endif

all: $(TARGET)

serial.o: serial.c
	$(CC) $(CFLAGS) $< -o $@

main.o: main.c
	$(CC) $(CFLAGS) $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(LFLAGS) $(OBJ) $(LDLIBS) -o $@

clean:
	rm -f $(TARGET) $(OBJ)
