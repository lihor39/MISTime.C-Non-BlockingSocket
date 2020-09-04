// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch and the PBBS team
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include "gettime.h"
#include "utils.h"
#include "graph.h"
#include "parallel.h"
#include <typeinfo>
#include "IO.h"
#include "graphIO.h"
#include "parseCommandLine.h"
#include "MIS.h"
#include <time.h>
#include <pthread.h>
using namespace std;
using namespace benchIO;
#include <stdbool.h>
#include <unistd.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <stdlib.h> 
#include <netinet/in.h> 
#include <time.h> 
#include <fcntl.h>

// --> random max set as: 2147483647


#define PORT 8080 
#include <errno.h>
//#include <stdio.h> 
//#include <sys/socket.h> 
#include <arpa/inet.h> 
//#include <unistd.h> 
//#include <string.h> 
//#define PORT 8080 
#define NUM_THREADS     5

int primary = 0;
int reboot = 1;
int batchSize;
int checker = 0;
int CRC_val = 0;
char str[200];
int wait = 0;
const char* iFileSEND;
char* iFile;
bool runCheck = true;
int waitToCheckCRC = 0;
int readyToChange = 0;

pthread_t threads[NUM_THREADS];


int guard(int n, const char * err) { 
    if (n == -1) { 
        perror(err); exit(1); 
    } 
    return n; 
}


int test_node() {
	printf("\n test node started \n");
	int listen_socket_fd = guard(socket(AF_INET, SOCK_STREAM, 0), "could not create TCP listening socket");
	int flags = guard(fcntl(listen_socket_fd, F_GETFL), "could not get flags on TCP listening socket");
	guard(fcntl(listen_socket_fd, F_SETFL, flags | O_NONBLOCK), "could not set TCP listening socket to be non-blocking");
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	guard(bind(listen_socket_fd, (struct sockaddr *) &addr, sizeof(addr)), "could not bind");
	guard(listen(listen_socket_fd, 100), "could not listen");
	for (;;) {
		int client_socket_fd = accept(listen_socket_fd, NULL, NULL);
		if (client_socket_fd == -1) {
			if (errno == EWOULDBLOCK) {
				printf("No pending connections; sleeping for one second.\n");
				sleep(1);
			} else {
				perror("error when accepting connection");
				exit(1);
			}
		} else {
			char msg[] = "hello\n";
			printf("Got a connection; writing 'hello' then closing.\n");
			send(client_socket_fd, msg, sizeof(msg), 0);
			close(client_socket_fd);
		}
	}
	return EXIT_SUCCESS;
}


uint32_t
rc_crc32(uint32_t crc, const char *buf, size_t len)
{
	static uint32_t table[256];
	static int have_table = 0;
	uint32_t rem;
	uint8_t octet;
	int i, j;
	const char *p, *q;
 
	/* This check is not thread safe; there is no mutex. */
	if (have_table == 0) {
		/* Calculate CRC table. */
		for (i = 0; i < 256; i++) {
			rem = i;  /* remainder from polynomial division */
			for (j = 0; j < 8; j++) {
				if (rem & 1) {
					rem >>= 1;
					rem ^= 0xedb88320;
				} else
					rem >>= 1;
			}
			table[i] = rem;
		}
		have_table = 1;
	}
 
	crc = ~crc;
	q = buf + len;
	for (p = buf; p < q; p++) {
		octet = *p;  /* Cast to unsigned octet. */
		crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
	}
	return ~crc;
}
 
int checksum(const char *number)
{
	const char *s = number;
	printf("%" PRIX32 "\n", rc_crc32(0, s, strlen(s)));
	CRC_val = rc_crc32(0, s, strlen(s));
	printf("VAL: %d", CRC_val);
	printf("\n ");
	return 0;
}

int genRandoms(int lower, int upper) { 
	int num = (rand() % (upper - lower + 1)) + lower; 
    return num; 
} 

