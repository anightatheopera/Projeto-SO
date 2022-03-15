CFLAGS=-Wall -Wextra -Wdouble-promotion -Wconversion -Wsign-conversion -std=c11 -g

CFILES := $(wildcard src/util/*.c)
HFILES := $(wildcard src/util/*.h)
OFILES := $(patsubst src/%, obj/%, $(CFILES:.c=.o))

all: server client

server: bin/sdstored

client: bin/sdstore

bin/sdstored: obj/sdstored.o $(OFILES)
	gcc $(CFLAGS) $^ -o $@

obj/sdstored.o: src/sdstored.c
	gcc $(CFLAGS) -c -o $@ $^

bin/sdstore: obj/sdstore.o
	gcc $(CFLAGS) $^ -o $@

obj/sdstore.o: src/sdstore.c
	gcc $(CFLAGS) -c -o $@ $^

obj/util/proc.o: src/util/proc.c
	gcc $(CFLAGS) -c -o $@ $^

obj/util/sv.o: src/util/sv.c
	gcc $(CFLAGS) -c -o $@ $^


.PHONY: clean
clean:
	rm obj/* tmp/* bin/{sdstore,sdstored}
