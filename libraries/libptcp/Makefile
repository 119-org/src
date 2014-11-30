
CC	= gcc
LD	= ld

CFLAGS	:= -g -Wall -fPIC
CFLAGS	+= -Werror
CFLAGS	+= -D__STDC_FORMAT_MACROS
LDFLAGS	:= -lrt -lpthread

TGT_LIB_A	= libptcp.a
TGT_LIB_SO	= libptcp.so
TGT_UNIT_TEST	= test_libptcp

OBJS_LIB	= libptcp.o
OBJS_UNIT_TEST	= test_libptcp.o

.PHONY : all clean

all: $(TGT_LIB_A) $(TGT_LIB_SO) $(TGT_UNIT_TEST) # test_file

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(TGT_LIB_A): $(OBJS_LIB)
	ar rcs $@ $^

$(TGT_LIB_SO): $(OBJS_LIB)
	$(CC) -shared -fPIC -o $@ $^

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST)
	$(CC) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

test_file: test_file_transfer.o
	$(CC) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

clean:
	rm -f *.o
	rm -f $(TGT_LIB_A) $(TGT_LIB_SO) $(TGT_UNIT_TEST)
