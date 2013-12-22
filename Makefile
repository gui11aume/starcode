P=starcode
OBJECTS= trie.o
#CFLAGS= -std=c99 -Wall -g -pg -Wall -fprofile-arcs -O3
CFLAGS= -std=c99 -Wall -g -Wall -O3
LDLIBS= -lm
CC= gcc
$(P): $(OBJECTS)

all: $(P) tquery

tquery: tquery.c starcode.h trie.o
	$(CC) $(CFLAGS) $(LDLIBS) tquery.c trie.o -o tquery

clean:
	rm -f $(P) $(OBJECTS) tquery
