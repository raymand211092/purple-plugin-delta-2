CC ?= gcc
PREFIX ?= /usr/local

libdelta.so: *.c *.h
	$(CC) -C \
		-Wall -Wextra -Werror \
		-std=c11 \
		-shared \
		-fpic \
		$(shell pkg-config --cflags purple) \
		-o libdelta.so \
		*.c \
		-shared \
		$(shell pkg-config --libs purple)

clean:
	rm libdelta.so
