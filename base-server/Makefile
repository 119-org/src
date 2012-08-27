
CC = gcc

GITHUB_ROOT = ../../
SRC_DIR = $(GITHUB_ROOT)/src/
LIB_DIR = $(SRC_DIR)/libraries/
LIB_TCPIP_DIR = $(LIB_DIR)/libtcpip/
LIB_TCPIP = $(LIB_TCPIP_DIR)/libtcpip.a -lstdc++

CFLAGS = -Wall -g
CFLAGS += -D_LINUX
CFLAGS += -I$(LIB_DIR)/ -I$(LIB_TCPIP_DIR)/

LIBS = -lpthread
TARGET = base-server
RM = rm -f 
OBJS = main.o
all:$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIB_TCPIP)
clean:
	$(RM) $(TARGET) $(OBJS)
