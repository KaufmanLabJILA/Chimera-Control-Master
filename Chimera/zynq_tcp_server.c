#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>
#define MAX 80
#define DEVLEN 3
#define PORT 8080 
#define SA struct sockaddr 
  
// Function that reads commands from Chimera and passes them to the axis-fifo interface of the Zynq FPGA
void chimera_interface(int sockfd)
{ 
    char dev[DEVLEN];
	unsigned int length;
	int n = 0;

    // infinite loop for recieving commands 
    for (;;) { 
        bzero(dev, DEVLEN);

		while (getchar() != '_')
			;
        // read the message from client and copy it in buffer 
        read(sockfd, dev, sizeof(dev));

		// print dev number
		printf("dev num: %s", dev);

		n = 0;
		while (getchar() != '\n')
			++n;
		char* buffer = new char[n-1];
		read(sockfd, buffer, sizeof(buffer));

		/*ReadXBytes(sockfd, sizeof(length), (void*)(&length));
		char* buffer = new char[length];
		ReadXBytes(sockfd, length, (void*)buffer);*/

        // print buffer which contains the client contents 
        printf("command from Chimera: %s", buffer);
  
        // if msg contains "Exit" then server exits 
        /*if (strncmp("exit", buff, 4) == 0) { 
            printf("Server Exit...\n"); 
            break; 
        } */
		delete [] buffer;
    } 

} 

// This assumes buffer is at least x bytes long,
// and that the socket is blocking.
void ReadXBytes(int socket, unsigned int x, void* buffer)
{
	int bytesRead = 0;
	int result;
	while (bytesRead < x)
	{
		result = read(socket, buffer + bytesRead, x - bytesRead);
		if (result < 1)
		{
			// Throw your error.
		}

		bytesRead += result;
	}
}
  
// Driver function 
int main() 
{ 
    int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    len = sizeof(cli); 
  
    // Accept the data packet from client and verification 
    connfd = accept(sockfd, (SA*)&cli, &len); 
    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server acccepted the client...\n"); 
  
    // Function for reading DIO, DAC, and DDS commands from Chimera
    chimera_interface(connfd);
  
    // After chatting close the socket 
    close(sockfd); 
}
