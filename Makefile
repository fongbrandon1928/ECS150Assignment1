
CC=gcc

CFLAGS=-Wall

TARGET=myshell

all: $(TARGET)

$(TARGET): sshell.o children.o
	$(CC) $(CFLAGS) -o $(TARGET) sshell.o children.o

sshell.o: sshell.c children.h
	$(CC) $(CFLAGS) -c sshell.c

children.o: children.c children.h
	$(CC) $(CFLAGS) -c children.c

clean:
	rm -f $(TARGET) sshell.o children.o
