# 指定编译器
CC=gcc

# 指定编译器标志
CFLAGS=-Wall

# 指定最终可执行文件的名称
TARGET=myshell

all: $(TARGET)

# 链接目标文件生成最终的可执行文件
$(TARGET): sshell.o children.o
	$(CC) $(CFLAGS) -o $(TARGET) sshell.o children.o

# 从 sshell.c 生成 sshell.o
sshell.o: sshell.c children.h
	$(CC) $(CFLAGS) -c sshell.c

# 从 children.c 生成 children.o
children.o: children.c children.h
	$(CC) $(CFLAGS) -c children.c

# 清理编译生成的文件
clean:
	rm -f $(TARGET) sshell.o children.o
