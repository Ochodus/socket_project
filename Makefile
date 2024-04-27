CC = gcc
TARGET = server
OBJECTS = csapp.o server.o
CFLAGS = -W -Wall

all : $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean :
	rm *.o server
