vpath %.c ../src
vpath %.c lib
vpath %.h ../src
vpath %.h lib

P= runtests

OBJECTS= tests_trie.o tests_starcode.o libunittest.so
SOURCES= starcode.c trie.c
HEADERS= starcode.h trie.h

CC= gcc
INCLUDES= -I../src -Ilib
COVERAGE= -fprofile-arcs -ftest-coverage
CFLAGS= -std=gnu99 -g -Wall -O0 $(INCLUDES) $(COVERAGE)
LDLIBS= -L`pwd` -Wl,-rpath=`pwd` -lunittest -lpthread

$(P): $(OBJECTS) $(SOURCES) $(HEADERS) runtests.c
	$(CC) $(CFLAGS) runtests.c $(OBJECTS) $(LDLIBS) -o $@

libunittest.so: unittest.c
	$(CC) -fPIC -shared $(CFLAGS) -o libunittest.so lib/unittest.c

test: $(P)
	./$(P)
	sh extratests.sh

inspect: $(P)
	gdb --command=.inspect.gdb --args $(P)

debug: $(P)
	gdb --command=.debug.gdb --args $(P)

valgrind: $(P)
	#valgrind --vgdb=yes --vgdb-error=0 ./$(P)
	valgrind --leak-check=full ./$(P)

clean:
	rm -f $(P) $(OBJECTS) *.gcda *.gcno *.gcov gmon.out .inspect.gdb
