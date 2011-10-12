CC=gcc
LIBS = -lpthread -lfred -lyajl -ldl -lm
FLAGS = -Wall -g -I../../include
EXECS = agg

all: $(EXECS)

agg: *.c ../../lib/*.c ../../lib/rbtree/*.c
		$(CC) $(FLAGS) -o $@ $? $(LIBS)
		cp $(EXECS) ../build/

clean: 
		rm -f $(EXECS)
