CC=gcc
LIBS = -lpthread -lfred -lyajl -ldl -lm
FLAGS = -Wall -g -I../../include
EXECS = spotter

all: $(EXECS)

spotter: spotter.c spotter_services.c ../../lib/*.c ../../lib/rbtree/*.c
		$(CC) $(FLAGS) -o $@ $? $(LIBS)

clean: 
		rm $(EXECS)

plugins: 
	gcc -I../../include -fPIC -g -c -Wall ir_sensor.c  
	gcc --export-dynamic -shared -Wl,-soname,libir_sensor.so.1 -o libir_sensor.so.1.0.1 ir_sensor.o -lc
	gcc -I../../include -fPIC -g -c -lbluetooth -lm -lpthread -lssl inq.c reporter.c
	gcc --export-dynamic -shared -Wl,-soname,libbt_sensor.so.1 -o libbt_sensor.so.1.0.1 inq.o reporter.o -lc -lbluetooth -lm -lpthread -lssl
	#gcc -I../../include -fPIC -g -c -Wall -lphidgets rfid_sensor.c
	#gcc --export-dynamic -shared -Wl,-soname,librfid_sensor.so.1 -o librfid_sensor.so.1.0.1 rfid_sensor.o -lc -lphidgets
	mv *.so* ../../lib/spot_plugins/
