#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define EOTSTRING "$end$of$transmission$"
#define PACKET 50
#define MAX 80
#define BUFFSIZE 2000
#define WINDOW_SIZE 5

int chat(int socket_ID, struct sockaddr_in server_addr, int server_struct_len)
{
	char buffer[MAX];
	int n;
	int base = 0; // base of the window
	int nextseqnum = 0; // next sequence number to be sent
	int ack; // acknowledgement received

	while(1){
		bzero(buffer, MAX);
		//to send a txt file
		printf("Enter File Name: ");
		n = 0;

		while((buffer[n++] = getchar()) != '\n');

		if(strncmp("exit",buffer,4) == 0){
			write(socket_ID, buffer, sizeof(buffer));
			printf("Client Exit...\n");
			break;
		}
		else{
			buffer[strcspn(buffer, "\n")] = 0;
			FILE* filep = fopen(buffer, "r");
		
			if (filep == NULL){
				printf("File not Found!\n");
				continue;
			}

			char filebuffer[PACKET];
			while(!feof(filep)){
				if(nextseqnum < base + WINDOW_SIZE){ // Send packets within the window
					bzero(filebuffer, sizeof(filebuffer));
					int offset = fread(filebuffer, sizeof(char), PACKET, filep); //amount of chars read
					if(sendto(socket_ID, filebuffer, sizeof(char)*offset, 0, 
					 (struct sockaddr*)&server_addr, server_struct_len) < 0){
						printf("Unable to send message.\n");
						return -1;
					}
					nextseqnum++;
				}
				else {
					printf("Window full, waiting for acknowledgements...\n");
					// Receive acknowledgements
					if(recvfrom(socket_ID, &ack, sizeof(ack), 0,
						(struct sockaddr*)&server_addr, &server_struct_len) < 0){
						printf("Error receiving acknowledgement.\n");
						return -1;
					}
					while(base < ack){
						printf("Received ACK for packet %d\n", ack);
						base++;
					}
				}
			}
			if(sendto(socket_ID, EOTSTRING, sizeof(EOTSTRING), 0, 
				 (struct sockaddr*)&server_addr, server_struct_len) < 0){
					printf("Unable to send message.\n");
					return -1;
				}
			bzero(buffer, MAX);
			//read
			//printf("From Server: %s\n", buffer);
		}
	}
	return 0;
}


int main(void){
	int socket_ID;
	struct sockaddr_in server_addr;
	char server_buffer[BUFFSIZE], client_buffer[BUFFSIZE];
	int server_struct_len = sizeof(server_addr);

	//clean buffers
	memset(server_buffer, '\0', sizeof(server_buffer));
	memset(client_buffer, '\0', sizeof(client_buffer));

	//create socket
	socket_ID = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	if(socket_ID < 0){
		printf("Error: Socket creation failed.\n");
		return -1;
	}
	printf("Socket Created Successfully.\n");

	//set port / IP
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(BUFFSIZE);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	//user input
	//printf("Message> ");
	//fgets(client_buffer,BUFFSIZE, stdin);

	//send the message to the server
	chat(socket_ID, server_addr, server_struct_len);

	//get server response
	if(recvfrom(socket_ID, server_buffer, sizeof(server_buffer), 0,
	(struct sockaddr*)&server_addr, &server_struct_len) < 0){
		printf("Error receiving server message.\n");
		return -1;
	}

	printf("Server> %s\n", server_buffer);

	return 0;
}
