CC=gcc
LIBS = -lpthread -lfred -lyajl
FLAGS = -Wall -g -I../../h
EXECS = gsd

all: $(EXECS)

gsd: *.c
		$(CC) $(FLAGS) -o $@ $? $(LIBS)

clean: 
		rm $(EXECS)
