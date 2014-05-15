
CC	= gcc
LD	= ld

CFLAGS	:= -g -Wall -fPIC
LDFLAGS	:= -lpthread

.PHONY : all clean

TGT_LIB_A	= libstring.a
TGT_LIB_SO	= libstring.so
TGT_UNIT_TEST	= test_libstring

TGT	:= $(TGT_LIB_A)
TGT	+= $(TGT_LIB_SO)
TGT	+= $(TGT_UNIT_TEST)

SRC	= $(wildcard *.c)
OBJS	= $(patsubst %.c,%.o,$(SRC))

OBJS_LIB	= libstring.o
OBJS_UNIT_TEST	= test_libstring.o

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
	rm $(OBJS)
	rm $(TGT)