void *primary_check(void *threadid) 
{ 
	//sleep(0.1);
	printf("\n Starting Communication Primary Check \n");
	int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	char buffer[1024] = {0}; 
	const char* hello = "STILL RUNNING"; 
	const char* reboot_message = "REBOOTED";
	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{ 
		printf("socket failed"); 
		return 0; 
	} 
	
	// int fileflags;
	// if (fileflags = fcntl(server_fd, F_GETFL, 0) == -1) {
    // 	perror("fcntl F_GETFL");
    // 	exit(1);
	// }
	// if (fcntl(server_fd, F_SETFL, fileflags | FNDELAY) == -1){
    // 	perror("fcntl F_SETFL, FNDELAY");
    // 	exit(1);
	// }
	// Forcefully attaching socket to the port 8080 
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
												&opt, sizeof(opt))) 
	{ 
		printf("setsockopt failure"); 
		return 0; 
	} 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( 9034 ); 
	
	// Forcefully attaching socket to the port 8080 
	if (bind(server_fd, (struct sockaddr *)&address, 
								sizeof(address))<0) 
	{ 
		printf("bind failed"); 
		return 0;  
	} 
	if (listen(server_fd, 3) < 0) 
	{ 
		printf("listen"); 
		return 0;
	} 
	printf("\n PRIMARY CHECK WAITING FOR CONNECTIONS \n");
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
					(socklen_t*)&addrlen))<0) 
	{ 
		printf("accept"); 
		return 0;  
	} 
	// printf("\n test node started \n");
	// int listen_socket_fd = guard(socket(AF_INET, SOCK_STREAM, 0), "could not create TCP listening socket");
	// int flags = guard(fcntl(listen_socket_fd, F_GETFL), "could not get flags on TCP listening socket");
	// guard(fcntl(listen_socket_fd, F_SETFL, flags | O_NONBLOCK), "could not set TCP listening socket to be non-blocking");
	// struct sockaddr_in addr;
	// addr.sin_family = AF_INET;
	// addr.sin_port = htons(9034);
	// addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// guard(bind(listen_socket_fd, (struct sockaddr *) &addr, sizeof(addr)), "could not bind");
	// guard(listen(listen_socket_fd, 100), "could not listen");
	// int client_socket_fd = accept(listen_socket_fd, NULL, NULL);
	// for(int i =0; i<40; i++){
	// 	if (client_socket_fd == -1) {
	// 		if (errno == EWOULDBLOCK) {
	// 			printf("No pending connections; sleeping for one second.\n");
	// 			sleep(1);
	// 		} else {
	// 			perror("error when accepting connection");
	// 			exit(1);
	// 		}
	// 	} else {
	// 		valread = recv( new_socket , buffer, sizeof(buffer), 0); 
	// 		printf("%s\n",buffer ); 
	// 		if (reboot != 1){
	// 			send(new_socket , reboot_message , strlen(reboot_message) , 0 ); 
	// 			printf("\n REBOOT status sent \n"); 
	// 			valread = recv( new_socket , buffer, sizeof(buffer), 0); 
	// 			printf("\n iFile is: ");
	// 			printf("%s\n",buffer );
	// 			iFile = buffer;
	// 			printf("Ending Communication \n");
	// 			close(client_socket_fd);
	// 			break;
	// 		} else {
	// 			printf("\n Running status sent SUCCESSFUL!\n"); 
	// 			printf("\n Ending Communication \n"); 
	// 			char msg[] = "hello\n";
	// 			printf("Got a connection; writing 'hello' then closing.\n");
	// 			send(client_socket_fd, msg, sizeof(msg), 0);
	// 			close(client_socket_fd);
	// 			break;
	// 		}
	// 	}
	// }
	printf("\n AT READ \n ");
	valread = recv( new_socket , buffer, sizeof(buffer), 0); 
	printf("%s\n",buffer ); 
	if (reboot != 1){
		send(new_socket , reboot_message , strlen(reboot_message) , 0 ); 
		printf("\n REBOOT status sent \n"); 
		valread = recv( new_socket , buffer, sizeof(buffer), 0); 
		printf("\n iFile is: ");
		printf("%s\n",buffer );
		iFile = buffer;
		printf("Ending Communication \n");
	} else {
		printf("\n Running status sent SUCCESSFUL!\n"); 
		send(new_socket , hello , strlen(hello) , 0 ); 
		printf("Running status sent\n"); 
		printf("Ending Communication \n");
	}
	close(new_socket);
	return 0;
} 


