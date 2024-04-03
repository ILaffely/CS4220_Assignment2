#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define EOTSTRING "$end$of$transmission$"
#define PACKET 64
#define DATALIMIT 1023
#define MAX 80
#define BUFFSIZE 2000
#define WINDOW 6
#define TIMEOUT 2000

volatile sig_atomic_t print_flag = false;

void handle_alarm( int sig ){
	print_flag = true;
}

struct dataPacket {
    int type;
    int seq_no;
    int length;
    char data[1024];
};
typedef struct dataPacket dataPacket;

struct segmentPacket createDataPacket (int seq_no, int length, char* data);
struct segmentPacket createTerminalPacket (int seq_no, int length);

long int FileSize(char* fileName){
	FILE* filep = fopen(buffer, "r");
		
	if (filep == NULL){
		printf("File not Found!\n");
		return -1;
	}

	fseek(filep, 0l, SEEK_END);

	long int fsize = ftell(filep);

	fclose(filep);

	return fsize;
}

dataPacket* packager(char* buffer, int packetCount)
{
	char filebuffer[PACKET];
	dataPacket packets[packetCount]; 

	packets[0] = createTitlePacket(0,strlen(buffer),buffer);

	int packetCount = 1;
	//packeter
	while(!feof(filep)){
		bzero(filebuffer, sizeof(filebuffer));
		int offset = fread(filebuffer, sizeof(char), PACKET, filep); //amount of chars read
		packets[packetCount] = createDataPacket(packetCount,offset,filebuffer);
		packetCount++;
	}

	packets[packetCount] = createTerminalPacket(packetCount);

	return 0;
}

void GBNSend(dataPacket* packets, int packetsLength, int socket_ID, struct sockaddr_in server_addr, char server_buffer[],int server_struct_len){
	signal( SIGALRM, handle_alarm );
	
	int sendBase = 0;
	int nextSeqNum = 0;
	bool timer = false;
	int countdown = TIMEOUT;
	clock_t before = 0;
	clock_t difference = 0;

	int noTerminalACK = 1;
	while(noTerminalACK){
		/*send packets from base to window size*/
		while(nextSeqNum <= packetsLength && nextSeqNum <= sendBase + WINDOW){
			if(sendto(socket_ID, (dataPacket*)&packets[nextSeqNum], sizeof(char)*offset, 0, 
				 (struct sockaddr*)&server_addr, server_struct_len) < 0){
					printf("Unable to send message.\n");
					return -1;
				}
			nextSeqNum++;
		}
		//rest goes below
		
	}
	/*while (true){
		if (nextSeqNum < sendBase + WINDOW){
			//send packet nextSeq
			nextSeqNum = nextSeqNum + 1
		}

		if(recvfrom(socket_ID, server_buffer, sizeof(server_buffer), 0,
		 (struct sockaddr*)&server_addr, &server_struct_len) < 0){
			printf("Error receiving server message.\n");
			return -1;
		}

		if(strcmp("$ACK#", server_buffer, 5) == 0){
			sendBase = n + 1;
			if (sendBase == nextSeqNum){
				//stop timer
				timer = false;
			}
			else{
				//start timer
				timer = true;
				before = clock();
			}
		}
		if(countdown == 0){
			//start timer
			int i;
			for(i = sendBase; i < nextSeqNum; i++){
				//send packet i
			}
			countdown = TIMEOUT;
		}
		if (timer == true){
			difference = clock() - before;
			countdown -= difference;
		}
	}*/
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

		while((buffer[n++] = getchar()) != '\n');

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

			long int fsize = FileSize(buffer);
			int packetsCount = (((fsize/sizeof(char))+(PACKET - 1))/PACKET) + 2; // number of packets

			dataPacket* packets = packager(buffer, packetsCount);

			//send the message to the server
			GBNSend(packets, packetsCount, socket_ID, server_addr, server_struct_len);
			
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

	pkt.type = 1;
	pkt.seq_no = seq_no;
	pkt.length = length;
	memset(pkt.data, 0, sizeof(pkt.data));
    strcpy(pkt.data, data);

	return pkt;
}

dataPacket createTerminalPacket (int seq_no){

    struct segmentPacket pkt;

    pkt.type = 2;
    pkt.seq_no = seq_no;
    pkt.length = 0;
    memset(pkt.data, 0, sizeof(pkt.data));

    return pkt;
}