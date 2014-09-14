target = mihttpsvr
srcs = main.cpp
libs = 
subdirs= net
INCLUDE= -I./ 
LIBRARY= -lpthread
STATIC_LIB= net/libdce.a
CPPFLAGS+=-g 
all : $(target)


objs=$(patsubst %.cpp, %.o, $(srcs))

all : $(target)

$(target) : $(objs) 
	for d in $(subdirs); do make -C $$d; done
	g++ $(CPPFLAGS) $(INCLUDE) $(LIBRARY) -o $@ $^ $(STATIC_LIB)

%.o : %.cpp
	g++ $(INCLUDE) $(CPPFLAGS) $< -c -o $@ 

.PHONY : clean 
clean:
	rm -f $(objs)
	rm -f $(target)
	for d in $(subdirs); do make -C $$d clean; done
