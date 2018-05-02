CC ?= gcc
PREFIX ?= /usr/local

libdelta.so: *.c *.h Makefile
	$(CC) -C \
		-Wall -Wextra -Werror \
		-std=c11 \
		-shared \
		-fpic \
		$(shell pkg-config --cflags purple libsoup-2.4) \
		-o libdelta.so \
		*.c \
		-shared \
		$(shell pkg-config --libs purple libsoup-2.4) \
		-ldeltachat

clean:
	rm libdelta.so
