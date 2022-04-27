CFLAGS=-Wall -Wextra -Wdouble-promotion -Wconversion -Wsign-conversion -std=c11 -g -O0

CFILES := $(wildcard src/util/*.c)
HFILES := $(wildcard src/util/*.h)
OFILES := $(patsubst src/%, obj/%, $(CFILES:.c=.o))

all: server client

server: bin/sdstored

client: bin/sdstore

bin/sdstored: obj/sdstored.o $(OFILES)
	@ mkdir -p bin obj obj/util 
	gcc $(CFLAGS) $^ -o $@

obj/sdstored.o: src/sdstored.c
	@ mkdir -p bin obj obj/util 
	gcc $(CFLAGS) -c -o $@ $^

bin/sdstore: obj/sdstore.o
	@ mkdir -p bin obj obj/util 
	gcc $(CFLAGS) $^ -o $@

obj/sdstore.o: src/sdstore.c
	@ mkdir -p bin obj obj/util 
	gcc $(CFLAGS) -c -o $@ $^

obj/util/proc.o: src/util/proc.c
	@ mkdir -p bin obj obj/util 
	gcc $(CFLAGS) -c -o $@ $^

obj/util/sv.o: src/util/sv.c
	@ mkdir -p bin obj obj/util 
	gcc $(CFLAGS) -c -o $@ $^

obj/util/operations.o: src/util/operations.c
	@ mkdir -p bin obj obj/util 
	gcc $(CFLAGS) -c -o $@ $^


.PHONY: clean
clean:
	rm -r obj  bin
