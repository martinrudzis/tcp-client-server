#include <sys/types.h> // socket, bind
#include <sys/socket.h> // socket, bind, listen, inet_ntoa
#include <netinet/in.h> // htonl, htons, inet_ntoa
#include <arpa/inet.h> // inet_ntoa
#include <netdb.h> // gethostbyname
#include <unistd.h> // read, write, close
#include <strings.h> // bzero
#include <netinet/tcp.h> // SO_REUSEADDR
#include <sys/uio.h> // writev
#include <cstring>
#include <iostream>
#include <stdio.h>
#include <sys/time.h> 
#include <errno.h>

// compilation note: use ./a.out port# hostname iterations nbufs bufsize test#

void handleScenario(timeval &tInitial, timeval &tFinal, int scenario, int sd, int iterations, int nbufs, int bufsize) {
   char databuf[nbufs][bufsize];
   if (scenario == 1) {
      gettimeofday(&tInitial, NULL); // Start round-trip timer
      for (int i = 0; i < iterations; i++) {
         for (int j = 0; j < nbufs; j++) {
            write(sd, databuf[j], bufsize); // Sd: socket descriptor
         }
      }
   }
   else if (scenario == 2) {
      gettimeofday(&tInitial, NULL); // Start round-trip timer
      for (int i = 0; i < iterations; i++) { 
         struct iovec vector[nbufs]; 
         for (int j = 0; j < nbufs; j++) { 
            vector[j].iov_base = databuf[j]; 
            vector[j].iov_len = bufsize; 
         } 
         writev(sd, vector, nbufs); // sd: socket descriptor 
      } 
   }
   else {
      gettimeofday(&tInitial, NULL); // Start round-trip timer
      for (int i = 0; i < iterations; i++){ 
         write(sd, databuf, nbufs * bufsize); // sd: socket descriptor 
      } 
   }
   gettimeofday(&tFinal, NULL); // Lap timer to get data transmission time
}

int main(int argc, char **argv) {
// Program arguments (specified in command line)
   std::string serverPort = argv[1]; // server port number
   std::string serverName = argv[2]; // server host name
   int iterations = std::stoi(argv[3]); // number of iterations to use in tests
   int nbufs = std::stoi(argv[4]); // number of buffers
   int bufsize = std::stoi(argv[5]); // data buffer size in bytes
   int type = std::stoi(argv[6]); // type of data transmission to send  
// Create new socket
   // Load address structs with getaddrinfo()
   struct addrinfo hints, *servinfo; 
   memset(&hints, 0, sizeof(hints)); // 2. Clear struct data initially
   hints.ai_family = AF_UNSPEC; // Use IPv4 or IPv6, don't specify just one
   hints.ai_socktype = SOCK_STREAM; // Use TCP

   // Call getaddrinfor to update servInfo
   getaddrinfo(serverName.c_str(), serverPort.c_str(), &hints, &servinfo); 

   struct timeval tInitial, tFinal; 
   // Open a new socket and establish a connection to the server
   // Make a socket, bind it, and listen on it
   int clientSd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

   // Avoid "Address alread in use" error
   int status = connect(clientSd, servinfo->ai_addr, servinfo->ai_addrlen);
   if (status < 0) {
      std::cerr << "Failed to connect to server." << std::endl;
      close(clientSd);
      return -1;
   } 

   gettimeofday(&tInitial, NULL); // Start round-trip timer

   handleScenario(tInitial, tFinal, type, clientSd, iterations, nbufs, bufsize); // Run tests

   // Calculate and print data transmission time in microseconds
   double tLap = (tFinal.tv_sec - tInitial.tv_sec) * 1000000 + (tFinal.tv_usec - tInitial.tv_usec); // usec
   std::cout << "Test " << type << ": data transmission time = " << tLap << " usec, ";

   int count = 0;
   int z = read(clientSd, &count, sizeof(count)); // Read server response regarding number of reads

   gettimeofday(&tFinal, NULL); // Stop round-trip timer

   // Calculate and print total round-trip time in microseconds
   double tTotal = (tFinal.tv_sec - tInitial.tv_sec) * 1000000 + (tFinal.tv_usec - tInitial.tv_usec); // usec
   std::cout << "round-trip time = " << tTotal << " usec, " << "#reads = " << count << std::endl;

   close(clientSd);
}