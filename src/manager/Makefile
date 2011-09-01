CC=gcc
LIBS = -lpthread -lfred -lyajl -ldl -lm
FLAGS = -Wall -g -I../../include
EXECS = manager

all: $(EXECS)

manager: *.c ../../lib/*.c ../../lib/rbtree/*.c
		$(CC) $(FLAGS) -o $@ $? $(LIBS)
		mv $(EXECS) ../build/

clean: 
		rm -f $(EXECS)
