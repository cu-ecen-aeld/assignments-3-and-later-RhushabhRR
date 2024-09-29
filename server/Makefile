CC = gcc
CFLAGS = -Werror -Wall

all:
	$(CROSS_COMPILE)$(CC) $(CFLAGS) socket.cpp -o aesdsocket

ref:
	$(CROSS_COMPILE)$(CC) $(CFLAGS) ref.cpp -o aesdsocket

clean:
	rm -f aesdsocket *.o