CC = gcc
CFLAGS = -Werror -Wall

# cc = aarch64-none-linux-gnu-gcc
# cxx = aarch64-none-linux-gnu-g++

all:
	$(CROSS_COMPILE)$(CC) $(CFLAGS) socket.c -o aesdsocket

clean:
	rm -f aesdsocket *.o