void *replica_check(void *threadid) 
{
    printf("\n Starting Communication Replica Check \n");
	int sock = 0, valread; 
	struct sockaddr_in serv_addr; 
	const char *hello = "RUNNING"; 
	const char *rebootMessage = iFileSEND;
	char buffer[1024] = {0}; 
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		printf(strerror(errno));
		return 0; 
	} 

	// fcntl(sock, F_SETFL, O_NONBLOCK);

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(9034); 

	// int fileflags;
	// if (fileflags = fcntl(sock, F_GETFL, 0) == -1) {
    // 	perror("fcntl F_GETFL");
    // 	exit(1);
	// }
	// if (fcntl(sock, F_SETFL, fileflags | FNDELAY) == -1){
    // 	perror("fcntl F_SETFL, FNDELAY");
    // 	exit(1);
	// }
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		return 0;   
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed with Primary CHECK \n");
		printf(strerror(errno));
		//listen(sock);
		//select(sock, &rset, &wset, NULL, NULL)
		return 0;  
	} 
	send(sock, hello , strlen(hello) , 0 ); 
	printf("Check in message sent, Replica CHECK waiting to read\n"); 
	valread = recv( sock , buffer, sizeof(buffer), 0); 
	if (buffer[0] == 'R'){
		printf("%s\n",buffer ); 
		printf("\n running rebooted version \n");
		printf("\n THIS IS THE IFILE BEING SENT: ");
		printf(iFileSEND);
		printf("\n that was IFile");
		send(sock , rebootMessage , strlen(rebootMessage) , 0 ); 
		printf(rebootMessage);
		printf("\n iFile sent \n");
	} else{
		printf("%s\n",buffer ); 
		printf("\n PRIMARY IS RUNNING! \n");
	}
	return 0;
} 

void *rebootPrimaryCheck(void *threadid) 
{ 
	//sleep(0.1);
	printf("\n Starting REBOOTED Communication AS Primary \n");
	int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	char buffer[1024] = {0}; 
	const char* hello = "REBOOTED"; 
	printf("\n here 0");
	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{ 
		printf("socket failed"); 
		return 0; 
	} 
	printf("\n here 1");
	// Forcefully attaching socket to the port 8080 
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
												&opt, sizeof(opt))) 
	{ 
		printf("setsockopt failure"); 
		return 0; 
	} 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( 9034 ); 
	
	// Forcefully attaching socket to the port 8080 
	if (bind(server_fd, (struct sockaddr *)&address, 
								sizeof(address))<0) 
	{ 
		printf("bind failed"); 
		return 0;  
	} 
	if (listen(server_fd, 3) < 0) 
	{ 
		printf("listen failed"); 
		return 0;
	} 
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
					(socklen_t*)&addrlen))<0) 
	{ 
		printf("accept failed"); 
		return 0;  
	} 
	printf("\n here 2");
	send(new_socket , hello , strlen(hello) , 0 ); 
	printf("\n here 3");
	printf("\n REBOOT status sent \n"); 
	valread = recv( new_socket , buffer, sizeof(buffer), 0); 
	printf("\n iFile is: ");
	printf("%s\n",buffer );
	iFile = buffer;
	printf("Ending Communication \n");
	close(new_socket);
	return 0;
} 

void *crash(void *threadid)
{
	usleep(5000);
    while (true){
        if(rand() <= 21474836470){
            printf("CRASH !!!!!!! CRASH !!!!!!! \n");
		    exit(EXIT_FAILURE);
		    exit(1);
        }
        usleep(5000);
	   //printf("\n CRASH! \n");
	   // exit(EXIT_FAILURE);
    }
    return 0;
}

void *masterNode(void *threadid)
{
   int rc;
   printf("Running master \n");
   long t = 2;
   while(true){
	   	//if (wait == 0 ){
			if (primary == 1){
				rc = pthread_create(&threads[t], NULL, primary_check, (void *)t);
			}
			else {
				rc = pthread_create(&threads[t], NULL, replica_check, (void *)t);
			}
			usleep(50);
			printf("waiting to finish \n");
			pthread_join(threads[2], NULL);
			printf("\n finished \n");
			usleep(500000);
		//}
		// else {
		// 	printf("\n no longer communicating!!!!! \n ");
		// 	sleep(1);
		// }
   }
   return 0;
}

