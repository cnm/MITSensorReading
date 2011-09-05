inq: inq.o reporter.o
	gcc -Wall -o inq -lbluetooth inq.o reporter.o -I../../include/ -lssl -lpthread -lm

inq.o: inq.c
	gcc -c -Wall -o inq.o inq.c -I../../include/

reporter.o: reporter.c
	gcc -c -Wall -o reporter.o reporter.c -I../../include/ -lssl
