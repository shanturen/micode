target = mihttpsvr
srcs = main.cpp
libs = 
subdirs= 
INCLUDE= -I./ 
LIBRARY= -lpthread
STATIC_LIB= net/libdce.a
CPPFLAGS+=-g 
all : $(target)


objs=$(patsubst %.cpp, %.o, $(srcs))

all : $(target)

$(target) : $(objs) 
	g++ $(CPPFLAGS) $(INCLUDE) $(LIBRARY) -o $@ $^ $(STATIC_LIB)

%.o : %.cpp
	g++ $(INCLUDE) $(CPPFLAGS) $< -c -o $@ 

.PHONY : clean 
clean:
	rm -f $(objs)
	rm -f $(target)
