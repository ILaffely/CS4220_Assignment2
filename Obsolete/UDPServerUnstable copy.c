#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFF 2000
#define PORT 2000
#define EOTSTRING "$end$of$transmission$"

int main(void){
    int socket_desc;
    struct sockaddr_in server_addr, client_addr;
    char client_message[BUFF];
    int client_struct_length = sizeof(client_addr);
    int valid;
    int eof;

    time_t rawtime;
    struct tm *timeinfo;
    char time_string[80];
    FILE *message = fopen("log.txt", "a"); //clear the recived file
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
        printf("Binding with port failed\n");
        return -1;
    }
    printf("Binding sucsess\n");
    valid = 1;
    eof = 1;
    while (valid && eof) {
       

        // Loop to receive multiple messages until delimiter is reached:
        while (eof) {
            // Receive client's message:
            if (recvfrom(socket_desc, client_message, sizeof(client_message), 0,
                (struct sockaddr*)&client_addr, &client_struct_length) < 0){
                printf("Receiving failed\n");
                valid = 0;// Break out of the loop if receiving fails
            }
            else {
                printf("Message received -- IP: %s and port: %i\n",
                    inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));


                if (strcmp(client_message, EOTSTRING) == 0) {
                    
                    printf("EOF Reached... Terminating\n"); 
                    printf("Getting message from client\n\n");
                    //get the time
                    time(&rawtime);
                    timeinfo = localtime(&rawtime);

                    // Format the time as a string
                    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", timeinfo);

                    // Print the current time
                    fprintf(message, "--Message received-- %s\n", time_string);

                    eof = 0;

                } else { 
                    
                    // Print to file if string is not EOFSTRING
                    fprintf(message, "%s", client_message);
                    printf("Appending to file -- %s\n", client_message);

                    //clear buffers
                    memset(client_message, '\0', sizeof(client_message));
                }       
               
            }
            
        }
        
        eof = 1;
        if (sendto(socket_desc, "File received", strlen("File received"), 0,
            (struct sockaddr*)&client_addr, client_struct_length) < 0){
            printf("Can't send\n");
        }
    }
    // Close the file
    fclose(message);
    // Close the socket:
    close(socket_desc);
    
    return 0;
}
