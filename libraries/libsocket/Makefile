
.PHONY : all clean
CFLAGS = -g -Wall 

all: test_libsocket

objs = libsocket.o test_libsocket.o

%.o:%.c
	gcc -c $(CFLAGS) $< -o $@

test_libsocket: $(objs)
	gcc -o $@ $^ 
clean:
	rm *.o
	rm test_libsocket
