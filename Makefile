CC=gcc
CFLAGS=-Wall -Wextra -Werror
TARGET=sshell

all: $(TARGET)

$(TARGET): sshell.c
	$(CC) $(CFLAGS) sshell.c -o $(TARGET)

clean:
	rm -f $(TARGET)
