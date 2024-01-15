CC=gcc

CFLAGS=-Wall -Wextra -Werror

TARGET=myshell

all: $(TARGET)

$(TARGET): sshell.o
	$(CC) $(CFLAGS) -o $(TARGET) sshell.o

sshell.o: sshell.c children.h
	$(CC) $(CFLAGS) -c sshell.c

clean:
	rm -f $(TARGET) sshell.o
