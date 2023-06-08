objs = 	./*.cpp ./src/buffer/*.cpp ./src/epoller/*.cpp \
		./src/http/*.cpp ./src/log/*.cpp ./src/server/*.cpp \
		./src/thread_poll/*.cpp ./src/timer/*.cpp ./src/mysql_connection_pool/*.cpp
my_server : $(objs)
	g++ $(objs) -o my_server -pthread -l mysqlclient
