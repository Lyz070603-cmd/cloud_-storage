# 编译器
CC = gcc

# 编译选项
CFLAGS = -Wall -g

# 最终生成的可执行文件名字
TARGET = main

# 所有源文件
SRC = $(wildcard *.c)

# 把.c替换成.o
OBJ = $(SRC:.c=.o)


# 默认目标
all: $(TARGET)


# 链接生成可执行文件
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) -lcrypt


# 单个.c生成.o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


# 清理生成文件
clean:
	rm -f $(OBJ) $(TARGET)
