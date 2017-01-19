CC = gcc -g -Wall -std=gnu99
LIBS = $(shell pkg-config --libs glib-2.0)
INCLUDE = $(shell pkg-config --cflags glib-2.0)
PRG_NAME = main

all: clean compile

compile:
	$(CC) g_lock_manager.c main.c $(INCLUDE) $(LIBS) -o ${PRG_NAME}

test:
	./${PRG_NAME}

clean:
	(rm -f ${PRG_NAME})
