CC=gcc
LIBS = -lpthread -lfred -lyajl -ldl
FLAGS = -Wall -g -I../../include
EXECS = spotter

all: $(EXECS)

spotter: spotter.c spotter_services.c ../../lib/*.c
		$(CC) $(FLAGS) -o $@ $? $(LIBS)

clean: 
		rm $(EXECS)

plugins: $(CC) -fPIC -g -c -Wall ir_sensor.c 
	
plugin_so: $(CC) -shared -Wl,-soname,ir_sensor.so.1 \ -o ir_sensor.so.1.0.1 ir_sensor.o -lc
