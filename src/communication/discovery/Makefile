CC=gcc
LIBS = -pthread
FLAGS = -Wall -I/home/novais/Dropbox/Tese/Implementacao/project/src/h
EXECS = GSD

all: $(EXECS)

proxy: *.c
		$(CC) $(FLAGS) -o $@ $? $(LIBS)

clean: 
		rm $(EXECS)
