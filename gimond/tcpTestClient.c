#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include <sys/types.h>

#define INTERNAL_ADDR "127.0.0.1"

#define BUFFER_SIZE 42

/* This is just a test client. */

int main(int argc, char **argv){
	struct hostent *hent;
	struct sockaddr_in addr;
	int sock;
	char rbuffer[BUFFER_SIZE];

	if (argc < 3){
		printf("Use: %s [port] [message size < %d]\n", argv[0], BUFFER_SIZE);
		return EXIT_FAILURE;
	}
	
	hent = gethostbyname(INTERNAL_ADDR);
	sock = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[1]));
	memcpy(&(addr.sin_addr.s_addr), hent->h_addr_list[0], hent->h_length);

	if(connect(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0){
		perror("Can't connect");
		close(sock);
		return EXIT_FAILURE;
	}
	
	if(send(sock, argv[2], BUFFER_SIZE, 0) < 0){
		perror("Can't send data");
		close(sock);
		return EXIT_FAILURE;
	}
	
	if(recv(sock, &rbuffer, BUFFER_SIZE, 0) < 0){
		perror("Can't receive");
		close(sock);
		return EXIT_FAILURE;
	}
	
	printf("Received: %s\n",rbuffer);
	
	close(sock);
	return EXIT_SUCCESS;
}