void timeMIS(graph<intT> G, int rounds, char* outFile) {
  graph<intT> H = G.copy(); //because MIS might modify graph
  char* flags = maximalIndependentSet(H);
  printf("\n HERE \n");
  printf("\n round is: ");
  printf("%d", rounds);
  srand(time(0));
  //char check[32];
  for (int i=0; i < rounds; i++) {	
	if (genRandoms(1, 1000000) < 100){
		return;
	}
	for (int x =0; x < sizeof(flags); x++){
		//char add[6];
		//printf("flags \n");
		//cout << int(flags[x]);
		//printf("checker: \n");
		checker = checker*10 + int(flags[x]);
		//cout << checker;
		//sprintf(add, "%f", int(flags[x]));
		//strcat(check, add);
	}
	printf("\n this was flags \n");
	char buf[32];
	sprintf(buf, "%d", checker);
	const char *p = buf;
	printf("\n this is p: ");
	cout << p;
	checksum(p);
	printf(typeid(flags).name());
    free(flags);
    H.del();
    H = G.copy();
    startTime();	
    flags = maximalIndependentSet(H);
    nextTimeN();
  }
  cout << endl;
  printf("\n now printing outFile \n");
  if (outFile != NULL) {
    int* F = newA(int, G.n);
    for (int i=0; i < G.n; i++) 
    {
      F[i] = flags[i];
    }
    writeIntArrayToFile(F, G.n, outFile);
    free(F);
  }
  
  free(flags);
  G.del();
  H.del();
}


int runAsPrimary(char* name){
  printf("\n running server as primary \n");
	int sock = 0, valread; 
	struct sockaddr_in serv_addr; 
	char *fileName = name; 
	char buffer[1024] = {0}; 
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(PORT); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\n Invalid address/ Address not supported \n"); 
		return -1; 
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed \n"); 
		return -1; 
	} 
	send(sock , fileName , strlen(fileName) , 0 ); 
	printf("\n Filename message sent\n"); 
	valread = recv( sock , buffer, sizeof(buffer), 0); 
	printf("%s\n",buffer ); 
	return 0;
}

void *checkCRCprimary(void *threadid)
{
	sprintf(str,"%d", CRC_val);
	printf("\n");
	printf(str);
	printf("\n");
  	printf("\n Checker running as primary \n");
	int sock = 0, valread; 
	struct sockaddr_in serv_addr; 
	char *fileName = str; 
	char buffer[1024] = {0}; 
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		printf("COULD NOT CONFIRM WITH REPLICA");
		pthread_exit(NULL);
	} 

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(8000); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		printf(strerror(errno));
		printf("\n COULD NOT CONFIRM WITH REPLICA \n");
		pthread_exit(NULL);
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed \n"); 
		printf("COULD NOT CONFIRM WITH REPLICA");
		pthread_exit(NULL);
	} 
	//while (waiting == 0){
	//	printf("\n waiting \n");
	//	usleep(500);
	//}
	send(sock , fileName , strlen(fileName) , 0 ); 
	printf("CRC message sent\n"); 
	valread = recv( sock , buffer, sizeof(buffer), 0); 
	printf("%s\n",buffer ); 
	//wait = 0;
	return 0;
}


int runAsReplica(){
  	printf("\n running as replica \n");
  	int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	char buffer[1024] = {0}; 
	string ackstr = "acknowledge";
	char *ack = (char*) ackstr.c_str();
	printf("\n waiting here START \n");
	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{ 
		printf("socket failed"); 
		return 1; 
	} 
	printf("\n waiting here start \n ");
	// Forcefully attaching socket to the port 8080 
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
												&opt, sizeof(opt))) 
	{ 
		printf("setsockopt"); 
		return 1; 
	} 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( PORT ); 
	printf("\n waiting here start \n ");
	// Forcefully attaching socket to the port 8080 
	if (bind(server_fd, (struct sockaddr *)&address, 
								sizeof(address))<0) 
	{ 
		printf("bind failed"); 
		return 1; 
	} 
	printf("\n waiting here start2 \n ");
	if (listen(server_fd, 3) < 0) 
	{ 
		printf("listen"); 
		return 1; 
	} 
	printf("\n waiting here start3 \n ");
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
					(socklen_t*)&addrlen))<0) 
	{ 
		printf("accept"); 
		return 1; 
	} 
	printf("\n waiting here READ start \n ");
	valread = recv( new_socket , buffer, sizeof(buffer), 0); 
	iFile = buffer;
	printf("%s\n",buffer ); 
	send(new_socket , ack , strlen(ack) , 0 ); 
	printf("Acknowledge message sent\n"); 
	close(new_socket);
	return 0;
}



