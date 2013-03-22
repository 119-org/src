
CC	= gcc
LD	= ld

CFLAGS	:= -g -Wall -fPIC -fno-builtin
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
#$(OBJS): $(SRC)
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
	$(CC) -shared -fPIC -o $@ $^

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST)
	@echo ""
	@echo "compiling $(TGT_UNIT_TEST) ..."
	$(CC) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(TGT)
