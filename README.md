# CS4220_Assignment2
Public repo for the CS4220 group Project

This project is a joint effort between Isaac Laffely and Grayson McKenzie.



This is a simple program with two parts, a client and a server. Neither program takes arguemnts at runtime.
to use the program compile both files on a linux machine using the following
gcc -o UDPClientFileGBN UDPClientFileGBN.c
gcc -o UDPServerFileGBN UDPServerFileGBN.c

Then you can run both, ideally in seperate instances on the same machine. The program is currently hardcoded to 
only work localy so if a different IP address is desired
the user must alter the source code, line 60 for the server and line 200 for the client. 
The server and the client both use port 2099, if the socket fails to bind it is likley because the port is taken, 
the port can be changed using the 
PORT constant defined at the top of the source code files. The server simulates packet loss using a function which 
ignores 1 out of every so many packets, this number can be adjusted
by changing the LOSS_RATE constant at the top of the source file of the server, packet loss is set to 1/LOSS_RATE. 

When the client and server files are run in seperate instances the client program will ask for files, it is suggested 
that the testfile be in the same directory as the compiled program
when a valid file is entered, it will be sent in 64 byte chunks in windows of 6, with the client waiting for an ACK of each sent packet.
If an ack is not recieved then the client will resend the next packet after the last one for which an ACK was recieved. 

Files sent to the server will be saved in the same directory as the server with the name recived--"inputfilename" 
This was done to distinguish the files when the program was run in
the same directory. 

The client program can be exited by typing exit when the program asks for a file. The server will not exit under normal use
instead it must be killed with ps and then kill -9 *pid* If a socket fails to bind it is likley becasue an 
instance of the server program is still running

In terms of chalanges there are three notable ones I can think of '

First we had an issue where a portion of the data packets were not being sent, we found that this 
was due to only the size of the datafield and not the whole packet being given to sendto

We also had an issue with a segmantation fault because c could not allocate enough memory for the array of
 packets on larger file sizes, the solution to this was to use malloc, which worked and 
now files up to 153 kb have been tested.

Finally we had an issue where two terminal ACK packets were being sent by the server casuing the client to stop
transmission of the second file early, the solution to this was to add a flag that made it so that only one 
terminal packet could be sent per file.