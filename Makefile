P=starcode
OBJECTS= trie.o
CFLAGS= -std=c99 -g -Wall -O3
CC= gcc
$(P): $(OBJECTS)

clean:
	rm -f $(P) $(OBJECTS)
