CC	= g++
AR	= ar

CFLAGS	:= -g -Wall

LDFLAGS	:= -levent
LDFLAGS	+= -lprotobuf

LIBRPC_CALLER_A		= librpc_caller.a
LIBRPC_CALLER_SO	= librpc_caller.so
LIBRPC_CALLEE_A		= librpc_callee.a
LIBRPC_CALLEE_SO	= librpc_callee.so
TEST_CLIENT		= test_client
TEST_SERVER 		= test_server

.PHONY: all clean

all: proto $(LIBRPC_A) $(LIBRPC_CALLER_A) $(LIBRPC_CALLEE_A) $(TEST_CLIENT) $(TEST_SERVER)

%.o:%.cc
	$(CC) -c $(CFLAGS) $< -o $@

librpc.pb.o: librpc.pb.cc
	$(CC) -c $(CFLAGS) $< -o $@

proto: librpc.proto
	protoc --cpp_out=. librpc.proto

$(LIBRPC_CALLER_A): librpc.pb.o rpc_client.o librpc_caller.o
	$(AR) rcs $@ $^

$(LIBRPC_CALLEE_A): librpc.pb.o rpc_server.o librpc_callee.o
	$(AR) rcs $@ $^

$(TEST_CLIENT): test_client.o $(LIBRPC_CALLER_A)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_SERVER): test_server.o $(LIBRPC_CALLEE_A)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o *.a *pb*
