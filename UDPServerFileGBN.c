#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdlib.h>

//constants
#define BUFF 1024
#define PORT 2099
#define EOTSTRING "$end$of$transmission$"
#define LOSS_RATE 0.25

//structs
struct dataPacket {
    int type;
    int seqNo;
    int length;
    char data[512];
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
int loosePacketOutOfTen();

int main(void){
    int socketDesc;
    struct sockaddr_in serverAddr, clientAddr;
    char clientMessage[BUFF];
    unsigned int clientLen;
    int messageSize;
    int seqNum;
    int valid;
    int base;
    FILE *outputFile;

    //clean server adderss
    memset(&serverAddr, 0, sizeof(serverAddr));

    // Create UDP socket:
    socketDesc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if(socketDesc < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");
    
    // Set port and IP:
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Bind to the set port and IP:
    if(bind(socketDesc, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
        printf("Binding with port failed\n");
        return -1;
    }
    printf("Binding sucsess\n");


    //service loop
    valid = 1;
    base = -2;
    int ackVal = 1;
    int oneTerm = 0;
    while (valid) {

        clientLen = sizeof(clientAddr);

        dataPacket inboundPacket;

        ACKPacket ACK;


        //wait until you get a message from the client and store it in inbound packet
        if ((messageSize = recvfrom(socketDesc, &inboundPacket, sizeof(inboundPacket), 0,
            (struct sockaddr *) &clientAddr, &clientLen)) < 0) 
        {
            printf("Receiving failed\n");
            return -1;
        }
        seqNum = inboundPacket.seqNo;
        printf("\n\nMessage Size Received: %d\n",messageSize);
        //schedruled packet loss for testing purposes
        if (!loosePacketOutOfTen()) 
        {

            //check if this is the initial packet
            if (inboundPacket.type == 1 && inboundPacket.seqNo == 0) 
            {

                //zero out the buffer
                memset(clientMessage, '\0', sizeof(clientMessage));


                //transfer the data from the packet to the buffer with a received prefix to distinguish the received file when run locally
               sprintf(clientMessage, "received--%s", inboundPacket.data);
                printf("\n\nInitial packet received with filename: %s",inboundPacket.data);
                //open a file for writing
                outputFile = fopen(clientMessage, "w");

                if (outputFile == NULL) {
                    printf("Error opening file\n");
                    return -1;
                }

                //printing information about the file
                memset(clientMessage, '\0', sizeof(clientMessage));
                printf("Initial packet received from %s\n", inet_ntoa(clientAddr.sin_addr));

                fprintf(outputFile, "Initial packet received from %s\n%s\n", 
                    inet_ntoa(clientAddr.sin_addr), clientMessage);

                base = 0;
                updateACK(&ACK,inboundPacket.type,base);

            } else if (inboundPacket.type == 2 && inboundPacket.seqNo == base + 1) //if it is not the initial packe, check if it is the correct sequential packet'
            {
                printf("Correct packet received. Packet: %d\nPacket Type: %d", inboundPacket.seqNo, inboundPacket.type);
                printf("\n\nData: -- %s\n\n", inboundPacket.data);
                //print data to file
                inboundPacket.data[inboundPacket.length] = '\0';
                fprintf(outputFile, "%s", inboundPacket.data);
                
                //update base and ack
                base++;
                updateACK(&ACK, inboundPacket.type, base);
                ackVal = 1;

            } else if (inboundPacket.type == 2 && inboundPacket.seqNo != base + 1) //if it is a data packet but the sequence is wrong
            {
                printf("Out of sequence packet received. Packet: %d\n", inboundPacket.seqNo);
                memset(clientMessage, '\0', sizeof(clientMessage));
                //update ack with num of last in seq packet DO NOT update base

                updateACK(&ACK,inboundPacket.type,base);

                ackVal= 0;


            } 

            //check for the terminal packet, which should also follow the last data packet
            if (inboundPacket.type == 3 && inboundPacket.seqNo == base + 1) {
                
                //update the base to inactive
                base = -1;
                //reset the message size to 0
                messageSize = 0;
                //print EOF
                printf("End of Message\n");
                fprintf(outputFile,"\nEnd of Message\n");

                //close the output file
                fclose(outputFile);
                memset(clientMessage, '\0', sizeof(clientMessage));
                updateACK(&ACK, inboundPacket.type, base);
            } 

            if (base >= 0 && ackVal == 1) //Send ACK for each packet in sequence, do not send ack for out os sequence packets
            {
                printf("Sending ACK: %d\n Sent to -- %s\nACK type: %d\n", base, inet_ntoa(clientAddr.sin_addr),ACK.type);
                if (sendto(socketDesc, &ACK, sizeof(ACK), 0,
                (struct sockaddr *) &clientAddr, sizeof(clientAddr)) != sizeof(ACK)) {
                    printf("Incorrect number of bytes sent");
                    return -1;
                }
            //if the base is -1 the terminal package was recived
            } else if (base == -1) {
                //reset base to -2 so that another terminal signal will not be sent until another file is recived
                base = -2;
                printf("Terminal signal received, EOF confirmed\n");
                if (sendto(socketDesc, &ACK, sizeof(ACK), 0,
                (struct sockaddr *) &clientAddr, sizeof(clientAddr)) != sizeof(ACK)) {
                    printf("Incorrect number of bytes sent");
                    return -1;

                }
            }
        } else {
            printf("Packet ignored: Loss Simulated");
        }
    }
    
    //currently this cant be accessed, thinking about adding a kill signal
    printf("\nLoop killed, server shutdown\n");
    // Close the socket
    close(socketDesc);
    
    return 0;
}

//tried to create a function that randomly lost packets, problem is rand is based on time
//and often the transmission completes in a single tick
//I could use a better random function from a different library 
//but the fixed loss rate below worked just fine for testing
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

//loss simulator that loses one out of every 10 packets
//useful for gaurenteeing a simulated loss
//10 was way too slow for alice in wonderland, changed to 1/50
//still works though
int loosePacketOutOfTen(){
    static int count = 0; 
    count++; 

    if (count % 50 == 0) {
        return 1; 
    } else {
        return 0; 
    }
}

//a function to update the ack packet
void updateACK (ACKPacket* ack, int type, int num) {
    ack->type = type;
    ack->num = num;
}