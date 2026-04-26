# TCP-PROJECT
1. Overview:
    - This TCP project implements a basic TCP client-server architecture in C using POSIX socket APIs on macOS

    The server:
        -Creates a TCP socket
        -Binds to port 8080
        -Listens for incoming connections
        -Accepts a client connection (one at a time for version 1)
        -Receives a message
        -Sends a response
        -Closes the connection

    The client:
        -Creates a TCP socket
        -Connects to the server
        -sends a message
        -receives a response
        -closes the connection

2. Technologies Used:
    -C(ANSI C)
    -POSIX socket API
    -GCC
    -make (build automation)
    -macOS terminal environment

3. Networking Concepts Implemented

    Socket Lifecycle
        -socket()
        -bind()
        -listen()
        -accept()
        -recv()
        -send()
        -close()
    
    TCP Concepts
        - IPv4 addressing (AF_INET)
        - Stream sockets (SOCK_STREAM)
        - Blocking I/O
        - Network byte order (htons())
    
    System-Level Details
        - File descriptors
        - Kernel-managed connection queue (BACKLOG)
        - Memory buffer management

5. Build Instructions

    Compile both programs:
    
        make


    Clean build artifacts:

        make clean

6. Run Instructions

    Terminal 1:

        ./server


    Terminal 2:

        ./client

7. Future Improvements

    -Multi-client concurrency using fork() or pthread

    -Non-blocking I/O (select() / poll())

    -Graceful shutdown handling

    -Modularization into header files

    -Error logging improvements
