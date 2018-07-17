SRC_DIR= src
INC_DIR= src
OBJECT_FILES= trie.o starcode.o
SOURCE_FILES= main-starcode.c

OBJECTS= $(addprefix $(SRC_DIR)/,$(OBJECT_FILES))
SOURCES= $(addprefix $(SRC_DIR)/,$(SOURCE_FILES))
INCLUDES= $(addprefix -I, $(INC_DIR))

# Development and debug flags.
GPROF_CFLAGS= -std=c99 -pg -O0
DEV_CFLAGS= -std=c99 -g -O0 -Wunused-parameter -Wredundant-decls \
	-Wreturn-type -Wswitch-default -Wunused-value -Wimplicit \
	-Wimplicit-function-declaration -Wimplicit-int -Wimport \
	-Wunused  -Wunused-function -Wunused-label -Wbad-function-cast \
	-Wno-int-to-pointer-cast -Wmissing-declarations -Wpointer-sign \
	-Wmissing-prototypes -Wnested-externs -Wold-style-definition \
	-Wstrict-prototypes  -Wextra -Wredundant-decls -Wunused \
	-Wunused-function -Wunused-parameter -Wunused-value -Wformat \
	-Wunused-variable -Wformat-nonliteral -Wparentheses -Wundef \
	-Wsequence-point -Wuninitialized -Wbad-function-cast

# Release flags.
REL_CFLAGS= -std=c99 -O3 -Wall

# Defaluts.
CFLAGS= $(REL_CFLAGS)
LDLIBS= -lpthread -lm
CC= gcc

# General rules.
all: starcode-release
release: starcode-release
dev: starcode-dev
gprof: starcode-profiling

# Compilation environments.
starcode-release: CFLAGS= $(REL_CFLAGS)
starcode-release: starcode

starcode-dev: CFLAGS= $(DEV_CFLAGS)
starcode-dev: starcode

starcode-profiling: CFLAGS= $(GPROF_CFLAGS)
starcode-profiling: starcode

# Compilation targets.
starcode: $(OBJECTS) $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) $(OBJECTS) $(LDLIBS) -o $@

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECTS) starcode
