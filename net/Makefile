target = libdce.a
srcs = buffer.cpp event_manager.cpp event_manager_impl_epoll.cpp thread.cpp server.cpp socket.cpp socket_event.cpp message.cpp safe_time.cpp http_server.cpp
libs = 
subdirs= 
INCLUDE= -I./ -I../log/include/
LIBRARY= -lpthread 
CPPFLAGS+=-g 
all : $(target)


objs=$(patsubst %.cpp, %.o, $(srcs))

all : $(target)

$(target) : $(objs) 
	for dir in $(subdirs); do make -C $(dir) ; done
	ar cr $(target) $^ $(libs)

%.o : %.cpp
	g++ $(INCLUDE) $(CPPFLAGS) $< -c -o $@

.PHONY : clean
clean:
	rm -f $(objs)
	rm -f $(target)
