CC=gcc
LIBS = -lpthread -lfred -lyajl
FLAGS = -Wall -g -I../../include
EXECS = gsd

all: $(EXECS)

gsd: *.c
		$(CC) $(FLAGS) -o $@ $? $(LIBS)

clean: 
		rm $(EXECS)
