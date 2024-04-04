#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFF 50
#define PORT 2000
#define EOTSTRING "$end$of$transmission$"
#define WINDOW_SIZE 6

int main(void) {
    int socket_desc;
    struct sockaddr_in server_addr, client_addr;
    char client_message[BUFF]; server_message[BUFF];
    int client_struct_length = sizeof(client_addr);
    int valid;
    int eof;
    int nextSeqNum = 0;
    FILE *message = fopen("received.txt", "a"); //clear the received file
    fclose(message);
    message = fopen("received.txt", "a");
    // Clean buffers:
    memset(client_message, '\0', sizeof(client_message));
    memset(server_message, '\0', sizeof(server_message));
    
    // Create UDP socket:
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if (socket_desc < 0) {
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");
    
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Bind to the set port and IP:
    if (bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Binding with port failed\n");
        return -1;
    }
    printf("Binding success\n");
    valid = 1;
    eof = 1;
    while (valid && eof) {
        printf("Getting message from client\n\n");
        
        int expectedSeqNum = nextSeqNum;
        int ackedSeqNum = -1;
        int windowStart = nextSeqNum;
        int windowEnd = windowStart + WINDOW_SIZE - 1;
        int i;

        // Loop to receive multiple messages until delimiter is reached:
        while (eof) {
            // Receive client's message:
            if (recvfrom(socket_desc, client_message, sizeof(client_message), 0,
                (struct sockaddr*)&client_addr, &client_struct_length) < 0) {
                printf("Receiving failed\n");
            }
            else {
                int receivedSeqNum = atoi(client_message);
                if (receivedSeqNum == expectedSeqNum) {
                    printf("Message received -- IP: %s and port: %i\n",
                        inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    printf("Appending to file -- %s\n", client_message);

                    // Check for EOTSTRING
                    if (strcmp(client_message, EOTSTRING) == 0) {
                        printf("EOF Reached... Terminating\n");
                        eof = 0;
                    }
                    if (eof != 0){
                        // Print to file if string is not EOFSTRING
                        fprintf(message, "%s", client_message);

                        // Increment expected sequence number
                        expectedSeqNum++;
                        ackedSeqNum = receivedSeqNum;

                        // Send ACK for received sequence number
                        sprintf(server_message, "$ACK#%d", receivedSeqNum);
                        sendto(socket_desc, client_message, strlen(client_message), 0,
                        (struct sockaddr*)&client_addr, client_struct_length);

                        // If last frame of window, update window start and end
                        if (receivedSeqNum == windowEnd) {
                            windowStart = receivedSeqNum + 1;
                            windowEnd = windowStart + WINDOW_SIZE - 1;
                        }
                    }
                }
                else {
                    // Send cumulative ACK for last received frame
                    sprintf(server_message, "$ACK#%d", ackedSeqNum);
                    //print ACK for testing
                    printf(server_message);
                    sendto(socket_desc, sever_message, strlen(server_message), 0,
                        (struct sockaddr*)&client_addr, client_struct_length);
                }

                // Clear buffers
                memset(client_message, '\0', sizeof(client_message));
            }
        }
    }
    
    // Close the file
    fclose(message);
    // Close the socket:
    close(socket_desc);
    
    return 0;
}
