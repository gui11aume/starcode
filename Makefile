P=starcode
OBJECTS= trie.o
CFLAGS=-g -Wall -O3 -std=c99 
LDLIBS=
CC=gcc
$(P): $(OBJECTS)
