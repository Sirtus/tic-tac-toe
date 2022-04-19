all:
	make server_
	make client_


server_:
	gcc -Wall -c -o server_funct.o server_funct.c
	ar rcs libserver_funct.a server_funct.o
	gcc -o server server.c -L. -lserver_funct -lpthread 

client_:
	gcc -Wall -c -o client_funct.o client_funct.c
	ar rcs libclient_funct.a client_funct.o
	gcc -o client client.c -L. -lclient_funct -lpthread

run_server:
	./server 30020 /tmp/9Lq7BNBnBycd6nxy.socket

run_unix_client1:
	./client unix1 unix /tmp/9Lq7BNBnBycd6nxy.socket

run_unix_client2:
	./client unix2 unix /tmp/9Lq7BNBnBycd6nxy.socket

run_unix_client3:
	./client unix3 unix /tmp/9Lq7BNBnBycd6nxy.socket

run_unix_client4:
	./client unix4 unix /tmp/9Lq7BNBnBycd6nxy.socket

run_inet_client1:
	./client inet1 inet 127.06.04.02 30020

run_inet_client2:
	./client inet2 inet 127.06.04.02 30020

run_inet_client3:
	./client inet3 inet 127.06.04.02 30020

run_inet_client4:
	./client inet4 inet 127.06.04.02 30020

run_inet_client5:
	./client inet5 inet 127.06.04.02 30020

clean:
	rm -f server client *.o *.a