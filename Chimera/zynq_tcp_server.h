#include <stdio.h> 
#include <iostream>
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <typeinfo>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>
#include "stdint.h"
#include <bitset>


void writeAxisFifo(char* device_str, char* command_str);

void writeDIO(int sockfd, int numSnapshots);

std::string readBuffer(int socket, char* buffer, int bufferSize);

int chimeraInterface(int sockfd);

