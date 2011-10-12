CC=gcc
LIBS = -lpthread -lfred -lyajl
FLAGS = -Wall -g -I../../include
EXECS = gsd

all: $(EXECS)

gsd: *.c ../../lib/listType.c
		$(CC) $(FLAGS) -o $@ $? $(LIBS)
		cp $(EXECS) ../build/

clean: 
		rm -f $(EXECS)
