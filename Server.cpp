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
#include <pthread.h>
#include <sys/time.h> 

// compilation note: use ./a.out port# iterations

const int BUFSIZE = 1500;

struct thread_data {
   int iterations;
   int sd;
};

void *serverThreadFunction(void *data_param) {
   struct thread_data *data = static_cast<thread_data*>(data_param);
   char databuf[BUFSIZE]; // Allocate buffer
   struct timeval tInitial, tFinal;
   int count = 0;

   gettimeofday(&tInitial, NULL); // Start timer to track data-receiving time
   for (int i = 0; i < data->iterations; i++) {
      for(int nRead = 0; (nRead += read(data->sd, databuf, BUFSIZE - nRead)) < BUFSIZE; ++count);
      count++;
   }
   gettimeofday(&tFinal, NULL); // Stop timer
   write(data->sd, &count, sizeof(count)); // Send number of reads to client as reponse

   close(data->sd);
 
   // Calculate and print data receiving time in microseconds
   double tTotal = (tFinal.tv_sec - tInitial.tv_sec) * 1000000 + (tFinal.tv_usec - tInitial.tv_usec); // usec
   std::cout << "data-receiving time = " << tTotal << std::endl;

   free(data);
   return NULL;
}

int main(int argc, char **argv) {
// Program arguments (specified in command line)
   std::string serverPort = argv[1]; // server port number
   int iterations = std::stoi(argv[2]); // number of reads
// Create TCP socket listening on port
   // Load address structs with getaddrinfo()
   struct addrinfo hints, *res; 
   memset(&hints, 0, sizeof(hints)); // 2. Clear struct data initially
   hints.ai_family = AF_UNSPEC; // Use IPv4 or IPv6, don't specify just one
   hints.ai_socktype = SOCK_STREAM; // Use TCP
   hints.ai_flags = AI_PASSIVE; // Auto-fill my IP

   // Call getaddrinfor to update res
   getaddrinfo(NULL, serverPort.c_str(), &hints, &res);

   // Create socket
   int serverSd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

   if (serverSd < 0) {
      std::cout << "Error connecting." << std::endl;
   } 
   std::cout << "Server socket created." << std::endl;

   // Avoid "address already in use" error
   const int yes = 1;
   setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes));

// Bind socket
   if (bind(serverSd, res->ai_addr, res->ai_addrlen) < 0) {
      std::cout << "Error binding socket." << std::endl;
   }
   std::cout << "Listening for clients." << std::endl;
   listen(serverSd, 50); // Listen to up to 50 concurrent connections

// Accept incoming connection
   struct sockaddr_storage clientAddr;
   socklen_t clientAddrSize = sizeof(clientAddr);
   while (1) {
      int newSd = accept(serverSd, (struct sockaddr *)&clientAddr, &clientAddrSize);
      // Create a new thread to handle connection
      pthread_t newThread;
      struct thread_data *data = new thread_data;
      data->iterations = iterations;
      data->sd = newSd;
      pthread_create(&newThread, NULL, serverThreadFunction, (void*) data);
   }
}
