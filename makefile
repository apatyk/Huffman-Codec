CC = gcc
CFLAGS = -Wall -g

BINS = huff

all: $(BINS)

$(BINS):  $(BINS).c list.c list.h
	$(CC) list.c $(BINS).c $(CFLAGS) -o $(BINS)

style:
	astyle --style=java --break-blocks --pad-oper --pad-header --align-pointer=name --delete-empty-lines *.c

clean:
	rm $(BINS)
	rm *.huf
	rm *-recovered*

cleano:
	rm *.orig
