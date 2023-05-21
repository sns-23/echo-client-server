CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

SERV_OBJS = server.o list.o util.o 
CLNT_OBJS = client.o util.o

all: echo-server echo-client

echo-server: $(SERV_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

echo-client: $(CLNT_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf echo-server echo-client *.o

.PHONY:
	all echo-server echo-client clean
