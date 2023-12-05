
Write a c program for a web server that recieves commands from the crackclient (sends requests like this):
```c
/**
 * Connects to the crackserver over localhost using the given port number
*/
void connect_to_server(int portnum, int* sockfd) {
    struct addrinfo hints, *res;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char portStr[10];
    snprintf(portStr, sizeof(portStr), "%d", portnum);

    if ((status = getaddrinfo("localhost", portStr, &hints, &res)) != 0) {
        fprintf(stderr, "crackclient: unable to connect to port %d\n", portnum);
        exit(3);
    }

    *sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (*sockfd == -1) {
        perror("crackclient: socket");
        exit(3);
    }

    if (connect(*sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        close(*sockfd);
        perror("crackclient: connect");
        exit(3);
    }

    freeaddrinfo(res);
}

/**
 * Sends a command up to the server
*/
void send_request(int sockfd, char* command) {
    if (send(sockfd, command, strlen(command), 0) == -1) {
        perror("crackclient: send");
        exit(3);
    }
}
```


Make sure the server implementes the following specifications:
"""
Your crackserver program is to accept command line arguments as follows: 147 ./crackserver [--maxconn connections] [--port portnum] [--dictionary filename] 148
In other words, your program should accept up to three optional arguments (with associated values) – which 149 can be in any order. 150 The connections argument, if specified, indicates the maximum number of simultaneous client connections 151 to be permitted. If this is zero or missing, then there is no limit to how many clients may connect (other than 152 operating system limits which we will not test). 153 Important: Even if you do not implement the connection limiting functionality, your program must correctly 154 handle command lines which include that argument (after which it can ignore any provided value – you will 155 simply not receive any marks for that feature). 156 The portnum argument, if specified, indicates which localhost port crackserver is to listen on. If the port 157 number is absent or zero, then crackserver is to use an ephemeral port. 158 The dictionary filename argument, if specified, indicates the path to a plain text file containing one word 159 or string per line, which represents the dictionary that crackserver will search when attempting to crack 160 passwords. If not specified, crackserver shall use the system dictionary file /usr/share/dict/words. 161
Program Operation 162 The crackserver program is to operate as follows: 163 • If the program receives an invalid command line then it must print the message: 164
     Usage: crackserver [--maxconn connections] [--port portnum] [--dictionary filename]
to stderr, and exit with an exit status of 1. 165 Invalid command lines include (but may not be limited to) any of the following: 166
– any of --maxconn, --port or --dict does not have an associated value argument 167
– the maximum connections argument (if present) is not a non-negative integer 168
– the port number argument (if present) is not an integer value, or is an integer value and is not either 169 zero, or in the range of 1024 to 65535 inclusive 170
6 Version 1.2
– any of the arguments is specified more than once 171 – any additional arguments are supplied
"""