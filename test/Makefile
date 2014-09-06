VPATH= ../src

P=testset
OBJECTS= trie.o starcode.o
CFLAGS= -I../src `pkg-config --cflags glib-2.0` -g -Wall -std=gnu99 \
	-fprofile-arcs -ftest-coverage \
	-O0 -Wno-aggressive-loop-optimizations
	#-pg -fprofile-arcs -ftest-coverage
LDLIBS= -L`pwd` -Wl,-rpath=`pwd` `pkg-config --libs glib-2.0` \
	-lfaultymalloc -lpthread
CC= gcc
$(P): $(OBJECTS) libfaultymalloc.so

clean:
	rm -f testset *.o *.so *.gcda *.gcno *.gcov gmon.out analysis.txt

libfaultymalloc.so: faultymalloc.c
	$(CC) -fPIC -shared $(CFLAGS) -o libfaultymalloc.so faultymalloc.c

test: testset
	gtester --verbose --keep-going testset

testperf: testset
	#perf stat -B -e cache-references,cache-misses,cycles,instructions,branches,faults,migrations gtester --verbose --keep-going -m=perf testset
	perf stat -d gtester --verbose --keep-going -m=perf testset
	#gprof -F trie testset gmon.out > analysis.txt
	#gcov *.gcda

debug: testset
	gdb --command=debug.gdb --args testset -m=perf

valgrind:
	valgrind --vgdb=yes --vgdb-error=0 ./testset -m=perf
