#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define PORT 8080
#define BUFFER_SIZE 1024
#define BACKLOG 5

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE]; 
    //A 1024-byte array on the stack. This is where incoming data from the network lands
    //Stack allocation is fine here because it lives for the duration of main().


    //Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    //AF_INET - Address Family Internet. IPv4. If you wanted IPv6, you'd use AF_INET6
    //SOCK_STREAM - This is what makes it TCP. Stream socket = reliable, ordered, connection-oriented, bidirecitonal byte stream
    //The alternative is SOCK_DGRAM = UDP = unreliable, unordered, connectionless packets
    // 0 - Protocol. When 0 is passed, the OS picks the default protocol for the given socket type. For SOCK_STREAM + AF_INET,
    //that's always TCP (protocol number 6). you could also explicitly pass IPPROTO_TCP

    // return value: An integer >= 0 on success(the file descriptor). -1 on failure and sets errno.


    if (server_fd < 0) {
        perror("Socket Failed");
        //prints:Socket Failed: <OS error message> ie.'too many open files'
        exit(EXIT_FAILURE);
        //terminates the process with status 1. I clean up nothing here because server_fd is invalid, so there's nothing to close
    }

    //setsockopt() - Set socket options.One of the most important calls you'll make in any real server (avoid "Address already in use")
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { //telling the os to allow this port to be reused immediately after program exit.
        //server_fd - the socket to configure
        //SOL_SOCKET - Socket Option Level. This tells the OS which layer of the network stack youre configuring. SOL_SOCKET
        //means the socket layer itself (as opposed to IPPROTO_TCP which configures TCP-level options)
        //SO_REUSEADDR - Socket Option:Reuse Address. Without this, if you run your server, kill it with Ctrl+C, and immediately restart it, you
        //get Bind Failed: Address already in use. Why? Because TCP has a state called TIME_WAIT - WHen a connection closes, the OS keeps the port "reserved" for ~60 secs
        //to catch any delayed packets stll in transit. SO_REUSEADDR tells the OS: "let me resue this adderss even if it's in TIME_WAIT." Essential for development
        //&opt - pointer to the option value. opt = 1 means "enable this option". Passing 0 would disable it
        //sizeof(opt) - size of the option calue (4 bytes for an int).

        //Return 0 on success, -1 on failure
        
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
        //Note: here we close(server_fd) before exiting - because server_fd IS valid 
        //(socket creation succeeded), so we must release it.
    }

    //Configure server address
    server_addr.sin_family = AF_INET; 
    // specify ipv4. Every sockaddr_in must have this set. The OS uses it to interpret the rest of the struct correctly. THink of it as a type tag.
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    //Bind to all interfaces (equivilant to 0.0.0.0). If your machine has multiple NICs(WIFi +Ethernet+loopback), this accepts connections on any of them. If you wanted to only accept local connections,
    //you'd use inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr). s_addr is the raw 32-bit IPv4 address stored as a uint32_t
    server_addr.sin_port = htons(PORT); 
    //htons() = Host TO Network Short. THis is about byte order (endianness)
    // -Little-endian (x86/ARM): least significant byte stored first. Port 8080 = 0x1F90. In memory: 90 1F.
    // - Big-endian/Network byte order: most significant byte first. In memory: 1F 90.
    //necessary for cross-architecture compatibility (port number to network byte order (big endian))

    //The network protocol(TCP/IP) always uses big-endian. If you skip htons() on a little-endian machine, you'd bind to port 0x901F = 36895 instead of 8080.
    //The client and server would never agree on what port to use. htons ensures portability across architectures. 

    //Bind socket to IP and port
    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        //bind() - Associates the socket with a specific address and port.
        // The OS created a socket with socket(), but it's floating in space with no address.
        //bind() nails it to 0.0.0.0:8080
        
        // -server_fd - which socket
        
        // (struct sockaddr*)&server_addr - The cast is necessary because bind() is a generic function that works with multiple addresses
        //families (IPv4, IPv6, Unix domain sockets, etc.). The generic type is struct sockaddr. We pass our struct sockaddr_in* cast to 
        // struct sockaddr*. The OS knows the real type because of sin_family.
        
        // - sizeof9server_addr) - so the OS knows how many bytes to read
        
        //After bind(), port 8080 is "claimed" - no other process can bind to it (unless they also use SO_REUSEADDR).
        perror("Bind Failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listens with backlog
    if (listen(server_fd, BACKLOG) < 0){
        //listen() - Marks the socket as passive(a server socket) and sets up the connection queue.

        //This is a pivotal moment. Before listen(), the socket is just a network endpoint. After listen(),the 
        //OS starts managing a connection queue for this socket. When clients call connect(), the TCP three-way handshake happens in the kernel -
        //completely automatically - and the completed connection sits in this queue waiting for accept().

        // -server_fd - the socket to make passive
        // - BACKLOG (=5) -the maximum number of fully established connections waiting in the queue. If 6 clients connect before accept() is called,
        //the 6th gets a Connection refused or a timeout depending on the OS. In production servers, this is set much higher (often 128 or SOMAXCONN)

        //Key insight: listen() does NOT block. It just flips a flag on the socket and returns immediately. The blocking 
        //call is accept()
        perror("Listen Failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while(1){ //infinite loop to accept mult clients sequentially
        //The server runs forever until killed (Ctrl+C, SIGTERM,etc..). Each iteration handles one complete client
        //interaction. THis makes the server sequential/iterative -it cannot handle a second client until it finishes with the first.
        //A real production server would fork() a child process or use pthread_create() to handle clients concurrently.
        
        //Accept new client
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        //accept() -The most important call on the server side. This is BLOCKING call.

        //The thread sleeps here until a client connects. When one does:
        // 1. The OS dequeues one completed connection from the backlog queue
        // 2. Creates a brand new socket specifically for that client
        // 3. Returns the new file descriptor (client_fd)
        // 4. Fills client_addr with the client's IP and port
        // 5. Updates client_len with the actual size written

        //server_fd stays open and keeps listening. client_fd is the private communication channel with this one client. This is why you need two FDs.

        if (client_fd < 0) {
            //On failure, continue goes back to the top of the while(1) loop to try accept()
            //again. We don't exit the server just because one accept failed. The server keeps running.
            perror("Accept Failed");
            continue; //go back to accept next client
        }

        printf("Client Connected.\n");

        //Receive data from client (blocking)
        memset(buffer, 0, BUFFER_SIZE);
        //memset(ptr, value, count) - fills count bytes starting at ptr with value. This zeroes out the entire buffer before reading into it. Why? Because the buffer is a stack 
        //variable that may contain garbage from a previous iteration or from memory reuse. Without this, if the new message is shorter
        //than the previous one, old bytes would remain in the buffer after the new message, potentially causing bugs or security issues.

        ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        //recv() - Receive data from a socket.
        // -client_fd - read from the client's socket
        // -buffer -where to put the received bytes
        //BUFFER_SIZE - 1 (=1023) - max bytes to read. The -1 reserves the last byte for the null terminator '\0' so we can 
        //treat the buffer as a C string safely
        //0 -flags. 0 = normal blocking receive. other flags:MSG_PEEK (read without consuming), MSG_WAITALL (wait for full buffer), MSG_DONTWAIT (non-blocking).

        //Return value (ssize_t - signed size type):
        // - >0: number of bytes actually received
        // - =0: the client gracefully closed the connection (sent a TCP FIN)
        // - <0: error occurred

        //Critical TCP concept: recv() is NOT guaranteed to return all the bytes in one call. TCP is a byte stream -there are no message boundaries. If the client sends 500 bytes, recv() might 
        // return 200 the first time and 300 the second time. For this simple project, it works fine because messages are short, but production code needs a loop.

        //Manual null termination. recv() does NOT null-terminate - it just copies raw bytes. Without this line, printf("%s", buffer) would read past the received
        //data into whatever garbage follows in memory - undefined behavior. We write '\0' at exactly bytes_received, which is safe because we read at most BUFFER_SIZE - 1 bytes.

        if (bytes_received < 0){
            perror("Recv Failed");
        }
        else if (bytes_received == 0) {
            printf("Client diconnected immediately.\n");
        }
        else{
            buffer[bytes_received] = '\0'; //ensure null-terminated
            printf("Client says: %s\n", buffer);

            //Send response
            char *response = "Message Received.\n";
            send(client_fd, response, strlen(response), 0);
            //send() - Transmit data through a socket.
            // -client_fd -send to this client
            // -response -pointer to data to send
            // -strlen(response) - number of bytes to send. NoteL strlen does NOT count the null
            //terminator, so we're not sending '\0' -just the printable string bytes. That's intentional; the reciever handles its own null termination.
            // -0 -flags (same as recv:0 =default behavior)

            //Return value: number of bytes actaully sent, or -1 on error. The return value is not checked here - a real production server
            //should check it and potentially retry if fewer bytes were sent than requested (partial sends are possible)
        }

        //CLose connection with this client
        close(client_fd);
        printf("Client Connection Closed.\n");
        
        //close(fd) - Release the file descriptor. This triggers the TCP FIN sequence - a graceful shutdown. The OS sends a FIN packet to the client, signaling 
        //"I'm done sending." The client's recv() will return 0. File descriptors are a finite OS resource. If you forget to close(), you have a file descriptor leak.
        //On Linux, the default per-process FD limit is 1024. A server that leaks FDs will eventually crash with "Too Many Open Files"
    }

    //close server socket (unreachable in this loop)
    close(server_fd);
    //This is never reached because of the while(1) loop - but it's good practice to write it anyway. It signals to the reader (and static analysis tools) that you understand
    //resource management. In a real server with signal handling, you'd catch SIGINT and break out of the loop gracefully.

    return 0;
}