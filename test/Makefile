vpath %.c ../src
vpath %.c lib
vpath %.h ../src
vpath %.h lib

P= runtests
OBJECTS= trie.o starcode.o tests.o libunittest.so

CC= gcc
INCLUDES= -I../src -Ilib
COVERAGE= -fprofile-arcs -ftest-coverage
CFLAGS= -std=gnu99 -g -Wall -O0 $(INCLUDES) $(COVERAGE)
LDLIBS= -L`pwd` -Wl,-rpath=`pwd` -lunittest -lpthread

$(P): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDLIBS) -o $@

clean:
	rm -f $(P) $(OBJECTS) *.gcda *.gcno *.gcov gmon.out .inspect.gdb

libunittest.so: unittest.c
	$(CC) -fPIC -shared $(CFLAGS) -o libunittest.so lib/unittest.c

test: $(P)
	./$(P)

inspect: $(P)
	gdb --command=.inspect.gdb --args $(P)

valgrind:
	valgrind --vgdb=yes --vgdb-error=0 ./$(P) --debug
	#valgrind ./$(P) --debug
