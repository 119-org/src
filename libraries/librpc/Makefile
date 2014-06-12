CC	= g++
AR	= ar

CFLAGS	:= -g -Wall

LDFLAGS	:= -levent
LDFLAGS	+= -lprotobuf

LIBRPC_A	= librpc.a
LIBRPC_SO	= librpc.so

.PHONY: all clean

all: proto $(LIBRPC_A)

%.o:%.cc
	$(CC) -c $(CFLAGS) $< -o $@

librpc.pb.o: librpc.pb.cc
	$(CC) -c $(CFLAGS) $< -o $@

proto: librpc.proto
	protoc --cpp_out=. librpc.proto

$(LIBRPC_A): librpc.pb.o rpc_client.o
	$(AR) rcs $@ $^

clean:
	rm -rf *.o *.a *pb*
