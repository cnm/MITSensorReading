CC=gcc
LIBS = -lpthread -lfred -lyajl -ldl
FLAGS = -Wall -g -I../../include
EXECS = spotter

all: $(EXECS)

gsd: spotter.c
		$(CC) $(FLAGS) -o $@ $? $(LIBS)

clean: 
		rm $(EXECS)
