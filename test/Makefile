vpath %.c ..
vpath %.h ..

P=test
OBJECTS= trie.o starcode.o
CFLAGS= -I.. `pkg-config --cflags glib-2.0` -g -pg -Wall -std=gnu99 \
	          -D_no_starcode_main -O3 -fprofile-arcs -ftest-coverage
LDLIBS= `pkg-config --libs glib-2.0`
CC=gcc
$(P): $(OBJECTS)

clean:
	rm -f test *.o *.gcda *.gcno *.gcov gmon.out analysis.txt
