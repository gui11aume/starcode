vpath %.c ..
vpath %.h ..

P=test
OBJECTS= trie.o starcode.o
CFLAGS= -I.. `pkg-config --cflags glib-2.0` -g -pg -Wall -std=gnu99 \
	          -D_test_starcode -O0 -fprofile-arcs -ftest-coverage
LDLIBS= `pkg-config --libs glib-2.0`
CC=gcc
$(P): $(OBJECTS)

clean:
	rm test *.o *.gcda *.gcno *.gcov gmon.out
