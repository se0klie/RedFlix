CC = gcc
CFLAGS = -g
SRCS = server.c common.c linkedlist.c
HEADER = common.h linkedlist.h
OBJS = $(SRCS:.c=.o)
TARGET = server

all: $(TARGET) 

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
	
%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET) 
