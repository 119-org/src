CC	= g++
AR	= ar

CFLAGS	:= -g -Wall

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

proto: librpc.proto
	protoc --cpp_out=. librpc.proto

$(LIBRPC_CLIENT_A): librpc.pb.o rpc_client.o
	$(AR) rcs $@ $^

$(LIBRPC_SERVER_A): librpc.pb.o rpc_server.o
	$(AR) rcs $@ $^

$(CLIENT): test_client.o librpc_caller.o $(LIBRPC_CLIENT_A)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SERVER): test_server.o librpc_callee.o $(LIBRPC_SERVER_A)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o *.a *pb*
	rm -fr $(TARGET)
