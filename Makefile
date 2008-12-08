FILES = main_serv.o server.o channel.o player.o array.o

default: ${FILES}
	gcc -o main ${FILES}

.c.o:
	gcc -c -ggdb $<

clean:
	rm *.o
  
