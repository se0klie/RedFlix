CC = gcc
CFLAGS = -g
SRCS = global_server.c  common.c linkedlist.c sbuf.c
HEADER = common.h linkedlist.h sbuf.h
OBJS = $(SRCS:.c=.o)
TARGET = global_server
LOGIC_SERVER = logic_side
CLIENT_SERVER = client

all: $(TARGET) 

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(LOGIC_SERVER): logic_side.o common.o linkedlist.o
	$(CC) $(CFLAGS) -o $(LOGIC_SERVER) logic_side.o common.o linkedlist.o -lm

$(CLIENT_SERVER): client.o common.o linkedlist.o
	$(CC) $(CFLAGS) -o $(CLIENT_SERVER) client.o common.o linkedlist.o -lm

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET) 
