CC=gcc
LIBS = -lpthread -lfred -lyajl
FLAGS = -Wall -g -I/home/novais/Dropbox/Tese/Implementacao/project/src/h
EXECS = gsd

all: $(EXECS)

gsd: *.c
		$(CC) $(FLAGS) -o $@ $? $(LIBS)

clean: 
		rm *.o $(EXECS)