void *checkForCrash(void *threadid) 
{
    printf("\n Starting Communication Replica Check \n");
	int sock = 0, valread; 
	struct sockaddr_in serv_addr; 
	const char *rebootMessage = iFileSEND;
	printf("\n THIS IS THE IFILE BEING SENT: ");
	printf(iFileSEND);
	printf("\n that was IFile");
	char buffer[1024] = {0}; 
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		return 0; 
	} 

	// serv_addr.sin_family = AF_INET; 
	// serv_addr.sin_port = htons(PORT); 
	// int fileflags;
	// if (fileflags = fcntl(sock, F_GETFL, 0) == -1) {
    // 	perror("fcntl F_GETFL");
    // 	exit(1);
	// }
	// if (fcntl(sock, F_SETFL, fileflags | FNDELAY) == -1){
    // 	perror("fcntl F_SETFL, FNDELAY");
    // 	exit(1);
	// }
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		return 0;   
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed \n");
		printf("\n FAILED HERE \n");
		perror(NULL);
		return 0;  
	} 
	valread = recv( sock , buffer, sizeof(buffer), 0); 
	if (buffer == "REBOOTED"){
		printf("\n THERE WAS A CRASH !!! \n");
		send(sock , rebootMessage , strlen(rebootMessage) , 0 ); 
		printf(rebootMessage);
		printf("\n iFile sent \n");
	}
	return 0;
} 



void *checkCRCreplica(void *threadid)
{
  	printf("\n running CHECKER as replica \n");
  	int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	char buffer[1024] = {0}; 
	char *ack = (char*) string("NOT MATCHING").c_str(); 
	char *confirm = (char*) string("CONFIRMED").c_str(); 
	printf("\n reached 0");
	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{ 
		printf("socket failed"); 
		printf("COULD NOT CONFIRM WITH PRIMARY");
		pthread_exit(NULL);
	} 
	// int fileflags;
	// if (fileflags = fcntl(server_fd, F_GETFL, 0) == -1){
	// 	perror("fcntl F_GETFL");
	// 	exit(1);
	// }
	// if (fcntl(server_fd, F_SETFL, fileflags | FNDELAY) == -1){
	// 	perror("fcntl F_SETFL, FNDELAY");
	// 	exit(1);
	// }
	// Forcefully attaching socket to the port 8080 
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
												&opt, sizeof(opt))) 
	{ 
		printf("setsockopt"); 
		printf("COULD NOT CONFIRM WITH PRIMARY");
		pthread_exit(NULL);
	} 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( 8000 ); 
	// Forcefully attaching socket to the port 8080 
	if (bind(server_fd, (struct sockaddr *)&address, 
								sizeof(address))<0) 
	{ 
		printf("bind failed"); 
		printf("COULD NOT CONFIRM WITH PRIMARY");
		pthread_exit(NULL); 
	} 
	printf("reached 3!");
	if (listen(server_fd, 3) < 0) 
	{ 
		printf("listen"); 
		printf("COULD NOT CONFIRM WITH PRIMARY");
		pthread_exit(NULL);
	} 
	printf("reached 4!");
	int again = 0;
	int counter = 0;
	while (again == 0){
		int client_socket_fd = accept(server_fd, NULL, NULL);
		if (client_socket_fd == -1) {
			if (errno == EWOULDBLOCK) {
				printf("No pending connections; sleeping for one second.\n");
				sleep(1);
				counter ++;
			} else {
				perror("error when accepting connection");
				exit(1);
			}
			// if (counter > 7){
			// 	counter = 0;
			// 	printf("\n creating crash checker \n");
			// 	long t;
			// 	t = 4;
			// 	int rc;
  	  		// 	rc = pthread_create(&threads[t], NULL, checkForCrash, (void *)t);
			// }
		} else {
			printf("Got a connection, will try and confirm checksum with primary\n");
			valread = recv( new_socket , buffer, sizeof(buffer), 0); 
			int chec = atoi(buffer);
			if (chec == int(CRC_val)){
				send(new_socket , confirm , strlen(confirm) , 0 );
				printf("Confirmation message sent\n"); 
				//wait = 0;
				return 0;
			} else {
				printf("\n Wasn't able to confirm \n");
				printf("buffer is: %s\n", buffer);
				usleep(5000);
			}
			close(client_socket_fd);
		}
	}

	// printf("\n reached READ");
	// // waiting = 1;
	// int repeat = 0;
	// int counterTwo = 0;
	// while (repeat == 0){
	// 	valread = recv( new_socket , buffer, sizeof(buffer), 0); 
	// 	int chec = atoi(buffer);
	// 	if (chec == int(CRC_val)){
	// 		send(new_socket , confirm , strlen(confirm) , 0 );
	// 		printf("Confirmation message sent\n"); 
	// 		//wait = 0;
	// 		repeat = 1;
	// 		return 0;
	// 	} else {
	// 		printf("\n Wasn't able to confirm \n");
	// 		usleep(50000);
	// 		counterTwo ++;
	// 		printf("%d", counterTwo);
	// 	}
	// 	if (counterTwo > 10){
	// 		counterTwo = 0;
	// 		long t;
	// 		t = 4;
	// 		int rc;
  	//   		rc = pthread_create(&threads[t], NULL, checkForCrash, (void *)t);
	// 	}	
	// }
	return 0;
	
}


