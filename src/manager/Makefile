CC=gcc
LIBS = -lpthread -lfred -lyajl -ldl
FLAGS = -Wall -g -I../../include
EXECS = manager

all: $(EXECS)

manager: *.c ../../lib/*.c ../../lib/rbtree/*.c
		$(CC) $(FLAGS) -o $@ $? $(LIBS)

clean: 
		rm $(EXECS)
