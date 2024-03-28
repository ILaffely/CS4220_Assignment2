#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define BUFFSIZE 2000
int main(void){
	int socket_ID;
	struct socketaddr_in server_addr;
	char server_buffer[BUFFSIZE], client_buffer[BUFFSIZE];
	int server_struct_len = sizeof(server_addr);

	//clean buffers
	memset(server_buffer, '\0', sizeof(server_buffer));
	memset(client_buffer, '\0', sizeof(client_buffer));

	//create socket
	socket_ID = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	if(socket_ID < 0){
		printf("Error: Socket creation failed./n");
		return -1;
	}
	printf("Socket Created Successfully.\n");

	//set port / IP
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(BUFFSIZE);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	//user input
	printf("Message> ");
	fgets(client_buffer,BUFFSIZE);

	//send the message to the server
	if(sendto(socket_ID, client_buffer, strlen(client_buffer), 0, 
	(struct sockaddr*)&server_addr, server_struct_len) < 0){
		printf("Unable to send message.\n");
		return -1;
	}

	//get server response
	if(recvfrom(socket_ID, server_buffer, sizeof(server_buffer), 0,
	(struct sockaddr*)&server_addr, server_struct_len) < 0){
		printf("Error receiving server message.\n");
		return -1;
	}

	printf("Server> %s\n", server_buffer);

	return 0;
}