int sendToClient(int arg, int CRC_val){
	#undef PORT
	#define PORT 3490
    printf("\n sending to client \n");
    int sock = 0, valread; 
	struct sockaddr_in serv_addr; 

    char name[300]; 
	sprintf(name, "%d", arg);
	char *fileName = name; 

	char nameTwo[300]; 
	sprintf(nameTwo, "%d", CRC_val);
	char *fileNameTwo = nameTwo; 
	
	char buffer[1024] = {0}; 
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(PORT); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\n Invalid address/ Address not supported \n"); 
		return -1; 
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed With Client\n"); 
		printf(strerror(errno));
		return -1; 
	} 
	send(sock , fileName , strlen(fileName) , 0 ); 
	usleep(1000);
	send(sock , fileNameTwo , strlen(fileNameTwo) , 0 ); 
	printf("\n Filename message sent TO CLIENT\n"); 
	return 0;
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc, argv, "[-o <outFile>] [-p <primary>] [-t <reboot>] [-r <rounds>] [-b <batchSize>] [-c <threadCount>] <inFile>");
  printf("\n started main \n");
  char* iFile = P.getArgument(0);
  iFileSEND = iFile;
  char* oFile = P.getOptionValue("-o");
  int rounds = P.getOptionIntValue("-r",1);
  primary = P.getOptionIntValue("-p", 1);
  reboot = P.getOptionIntValue("-t", 1);
  batchSize = P.getOptionIntValue("-b", 10000);
  int cilkThreadCount = P.getOptionIntValue("-c", -1);
  int rc;
  long t;
  printf("about to create threads \n");
  if(primary == 1 && reboot != 2){
    runAsPrimary(iFile);
	printf("\n reboot is: ");
  	printf("%d", reboot);
  } else if (primary == 1 && reboot == 2){
		t = 0;
    	rc = pthread_create(&threads[t], NULL, masterNode, (void *)t);
		pthread_join(threads[0], NULL);
  }
  else if(primary != 1) {
    runAsReplica();
  }
  t = 0;
  rc = pthread_create(&threads[t], NULL, masterNode, (void *)t);
  if (primary == 1 && reboot != 2){
	  t = 1;
  	  rc = pthread_create(&threads[t], NULL, crash, (void *)t);
  }
//   if(reboot == 2 && primary ==1){
// 	  printf("\n rebooted version \n");
// 	  t = 0;
//   	  rc = pthread_create(&threads[t], NULL, rebootPrimaryCheck, (void *)t);
// 	  printf("\n waiting here to get iFile \n");
// 	  pthread_join(threads[0], NULL);
//   } else {
// 	  printf("\n running for first time \n");
//  }
  if(cilkThreadCount > 0)
	{
		//std::string s = std::to_string(cilkThreadCount);
		char num[3];
		sprintf(num,"%d",cilkThreadCount);
		__cilkrts_end_cilk();
		__cilkrts_set_param("nworkers", num);
		__cilkrts_init();
		std::cout << "The number of threads " << cilkThreadCount << std::endl;
	}
  graph<intT> G = readGraphFromFile<intT>(iFile);
  timeMIS(G, rounds, oFile);
  sprintf(str,"%d", CRC_val);
  char *message = str;
  sendToClient(checker, CRC_val);
  #undef PORT
  #define PORT 8080
  if (primary != 1){
	  t = 3;
	  //wait = 1;
  	  rc = pthread_create(&threads[t], NULL, checkCRCreplica, (void *)t);
  }
  if (primary == 1){
	  t = 3;
	  //wait = 1;
	  usleep(1000);
  	  rc = pthread_create(&threads[t], NULL, checkCRCprimary, (void *)t);
  }
  printf("check:  ");
  cout << checker;
  cout << CRC_val;
  pthread_join(threads[3], NULL);
  printf("DONE");
  printf("EXITING");
  exit(0);
}