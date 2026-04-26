#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock;
    //the client only needs one socket(sock) and only needs the server's address struct (it fills out itself, and
    // connect() fills in the client side automatically from an ephermeral port)
    //Ephermeral Ports: When client calls connect() without calling bind() first, the OS automatically assigns the client a random
    //available port (typically 49152-65535). This is called an ephermeral port. That's why clients don't need to bind()
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket Failed");
        exit(EXIT_FAILURE);
    }

    //Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    //inet_pton() - Internet Presentation TO Network. Converts a human-readable IP
    //address string to binary network representation.

    //AF_INET - tells the function it's an IPv4 address
    //"127.0.0.1" - the loopback address. This always refers to "this machine". Client
    //and server are running on the same machine, so we connect to outselves

    //"127.0.0.1" --> stoed as 0x7F000001 in network byte order (big-endian). The inverse function
    //is inet_ntop() (Network TO Presentation).

    //?Why not just write server_addr.sin_addr.s_addr = 127001? 
    // -Because that integer interpretation would be wrong due to byte ordering and dotted-decimal
    //encoding. Always use inet_pton for IP address conversion

    //Connect
    if(connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        //connect() - Initiates the TCP three-way handshake.
        //This is a blocking call (by default). Here's what happens under the hood:
        //1. Client --> Server: SYN packet (synchronize sequence numbers)
        //2. Server --> Client: SYN-ACK (acknowledge client's SYN, send server's SYN)
        //3. Client --> Server: Server: ACK (acknowledge server's SYN)
        //4. Connection is now ESTABLISHED on both sides

        //All of this happens inside connect(). If the server isnt running, connect() returns -1 with 
        // errno = ECONNREFUSED. If the server is unreachable, it times out (can take 75+ seconds on some systems)

        // After connect(0 succeeds, sock is a fully bidirectional channel to the server. You can send() and recv() on it.)
        perror ("Connection Failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");

    printf("Enter Message: ");
    fgets(buffer, BUFFER_SIZE, stdin);
    //fgets(buffer, size, stream) - Reads up to size - 1 characters from stream into buffer, stopping at newline or EOF. It includes the newline('\n') in the bufffer and 
    //always null-terminates from C11 because it has no bounds checking - a classic buffer overflow vulnerability.)

    //Send
    send(sock, buffer, strlen(buffer), 0);
    //strlen(buffer) -counts bytes up to (not including) the ull terminator. The newline from fgets IS included, so the server 
    // receives "Hello\n" not "Hello", That's why the server's print looks normal - the \n is already there

    //receive
    memset(buffer, 0, BUFFER_SIZE);
    recv(sock, buffer, BUFFER_SIZE, 0);
    printf("Server says: %s\n", buffer);

    close(sock);
    return 0;
}