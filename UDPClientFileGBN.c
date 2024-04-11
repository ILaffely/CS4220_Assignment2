#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define EOTSTRING "$end$of$transmission$"
#define PACKET 64
#define DATALIMIT 1023
#define MAX 80
#define BUFFSIZE 2047
#define WINDOW 6
#define TIMEOUT 3
#define MAXTRIES 10

int tries = 0;
	
char server_buffer[BUFFSIZE], client_buffer[BUFFSIZE];

void CatchAlarm(int ignored);


struct dataPacket {
    int type;
    int seq_no;
    int length;
    char data[1024];
};
typedef struct dataPacket dataPacket;

dataPacket* allPackets;

struct ACKPacket {
    int type;
    int ack_no;
};

dataPacket createTitlePacket(int seq_no, int length, char* data);
dataPacket createDataPacket(int seq_no, int length, char* data);
dataPacket createTerminalPacket(int seq_no, int length);

long int FileSize(FILE* fileName){

	fseek(fileName, 0l, SEEK_END);

	long int fsize = ftell(fileName);

	return fsize;
}

void packager(char buffer[MAX], int packetCount)
{
	char filebuffer[PACKET];
	allPackets = malloc(packetCount * sizeof(dataPacket)); 
	/*int i;
	for (i = 0; i < packetCount; i++){
		packets[i] = createDataPacket(NULL,NULL,NULL);
	}*/

	allPackets[0] = createTitlePacket(0,strlen(buffer),buffer);

	int currPacket = 1;
	FILE* filep = fopen(buffer, "r");
		
	if (filep == NULL){
		printf("File not Found!\n");
		return;
	}
	//packeter
	while(!feof(filep)){
		bzero(filebuffer, sizeof(filebuffer));
		int offset = fread(filebuffer, sizeof(char), PACKET, filep); //amount of chars read
		allPackets[currPacket] = createDataPacket(currPacket,offset,filebuffer);
		currPacket++;
	}
	fclose(filep);
	allPackets[currPacket] = createTerminalPacket(currPacket, 0);
}

void GBNSend(int packetsLength, int socket_ID, struct sockaddr_in server_addr,int server_struct_len){
	
	int sendBase = -1;
	int nextSeqNum = 0;
	int dataLength = 0;

	int respStringLen;

	struct sigaction AlrmSig;
	AlrmSig.sa_handler = CatchAlarm;
	if (sigemptyset(&AlrmSig.sa_mask) < 0){ printf("sigfillset() failed"); return; }
	AlrmSig.sa_flags = 0;

	if (sigaction(SIGALRM, &AlrmSig, 0) < 0){ printf("sigaction() failed for SIGALRM"); return; }

	int noTerminalACK = 1;
	while(noTerminalACK){
		/*send packets from base to window size*/
		while(nextSeqNum <= packetsLength && nextSeqNum <= sendBase + WINDOW){
			if(sendto(socket_ID, &allPackets[nextSeqNum], sizeof(char)*strlen(allPackets[nextSeqNum].data), 0, 
				 (struct sockaddr*)&server_addr, server_struct_len) < 0){
					printf("Unable to send message.\n");
					return;
				}
			nextSeqNum++;
		}
		
		//timer
		alarm(3);

		struct ACKPacket ack;

		while(respStringLen = recvfrom(socket_ID, &ack, sizeof(ack), 0,
		 (struct sockaddr*)&server_addr, &server_struct_len) < 0){

			if (errno == EINTR){ /*alarm activated*/
				//reset to one after last successfull ack
				nextSeqNum = sendBase + 1;

				printf("Timeout: Trying Again");
				if (tries >= MAXTRIES){
					printf("Max Attemps Exceded: Returning");
					return;
				}
				else{
					alarm(0);

					while(nextSeqNum <= packetsLength && nextSeqNum <= sendBase + WINDOW){
						if(sendto(socket_ID, &allPackets[nextSeqNum], sizeof(char)*strlen(allPackets[nextSeqNum].data), 0, 
				 		 (struct sockaddr*)&server_addr, server_struct_len) < 0){
							printf("Unable to send message.\n");
							return;
						}
						nextSeqNum++;
					}
					alarm(TIMEOUT);
				}
				tries++;
			}
			else{
				printf("recvfrom() failed");
				return;
			}
		 }
		if(ack.type != 2){
			printf("-------------------->>> Recieved ACK: %d\n", ack.ack_no);
            if(ack.ack_no > sendBase){
                /* Advances the sendbase, reset tries */
                sendBase = ack.ack_no;
            }
        } else {
            printf("Recieved Terminal ACK\n");
            noTerminalACK = 0;
        }
		alarm(0);
		tries = 0;
	}
}

int main(void){
	int socket_ID;
	struct sockaddr_in server_addr;
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

	//send file
	char buffer[MAX];
	int n;

	while(1){
		bzero(buffer, MAX);
		//to send a txt file
		printf("Enter File Name: ");
		n = 0;

		while((buffer[n++] = getchar()) != '\n' && strlen(buffer) < MAX );

		if(strncmp("exit",buffer,4) == 0){
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

			long int fsize = FileSize(filep);
			int packetsCount = (((fsize/sizeof(char))+(PACKET - 1))/PACKET) + 2; // number of packets

			packager(buffer, packetsCount);

			//send the message to the server
			GBNSend(packetsCount, socket_ID, server_addr, server_struct_len);
			
			free(allPackets);
			/**/

			bzero(buffer, MAX);
		}
	}


	/*//get server response
	if(recvfrom(socket_ID, server_buffer, sizeof(server_buffer), 0,
	(struct sockaddr*)&server_addr, &server_struct_len) < 0){
		printf("Error receiving server message.\n");
		return -1;
	}

	printf("Server> %s\n", server_buffer);*/

	return 0;
}

void CatchAlarm(int ignored)     /* Handler for SIGALRM */
{
    //printf("In Alarm\n");
}

dataPacket createTitlePacket(int seq_no, int length, char* data){
	dataPacket pkt;

	pkt.type = 0;
	pkt.seq_no = seq_no;
	pkt.length = length;
	memset(pkt.data, 0, sizeof(pkt.data));
    strcpy(pkt.data, data);

	return pkt;
}

dataPacket createDataPacket(int seq_no, int length, char* data){
	dataPacket pkt;

	pkt.type = 1;
	pkt.seq_no = seq_no;
	pkt.length = length;
	memset(pkt.data, 0, sizeof(pkt.data));
    strcpy(pkt.data, data);

	return pkt;
}

dataPacket createTerminalPacket(int seq_no, int length){

    dataPacket pkt;

    pkt.type = 2;
    pkt.seq_no = seq_no;
    pkt.length = 0;
    memset(pkt.data, 0, sizeof(pkt.data));

    return pkt;
}