CC	= g++
AR	= ar

CFLAGS	:= -g -Wall -Werror

LDFLAGS	:= -levent
LDFLAGS	+= -lprotobuf

LIBRPC_CLIENT_A		= librpc_client.a
LIBRPC_CLIENT_SO	= librpc_client.so
LIBRPC_SERVER_A		= librpc_server.a
LIBRPC_SERVER_SO	= librpc_server.so
CLIENT			= client
SERVER 			= server

TARGET = proto $(LIBRPC_A) $(LIBRPC_CLIENT_A) $(LIBRPC_SERVER_A) $(CLIENT) $(SERVER)

.PHONY: all clean

all: $(TARGET)

%.o:%.cc
	$(CC) -c $(CFLAGS) $< -o $@

librpc.pb.o: librpc.pb.cc
	$(CC) -c $(CFLAGS) $< -o $@

proto:
	protoc -I=. --cpp_out=. librpc.proto \
				calc.proto \
				hello.proto

$(LIBRPC_CLIENT_A): librpc.pb.o rpc_client.o \
		hello_caller.o hello.pb.o \
		calc_caller.o calc.pb.o
	$(AR) rcs $@ $^

$(LIBRPC_SERVER_A): librpc.pb.o rpc_server.o \
		hello_callee.o hello.pb.o \
		calc_callee.o calc.pb.o
	$(AR) rcs $@ $^

$(CLIENT): test_client.o $(LIBRPC_CLIENT_A)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SERVER): test_server.o $(LIBRPC_SERVER_A)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o *.a *pb*
	rm -fr $(TARGET)
