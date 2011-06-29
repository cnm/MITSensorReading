CC=gcc
LIBS = -lpthread -lfred -lyajl -ldl
FLAGS = -Wall -g -I../../include
EXECS = manager

all: $(EXECS)

spotter: *.c ../../lib/*.c
		$(CC) $(FLAGS) -o $@ $? $(LIBS)

clean: 
		rm $(EXECS)
