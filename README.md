# UnixNetworkSockets
## Purpose:
This client-server application demonstrates network communication using sockets. It operates with two sockets—a client and a server—that communicate over a predefined IP address and port. The client sends a key and a file's contents to the server, which encrypts the data using the key and returns the encrypted result. The server supports I/O multiplexing, enabling it to handle multiple clients simultaneously, up to a preset limit of five.

## Installing:

### Obtaining
```sh
git clone https://github.com/ClownCrest/UNIX_Network_Sockets
```

### Building
```sh
gcc server.c -o server
gcc client.c -o client
```

### Running
```sh
./server -ip <Server IP Address> -p <Port to run on>
./client -ip <Server IP Address> -p <Port> -f <Filename> -key <Keyword>
```
  
## Examples
```sh
./server -p 8000 -ip 10.0.0.30
./client -ip 10.0.0.30 -p 8000 -f hello.txt -key test
```
