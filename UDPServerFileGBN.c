#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdlib.h>

//constants
#define BUFF 2048
#define PORT 2000
#define EOTSTRING "$end$of$transmission$"
#define LOSS_RATE 0.05

//structs
struct dataPacket {
    int type;
    int seq_no;
    int length;
    char data[1024];
};
typedef struct dataPacket dataPacket;

struct ACKPacket {
    int type;
    int num;
};
typedef struct ACKPacket ACKPacket;

//functions 
int losePacket(float lossRate);
void updateACK (ACKPacket* ack, int type, int num);

int main(void){
    int socket_desc;
    struct sockaddr_in server_addr, client_addr;
    char client_message[BUFF];
    int client_struct_length;
    int messageSize;
    int seqNum;
    int valid;
    int base;
    FILE *outputFile;

    //clean server adderss
    memset(&server_addr, 0, sizeof(server_addr));

    // Create UDP socket:
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");
    
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Binding with port failed\n");
        return -1;
    }
    printf("Binding sucsess\n");


    //service loop

    valid = 1;
    base = -1;
    while (valid) {

        client_struct_length = sizeof(client_addr);

        dataPacket inboundPacket;

        ACKPacket ACK;

        //wait until you get a message from the client and store it in inbound packet
        if ((messageSize = recvfrom(socket_desc, &inboundPacket, sizeof(inboundPacket), 0,
            (struct sockaddr *) &client_addr, &client_struct_length)) < 0) 
        {
            printf("Receiving failed\n");
            return -1;
        }
        seqNum = inboundPacket.seq_no;

        //random packet loss for testing purposes, controlled by LOSS_RATE
        if (!losePacket(LOSS_RATE)) 
        {

            //check if this is the initial packet
            if (inboundPacket.type == 0 && inboundPacket.seq_no == 0) 
            {

                //zero out the buffer
                memset(client_message, '\0', sizeof(client_message));

                //transfer the data from the packet to the buffer with a received postfix to distinguish the received file when run locally
               sprintf(client_message, "received--%s", inboundPacket.data);

                //open a file for writing
                outputFile = fopen(client_message, "w");

                if (outputFile == NULL) {
                    printf("Error opening file\n");
                    return -1;
                }

                printf("Initial packet received from %s\n", inet_ntoa(client_addr.sin_addr));

                fprintf(outputFile, "Initial packet received from %s\n%s\n", 
                    inet_ntoa(client_addr.sin_addr), client_message);

                base = 0;

                updateACK(&ACK,inboundPacket.type,base);

            } else if (inboundPacket.seq_no == base + 1) //if it is not the initial packe, check if it is the correct sequential packet'
            {
                printf("Correct packet received. Packet: %d\n", inboundPacket.seq_no);

                //print data to file
                fprintf(outputFile, "%s", inboundPacket.data);
                
                //update base and ack
                base++;
                updateACK(&ACK, inboundPacket.type, base);

            } else if (inboundPacket.type == 1 && inboundPacket.seq_no != base + 1) //if it is a data packet but the sequence is wrong
            {
                printf("Out of sequence packet received. Packet: %d", inboundPacket.seq_no);

                //update ack with num of last in seq packet DO NOT update base

                updateACK(&ACK,inboundPacket.type,base);
            } 

            //check for the terminal packet, which should also be the last 
            if (inboundPacket.type == 2 && inboundPacket.seq_no == base + 1) {
                
                //update the base to inactive
                base = -1;

                //print EOF
                printf("End of Message\n");
                fprintf(outputFile,"\nEnd of Message\n");

                //close the output file
                fclose(outputFile);
                memset(client_message, '\0', sizeof(client_message));

            }

            if (base >= 0) //Send ACK for each packet
            {
                printf("Sending ACK:%d\n Sent to -- %s", base, inet_ntoa(client_addr.sin_addr));
                if (sendto(socket_desc, &ACK, sizeof(ACK), 0,
                (struct sockaddr *) &client_addr, sizeof(client_addr)) != sizeof(ACK)) {
                    printf("Incorrect number of bytes sent");
                    return -1;
                }
            } else if (base == -1) {
                printf("Terminal signal received, EOF confirmed\n");
                if (sendto(socket_desc, &ACK, sizeof(ACK), 0,
                (struct sockaddr *) &client_addr, sizeof(client_addr)) != sizeof(ACK)) {
                    printf("Incorrect number of bytes sent");
                    return -1;

                }
            }
        } else {
            printf("Packet ignored: Loss Simulated");
        }
    }
    
    printf("\nLoop killed, server shutdown\n");
    // Close the socket
    close(socket_desc);
    
    return 0;
}

int losePacket(float lossRate) {
    srand(time(0));
    int random;
    random = rand();
    float lossVal = lossRate * RAND_MAX;
    int loss;
    if (random < lossVal) {
        loss = 1;
    } else {
        loss = 0;
    }
    return loss;
}

//a function to update the ack packet
void updateACK (ACKPacket* ack, int type, int num) {
    ack->type = type;
    ack->num = num;
}