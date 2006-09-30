
CC=gcc
CFLAGS=-Wall -g -DREAL_IS_FLOAT

all: mpglib


*.o: mpg123.h mpglib.h

mpglib: common.o dct64_i386.o decode_i386.o layer3.o tabinit.o interface.o main.o
	$(CC) -o mpglib common.o dct64_i386.o decode_i386.o layer3.o \
		tabinit.o interface.o main.o -lm

clean:
	rm *.o mpglib


