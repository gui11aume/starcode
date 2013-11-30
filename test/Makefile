vpath %.c ..
vpath %.h ..

P=testset
OBJECTS= trie.o starcode.o
CFLAGS= -I.. `pkg-config --cflags glib-2.0` -g -pg -Wall -std=gnu99 \
	          -D_no_starcode_main -O0 -fprofile-arcs -ftest-coverage
LDLIBS= -L`pwd` -Wl,-rpath=`pwd` `pkg-config --libs glib-2.0` -lmalloc 
CC= gcc
$(P): $(OBJECTS) libmalloc.so

clean:
	rm -f testset *.o *.so *.gcda *.gcno *.gcov gmon.out analysis.txt

libmalloc.so: malloc.c
	$(CC) -fPIC -shared $(CFLAGS) -o libmalloc.so malloc.c

test: testset
	gtester --verbose --keep-going testset

testperf: testset
	perf stat -B -e cache-references,cache-misses,cycles,instructions,branches,faults,migrations gtester --verbose --keep-going -m=perf testset
	gprof -F trie testset gmon.out > analysis.txt
	gcov *.gcda

debug: testset
	gdb --command=debug.gdb --args testset -m=perf
