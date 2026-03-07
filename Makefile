CC := gcc

# collect all the object files
LIB := $(shell find lib -type f -name *.o)
SRC := $(shell find src -not -path '*/\.*' -type f -name *.c)
INC := -I include

DFLAGS := -g -DDEBUG
CFLAGS := -DCOLOR $(INC) 


.PHONY: clean all setup

all: setup
	$(CC) $(CFLAGS) $(LIB) $(SRC) -o bin/minishell -lreadline

debug: setup
	$(CC) $(DFLAGS) $(CFLAGS) $(LIB) $(SRC) -o bin/minishell -lreadline

setup:
	mkdir -p bin

clean:
	$(RM) -r bin
