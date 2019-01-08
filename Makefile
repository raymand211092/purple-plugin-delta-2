CC ?= gcc
PREFIX ?= /usr/local

PKG_CONFIG ?= pkg-config
LIB_TARGET = libdelta.so
LIB_DEST = $(DESTDIR)`$(PKG_CONFIG) --variable=plugindir purple`

$(LIB_TARGET): *.c *.h Makefile
	$(CC) -C \
		-Wall -Wextra -Werror \
		-std=c11 \
		-shared \
		-fpic \
		$(shell $(PKG_CONFIG) --cflags purple libcurl) \
		-o $(LIB_TARGET) \
		*.c \
		-shared \
		$(shell $(PKG_CONFIG) --libs purple libcurl) \
		-ldeltachat

install:
	install -D $(LIB_TARGET) $(LIB_DEST)

uninstall:
	rm -f $(LIB_DEST)/$(LIB_TARGET)

clean:
	rm $(LIB_TARGET)
