#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
/* For portability */
#include <sys/types.h>

/* Temporary */
#define INTERNAL_ADDR "127.0.0.1"
#define BUFFER_SIZE 42
/* End Temporary */

/* This will be filled in later */
typedef struct{
	int sock;
}thread_data;

/* Temporary */
void *doStuff(void *arg){
	thread_data *td = (thread_data*)arg;
	char buffer[BUFFER_SIZE];
	printf("I got one!\n");
	
	if(recv(td->sock, &buffer, BUFFER_SIZE, 0) < 0){
		perror("Can't receive");
	}
	
	printf("He asked for: %s\n",buffer);
	
	if(send(td->sock, &buffer, strlen(buffer), 0) < 0){
		perror("Can't send data");
	}
	
	close(td->sock);
	return EXIT_SUCCESS;
}
/* End Temporary */

/* This will be turned into a thread later on. */
int main(int argc, char **argv){
	int sock, thr;
	struct sockaddr_in addr;
	socklen_t lg_addr;
	pthread_t pth;
	thread_data *td;
	/* Temporary */
	if (argc == 1){
		printf("Use: %s [port]\n", argv[0]);
		return EXIT_FAILURE;
	}
	/* End Temporary */
	
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Can't open socket");
		/* TODO Define what should be done */
	}
	
	addr.sin_family = AF_INET;
	/* Temporary */
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_addr.s_addr = inet_addr(INTERNAL_ADDR);
	/* End Temporary */

	if(bind(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0){
		perror("Can't bind");
		close(sock);
		/* TODO Define what should be done */
	}
	
	if(listen(sock, 0) < 0){
		perror("Can't listen");
		close(sock);
		/* TODO Define what should be done */
	}
	/* Connexion loop */
	for(;;){
		
		if((td = calloc(1, sizeof(thread_data))) == NULL){
			perror("Can't Calloc thread data");
			/* TODO Define what should be done */
		}

		if((td->sock = accept(sock, (struct sockaddr*) &addr, &lg_addr)) < 0){
			perror("Accept failed");
			/* TODO Define what should be done */
		}
		
		if((thr = pthread_create(&pth,NULL,doStuff,(void*)td)) == 0){
#ifdef VERBOSE
			printf("The stats have been sent.\n");
#endif
		}else{
			/* Should get the errno from thr */
			perror("A stat thread has crashed");
			/* TODO Define what should be done */
		}
	}
	free(td);
	close(sock);
	return EXIT_SUCCESS;
}