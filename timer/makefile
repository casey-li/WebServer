objs = $(wildcard *.cpp *.hpp)

server:$(objs)
	g++ $^ -o $@ -pthread

.PHONY:clean
clean:
	rm $(objs) -f
