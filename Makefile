OBJECTS= trie.o starcode.o
CFLAGS= -std=c99 -Wall -g -Wall -O3
LDLIBS= -lm
CC= gcc

all: starcode tquery

tquery: $(OBJECTS) main-tquery.c
	$(CC) $(CFLAGS) $(OBJECTS) main-tquery.c $(LDLIBS) -o tquery

starcode: $(OBJECTS) main-starcode.c

clean:
	rm -f $(OBJECTS) starcode tquery
