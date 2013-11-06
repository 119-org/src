
CC	= gcc
LD	= ld

CFLAGS	= -g -Wall -fPIC
# -std=c99
LDFLAGS	= -lpthread -lreadline


TGT_LIB_A	= libcmd.a
TGT_LIB_SO	= libcmd.so
TGT_UNIT_TEST	= test_libcmd

OBJS_LIB	= libcmd.o
OBJS_UNIT_TEST	= test_libcmd.o

.PHONY : all clean

all: $(TGT_LIB_A) $(TGT_LIB_SO) $(TGT_UNIT_TEST)

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
	$(CC) -shared -fPIC -o $@ $^

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST)
	@echo ""
	@echo "compiling $(TGT_UNIT_TEST) ..."
	$(CC) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

clean:
	rm -f *.o *.so *.a
	rm -f $(TGT_LIB_A) $(TGT_LIB_SO) $(TGT_UNIT_TEST)
