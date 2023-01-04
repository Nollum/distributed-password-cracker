all: server request_client worker_client

server: server.c ./lib/misc.c ./lib/misc.h ./lib/commands.h
	gcc server.c ./lib/misc.c -o server 

request_client: ./clients/request_client.c ./lib/misc.c ./lib/misc.h ./lib/commands.h
	gcc ./clients/request_client.c ./lib/misc.c -o request_client 

worker_client: ./clients/worker_client.c ./lib/misc.c ./lib/misc.h ./lib/commands.h
	gcc ./clients/worker_client.c ./lib/misc.c -o worker_client