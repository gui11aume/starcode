SRC_DIR= src
INC_DIR= src
OBJECT_FILES= trie.o starcode.o view.o rbtree.o
SOURCE_FILES= main-starcode.c

OBJECTS= $(addprefix $(SRC_DIR)/,$(OBJECT_FILES))
SOURCES= $(addprefix $(SRC_DIR)/,$(SOURCE_FILES))
INCLUDES= $(addprefix -I, $(INC_DIR))

CFLAGS= `pkg-config --cflags cairo` -std=c99 -Wall -g -O0
LDLIBS= -lcairo -lpthread -lm
CC= gcc
all: starcode

starcode: $(OBJECTS) $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) $(OBJECTS) $(LDLIBS) -o $@

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
clean:
	rm -f $(OBJECTS) starcode
