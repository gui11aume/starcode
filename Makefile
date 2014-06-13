OBJECTS= trie.o starcode.o
CFLAGS= -std=c99 -Wall -g -Wall -O3
LDLIBS= -lm -lpthread
CC= gcc

all: starcode

starcode: $(OBJECTS) main-starcode.c 

clean:
	rm -f $(OBJECTS) starcode
