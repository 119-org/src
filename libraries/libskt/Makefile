
CC	= gcc
LD	= ld

INCLUDE = -I../libutils
CFLAGS	= -g -Wall -fPIC
CFLAGS	+= $(INCLUDE)
LDFLAGS	= -lpthread


TGT_LIB_A	= libskt.a
TGT_LIB_SO	= libskt.so
TGT_UNIT_TEST	= test_libskt

OBJS_LIB	= libskt.o epoll.o select.o poll.o event.o
OBJS_UNIT_TEST	= test_libskt.o

.PHONY : all clean

all: $(TGT_LIB_A) $(TGT_LIB_SO) $(TGT_UNIT_TEST)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(TGT_LIB_A): $(OBJS_LIB)
	ar rcs $@ $^

$(TGT_LIB_SO): $(OBJS_LIB)
	$(CC) -shared -fPIC -o $@ $^

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST)
	$(CC) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

clean:
	rm -f *.o *.so *.a
	rm -f $(TGT_LIB_A) $(TGT_LIB_SO) $(TGT_UNIT_TEST)
