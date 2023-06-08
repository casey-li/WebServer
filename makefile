CC = g++

CFLAGS = -std=c++14 -O2 -Wall -l pthread -l mysqlclient 

TARGET = ./bin/server

SRCS = 	./src/buffer/*.cpp ./src/epoller/*.cpp \
		./src/http/*.cpp ./src/log/*.cpp ./src/server/*.cpp \
		./src/thread_pool/*.cpp ./src/timer/*.cpp ./src/mysql_connection_pool/*.cpp ./main.cpp 

# OBJS = $(patsubst %.cpp, %.o, $(SRCS))

# 通过使用@符号，可以在编译过程中减少冗长的输出信息，使输出更加简洁
# 创建目录，-p 表示目标文件夹不存在则创建，若存在则不进行任何操作
$(TARGET) : $(SRCS)
	@ mkdir -p bin
	$(CC) $^ -o $@ $(CFLAGS)



