
CC	= gcc
LD	= ld

CFLAGS	:= -g -Wall -Werror -fPIC -fno-builtin
LDFLAGS	:= -lpthread

.PHONY : all clean

TGT_LIB_A	= liblog.a
TGT_LIB_SO	= liblog.so
TGT_UNIT_TEST	= test_liblog

TGT	:= $(TGT_LIB_A)
TGT	+= $(TGT_LIB_SO)
TGT	+= $(TGT_UNIT_TEST)

SRC	= $(wildcard *.c)
OBJS	= $(patsubst %.c,%.o,$(SRC))

OBJS_LIB	= liblog.o
OBJS_UNIT_TEST	= test_liblog.o

all: $(TGT)


%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(TGT_LIB_A): $(OBJS_LIB)
	@echo ""
	@echo "compiling $(TGT_LIB_A) ..."
	ar rcs $@ $^

$(TGT_LIB_SO): $(OBJS_LIB)
	@echo ""
	@echo "compiling $(TGT_LIB_SO) ..."
	$(CC) -shared -fPIC -o $@ $^

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST)
	@echo ""
	@echo "compiling $(TGT_UNIT_TEST) ..."
	$(CC) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

install:
	sudo cp -f liblog.h /usr/local/include/
	sudo cp -f $(TGT_LIB_A) /usr/local/lib/
	sudo cp -f $(TGT_LIB_SO) /usr/local/lib/
clean:
	rm -f $(OBJS)
	rm -f $(TGT)
