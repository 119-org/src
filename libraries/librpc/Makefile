CC	= g++
AR	= ar
PWD	= $(shell pwd)

CFLAGS	:= -g -Wall -Werror
CFLAGS	+= -I$(PWD) -I$(PWD)/proto -I$(PWD)/client -I$(PWD)/server

LDFLAGS	:= -levent
LDFLAGS	+= -lprotobuf

LIBRPC_CLIENT_A		= librpc_client.a
LIBRPC_CLIENT_SO	= librpc_client.so
LIBRPC_SERVER_A		= librpc_server.a
LIBRPC_SERVER_SO	= librpc_server.so
CLIENT			= test_client
SERVER 			= test_server

TARGET = autogen $(LIBRPC_A) $(LIBRPC_CLIENT_A) $(LIBRPC_SERVER_A) $(CLIENT) $(SERVER)

.PHONY: all clean

all: $(TARGET)

autogen:
ifneq (proto/librpc.pb.h, $(wildcard proto/librpc.pb.h))
	protoc -I=./proto/ --cpp_out=./proto/ \
		./proto/librpc.proto \
		./proto/calc.proto \
		./proto/hello.proto
endif

%.o:%.cc
	$(CC) -c $(CFLAGS) $< -o $@

proto/librpc.pb.o: proto/librpc.pb.cc
	$(CC) -c $(CFLAGS) $< -o $@

$(LIBRPC_CLIENT_A): proto/librpc.pb.o rpc_client.o \
		client/hello_caller.o proto/hello.pb.o \
		client/calc_caller.o proto/calc.pb.o
	$(AR) rcs $@ $^

$(LIBRPC_SERVER_A): proto/librpc.pb.o rpc_server.o \
		server/hello_callee.o proto/hello.pb.o \
		server/calc_callee.o proto/calc.pb.o
	$(AR) rcs $@ $^

$(CLIENT): test_client.o $(LIBRPC_CLIENT_A)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SERVER): test_server.o $(LIBRPC_SERVER_A)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o *.a proto/*pb* client/*.o server/*.o
	rm -fr $(TARGET)
