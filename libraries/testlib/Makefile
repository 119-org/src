
CC = g++
LD = ld

CFLAGS += -D_LINUX
LDFLAGS := 

LIB_SRCS = main.c
EXE_SRCS = 

LIB_OBJS = $(patsubst %.cpp, %.o, $(LIB_SRCS))
EXE_OBJS = $(patsubst %.cpp, %.o, $(EXE_SRCS))

OBJS = $(LIB_OBJS) $(EXE_OBJS)

TGT = test
TGT_LIB_A =
TGT_LIB_SO =

all: $(TGT) $(TGT_LIB_A) $(TGT_LIB_SO)

$(TGT): $(OBJS)
	$(CC) -static $(LDFLAGS) -o $@ $(OBJS)

$(TGT_LIB_A): $(LIB_OBJS) 
	$(AR) -rc $@ $(LIB_OBJS)

$(TGT_LIB_SO): $(LIB_OBJS) 
	$(LD) -shared $(LDFLAGS) -o $@ $(LIB_OBJS) 

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

#install:

.PHONY: clean
clean:
	rm -rf $(TGT) $(TGT_LIB_A) $(TGT_LIB_SO) $(OBJS)
