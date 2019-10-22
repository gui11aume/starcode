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
WARNINGS= -Waddress -Warray-bounds=1 -Wbool-compare -Wbool-operation         \
	-Wbad-function-cast -Wchar-subscripts -Wclobbered -Wcomment          \
	-Wduplicate-decl-specifier -Wempty-body -Wenum-compare               \
	-Wformat -Wignored-qualifiers -Wimplicit -Wimplicit-fallthrough=3    \
	-Wint-in-bool-context -Wlogical-not-parentheses -Wmain               \
	-Wmaybe-uninitialized -Wmemset-elt-size -Wmemset-transposed-args     \
	-Wmisleading-indentation -Wmissing-braces                            \
	-Wmissing-field-initializers -Wmissing-parameter-type -Wnonnull      \
	-Wnonnull-compare -Wold-style-declaration -Wopenmp-simd              \
	-Woverride-init -Wparentheses -Wpointer-sign -Wrestrict              \
	-Wreturn-type -Wsequence-point -Wshift-negative-value -Wsign-compare \
	-Wsizeof-pointer-memaccess -Wstrict-aliasing -Wstrict-overflow=4     \
	-Wswitch -Wtautological-compare -Wtrigraphs -Wtype-limits            \
	-Wuninitialized -Wuninitialized -Wunused-function -Wunused-label     \
	-Wunused-value -Wunused-variable -Wvolatile-register-var -Werror

CFLAGS= -std=gnu99 -g -O0 $(WARNINGS) $(INCLUDES) $(COVERAGE)
LDLIBS= -L`pwd` -Wl,-rpath=`pwd` -lunittest -lpthread -lm

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
