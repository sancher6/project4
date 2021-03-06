#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT_NUM 1004

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

typedef struct _ThreadArgs {
	int clisockfd;
	int *arrpt;
} ThreadArgs;

void* thread_main(void* args)
{
	// make sure thread resources are deallocated upon return
	pthread_detach(pthread_self());

	// get socket descriptor from argument
	int clisockfd = ((ThreadArgs*) args)->clisockfd;
	int *clients = ((ThreadArgs*) args)->arrpt; 
	free(args);

	//-------------------------------
	// Now, we receive/send messages
	char buffer[256];
	int nsen, nrcv;



	nrcv = recv(clisockfd, buffer, 256, 0);
	if (nrcv < 0) error("ERROR recv() failed");

	while (nrcv > 0) {
		while(clients != NULL){
			nsen = send(*clients, buffer, nrcv, 0);
			if (nsen != nrcv) error("ERROR send() failed");

			nrcv = recv(clisockfd, buffer, 256, 0);
			if (nrcv < 0) error("ERROR recv() failed");
			clients += sizeof(int); 
		}
	}

	close(clisockfd);
	//-------------------------------

	return NULL;
}

int main(int argc, char *argv[])
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) error("ERROR opening socket");

	struct sockaddr_in serv_addr;
	socklen_t slen = sizeof(serv_addr);
	memset((char*) &serv_addr, 0, sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT_NUM);

	printf("sockfd : %d\n",sockfd);		
	printf("slen : %d\n",slen);
	int status = bind(sockfd, (struct sockaddr *)&serv_addr, slen);
	if (status < 0) error("ERROR on binding");

	listen(sockfd, 5);
	int clients[5];
	int cli_count = 0; 
	int *p = clients; 
	while(1) {
		struct sockaddr_in cli_addr;
		socklen_t clen = sizeof(cli_addr);
		int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clen);
		if (newsockfd < 0) error("ERROR on accept");

		printf("Connected: %s\n", inet_ntoa(cli_addr.sin_addr));

		// prepare ThreadArgs structure to pass client socket
		ThreadArgs* args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
		if (args == NULL) error("ERROR creating thread argument");
		
		clients[cli_count] = newsockfd; 
		cli_count += 1; 

		args->clisockfd = newsockfd;
		args->arrpt = p; 

		pthread_t tid;
		if (pthread_create(&tid, NULL, thread_main, (void*) args) != 0) error("ERROR creating a new thread");
	}

	return 0; 
}

