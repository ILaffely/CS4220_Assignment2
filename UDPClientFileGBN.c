#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PACKET 64
#define DATALIMIT 1023
#define MAX 80
#define BUFFSIZE 2047
#define WINDOW 6
#define TIMEOUT 3
#define MAXTRIES 10
#define PORT 2099

int tries = 0;
	
char server_buffer[BUFFSIZE], client_buffer[BUFFSIZE];

void CatchAlarm(int ignore);


struct dataPacket {
    int type;
    int seq_no;
    int length;
    char data[512];
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


//packager function to take the contents of the file and split it up into an array of numbered packets
void packager(char buffer[MAX], int packetCount)
{
	char filebuffer[PACKET];
	//allocate memory for packets, free() near the bottom of main
	allPackets = malloc(packetCount * sizeof(dataPacket)); //global pointer

	//initial packet
	allPackets[0] = createTitlePacket(0,strlen(buffer),buffer);

	int currPacket = 1;

	//open the file
	FILE* filep = fopen(buffer, "r");
		
	if (filep == NULL){
		printf("File not Found!\n");
		return;
	}
	//packets all file data
	while(!feof(filep)){
		bzero(filebuffer, sizeof(filebuffer));
		int offset = fread(filebuffer, sizeof(char), PACKET, filep); //amount of chars read
		allPackets[currPacket] = createDataPacket(currPacket,offset,filebuffer);
		currPacket++;
	}
	fclose(filep);
	//creates and places terminal packet
	allPackets[currPacket] = createTerminalPacket(currPacket, 0);
}


//function to send the data packets recive the ack's, loops until a terminal packet is recieved 
void GBNSend(int packetsLength, int socket_ID, struct sockaddr_in server_addr,int server_struct_len){
	
	int sendBase = -1;
	int nextSeqNum = 0;
	int dataLength = 0;
	int sentSize;

	int respStringLen;

	//set up the alarm to timeout no ack packet is recieved
	struct sigaction AlrmSig;
	AlrmSig.sa_handler = CatchAlarm;
	if (sigemptyset(&AlrmSig.sa_mask) < 0){ printf("sigfillset() failed"); return; }
	AlrmSig.sa_flags = 0;

	if (sigaction(SIGALRM, &AlrmSig, 0) < 0){ printf("sigaction() failed for SIGALRM"); return; }

	int noTerminalACK = 1;
	while(noTerminalACK){
		
		//sends packets of the specified window size to the server 
		while(nextSeqNum <= packetsLength  && nextSeqNum <= sendBase + WINDOW){
			if(sentSize = sendto(socket_ID, &allPackets[nextSeqNum], sizeof(dataPacket), 0, 
				 (struct sockaddr*)&server_addr, server_struct_len) < 0){
					printf("Unable to send message.\n");
					return;
				} else {
					printf("Packet sent...\nPacket Number: %d\nPacket Type: %d\n",allPackets[nextSeqNum].seq_no, allPackets[nextSeqNum].type);
					printf("\n\nPacket Data: \n %s\n\n",&allPackets[nextSeqNum].data);
				} 
			nextSeqNum++;
		}
		
		//timer to timeout server if no ack
		alarm(3);

		//set up the ACK packet to recieve the ACK
		struct ACKPacket ack;
		memset(&ack, 0, sizeof(ack));

		/*loops while NOT receiving an ACK*/
		while(respStringLen = recvfrom(socket_ID, &ack, sizeof(ack), 0,
		 (struct sockaddr*)&server_addr, &server_struct_len) < 0){

			if (errno == EINTR){ /*alarm activated*/
				//reset to one after last successfull ack
				nextSeqNum = sendBase;

				//try to resent until max tries have been reached
				printf("Timeout: Trying Again");
				if (tries >= MAXTRIES){
					printf("Max Attemps Exceded: Returning");
					return;
				}
				else{
					alarm(0);
					/*packet re-sending loop*/
					while(nextSeqNum <= packetsLength  && nextSeqNum <= sendBase + WINDOW){
						if(sentSize = sendto(socket_ID, &allPackets[nextSeqNum], sizeof(dataPacket), 0, 
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
		if(ack.type != 3){
			printf("-------------------->>> Recieved ACK: %d\nACK type: %d\n", ack.ack_no, ack.type);
            if(ack.ack_no > sendBase){
                // Advances the sendbase
                sendBase = ack.ack_no;
            }
        } else {
            printf("Recieved Terminal ACK\n");
            noTerminalACK = 0;
        }
		//if recvfrom recived data, cancel the timeout and reset the number of tries
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
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	//send file
	char buffer[MAX];
	int n;
	int exitFlag = 1;

	while(exitFlag){
		bzero(buffer, MAX);
		//to send a txt file
		printf("Enter File Name: ");
		n = 0;

		while((buffer[n++] = getchar()) != '\n' && strlen(buffer) < MAX );

		if(strncmp("exit",buffer,4) == 0){
			printf("Client Exit...\n");
			exitFlag = 0;
		}
		else{
			//open the file
			buffer[strcspn(buffer, "\n")] = 0;
			FILE* filep = fopen(buffer, "r");
		
			if (filep == NULL){
				printf("File not Found!\n");
				continue;
			}

			long int fsize = FileSize(filep);

			//determines # of packets needed to aid in memory allocation
			int packetsCount = (((fsize/sizeof(char))+(PACKET - 1))/PACKET) + 2; // number of packets

			//packages file data and creates packet array
			packager(buffer, packetsCount);

			//send the message to the server
			GBNSend(packetsCount, socket_ID, server_addr, server_struct_len);
			
			//free up the memory for the packet array and zero the buffer
			free(allPackets);
			bzero(buffer, MAX);
		}
	}

	return 0;
}

//Handler function for Sigalarm
void CatchAlarm(int ignore)  
{
}


//functions to create the three packet types
dataPacket createTitlePacket(int seq_no, int length, char* data){
	dataPacket pkt;

	pkt.type = 1;
	pkt.seq_no = seq_no;
	pkt.length = length;
	memset(pkt.data, 0, sizeof(pkt.data));
    strcpy(pkt.data, data);

	return pkt;
}

dataPacket createDataPacket(int seq_no, int length, char* data){
	dataPacket pkt;

	pkt.type = 2;
	pkt.seq_no = seq_no;
	pkt.length = length;
	memset(pkt.data, 0, sizeof(pkt.data));
    strcpy(pkt.data, data);
	return pkt;
}

dataPacket createTerminalPacket(int seq_no, int length){

    dataPacket pkt;

    pkt.type = 3;
    pkt.seq_no = seq_no;
    pkt.length = 0;
    memset(pkt.data, 0, sizeof(pkt.data));
    return pkt;
}