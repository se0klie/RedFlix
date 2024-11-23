CC = gcc
CFLAGS = -g
SRCS_COMMON = common.c linkedlist.c sbuf.c
HEADER = common.h linkedlist.h sbuf.h
LOGIC_SERVER = logic_side
TARGETS = global_server client

all: $(TARGETS) $(LOGIC_SERVER)

# Compilar global_server
global_server: global_server.o $(SRCS_COMMON:.c=.o)
	$(CC) $(CFLAGS) -o global_server global_server.o $(SRCS_COMMON:.c=.o)

# Compilar client
client: client.o $(SRCS_COMMON:.c=.o)
	$(CC) $(CFLAGS) -o client client.o $(SRCS_COMMON:.c=.o)

# Compilar logic_side
$(LOGIC_SERVER): logic_side.o $(SRCS_COMMON:.c=.o)
	$(CC) $(CFLAGS) -o $(LOGIC_SERVER) logic_side.o $(SRCS_COMMON:.c=.o) -lm

# Regla general para compilar .o
%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGETS) 
