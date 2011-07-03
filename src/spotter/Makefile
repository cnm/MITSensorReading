CC=gcc
LIBS = -lpthread -lfred -lyajl -ldl
FLAGS = -Wall -g -I../../include
EXECS = spotter

all: $(EXECS)

spotter: spotter.c spotter_services.c ../../lib/*.c
		$(CC) $(FLAGS) -o $@ $? $(LIBS)

clean: 
		rm $(EXECS)

plugins: 
	gcc -I../../include -fPIC -g -c -Wall ir_sensor.c  
	gcc --export-dynamic -shared -Wl,-soname,libir_sensor.so.1 -o libir_sensor.so.1.0.1 ir_sensor.o -lc
	mv libir_sensor.so.1.0.1 ../../lib/spot_plugins/
