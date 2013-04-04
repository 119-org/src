
CC	= gcc
LD	= ld
GITHUB	= ../../..
LIB_DIR	= $(GITHUB)/src/libraries

CFLAGS	:= -g -Wall -fPIC -fno-builtin-log
CFLAGS	+= -I$(LIB_DIR)/libsock/ -I$(LIB_DIR)/libmempool/ -I$(LIB_DIR)/liblog/

LDFLAGS	:= -lpthread

.PHONY : all clean

TGT_LIB_A	= librpc.a
TGT_LIB_SO	= librpc.so
TGT_UNIT_TEST	= test_librpc
LIBS	:= $(TGT_LIB_A) $(LIB_DIR)/libsock/libsock.a $(LIB_DIR)/libmempool/libmempool.a $(LIB_DIR)/liblog/liblog.a

TGT	:= $(TGT_LIB_A)
TGT	+= $(TGT_LIB_SO)
TGT	+= $(TGT_UNIT_TEST)

SRC	= $(wildcard *.c)
OBJS	= $(patsubst %.c,%.o,$(SRC))

OBJS_LIB	= librpc.o
OBJS_UNIT_TEST	= test_librpc.o

all: $(TGT)


%.o:%.c
	@echo ""
	@echo "compiling $@ ..."
	$(CC) -c $(CFLAGS) $< -o $@

$(TGT_LIB_A): $(OBJS_LIB)
	@echo ""
	@echo "compiling $(TGT_LIB_A) ..."
	ar rcs $@ $^

$(TGT_LIB_SO): $(OBJS_LIB)
	@echo ""
	@echo "compiling $(TGT_LIB_SO) ..."
	$(CC) -shared -fPIC -o $@ $^ $(LIBS) $(LDFLAGS)

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST)
	@echo ""
	@echo "compiling $(TGT_UNIT_TEST) ..."
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(TGT)
