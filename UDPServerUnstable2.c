#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFF 2000
#define PORT 2000

int main(void){
    int socket_desc;
    struct sockaddr_in server_addr, client_addr;
    char client_message[BUFF];
    int client_struct_length = sizeof(client_addr);
    int valid;
    FILE *message = fclose(fopen("received.txt", "w"));

    // Clean buffers:
    memset(client_message, '\0', sizeof(client_message));
    
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
        printf("Couldn't bind to the port\n");
        return -1;
    }
    printf("Done with binding\n");
    
    message = fopen("received.txt", "a");
    valid = 1;
    while (valid == 1) {
        printf("Listening for incoming messages...\n\n");
    
        // Receive client's message:
        if (recvfrom(socket_desc, client_message, sizeof(client_message), 0,
            (struct sockaddr*)&client_addr, &client_struct_length) < 0){
            printf("Couldn't receive\n");
            valid = 0;
        }
        else {
            printf("Received message from IP: %s and port: %i\n",
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
            printf("Msg from client: %s\n", client_message);
            
            //save message to a file
            

            fprintf(message, "%s", client_message);
            
            // Respond to client:
            if (sendto(socket_desc, "File received from client", strlen("File received from client"), 0,
                (struct sockaddr*)&client_addr, client_struct_length) < 0){
                printf("Can't send\n");
                valid = 0;
            }
        }
    }
    
    //close the message
    fclose(message);
    // Close the socket:
    close(socket_desc);
    
    return 0;
}