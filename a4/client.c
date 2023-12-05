// CSSE2310 Assignment 4: Brute-Force Password Cracking
// 
// client.c
// 
// by Alex Nicholson
// 19/05/2023


/* -------------------------------------------------------------------------- */
/*                                  INCLUDES                                  */
/* -------------------------------------------------------------------------- */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <csse2310a3.h>
#include <csse2310a4.h>
#include <sys/socket.h>
#include <crypt.h>


/* -------------------------------------------------------------------------- */
/*                                  CONSTANTS                                 */
/* -------------------------------------------------------------------------- */
#define BUFFER_SIZE 100


/* -------------------------------------------------------------------------- */
/*                                   STRUCTS                                  */
/* -------------------------------------------------------------------------- */
typedef struct ArgSet {
    char* jobFile; // filename of jobfile
    char* portnum; // port to connect to on server
} ArgSet;

// typedef struct JobSpec {
//     char* command; // crypt or crack
//     // Only initialised if command == crypt:
//     char* plaintext; // the password to be encrypted
//     char* salt; // salt to be used in encryption
//     // Only initialised if command == crack:
//     char* ciphertext; // the hashed password to be cracked
//     char* numThreads; // number of threads to use for cracking
// } JobSpec;


/* -------------------------------------------------------------------------- */
/*                            FUNCTION DECLATATIONS                           */
/* -------------------------------------------------------------------------- */
void parse_args(int argc, char* argv[], ArgSet* args);
FILE* open_jobfile(char* jobFile);

void connect_to_server(char* portStr, int* sockfd);
void send_request(int sockfd, char* command);
void await_response(int sockfd, char* response);
void handle_eof(int sockfd);
void show_response(char* response);


/* -------------------------------------------------------------------------- */
/*                            FUNCTION DEFINITIONS                            */
/* -------------------------------------------------------------------------- */
/**
 * Checks the command line args to see that they're not completely broken 
 * or invalid.
 */
void parse_args(int argc, char* argv[], ArgSet* args) {
    // Check number of args is correct
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Usage: crackclient portnum [jobfile]\n");
        exit(1);
    }

    // Store the port number
    args->portnum = argv[1];

    // If a jobfile is specified, store its name
    if (argc == 3) {
        args->jobFile = argv[2];
        // printf("JOBFILE: %s\n", args->jobFile);
    } else {
        args->jobFile = NULL;
        // printf("JOBFILE: NO JOBFILE PROVIDED\n");
    }
}

/**
 * Parses the jobfile, returning an array of strings.
 */
FILE* open_jobfile(char* jobFile) {
    FILE* jobfileFd;
    // char* line;
    // int lineIndex = 0;
    // char** jobs = (char**)malloc(sizeof(char*));

    // Open jobfile for reading
    jobfileFd = fopen(jobFile, "r");
    // Jobfile can't be opened, return error
    if (!jobfileFd) {
        fprintf(stderr, "crackclient: unable to open job file \"%s\"\n", jobFile);
        exit(2);
    }

    // // Parse each line
    // while ((line = read_line(jobfileFd))) { // read_line returns NULL if EOF
    //     if (!(line[0] == '\0' || line[0] == '#')) {
    //         printf("<%s>\n", line);
    //         // If not a comment or empty line
    //         jobs[lineIndex] = strdup(line);
    //         lineIndex++;
    //         jobs = (char**)realloc(jobs, sizeof(char*) * (lineIndex + 1));
    //     }
    // }

    // jobs[lineIndex] = NULL; // Null-terminate the array
    // fclose(jobfileFd);
    return jobfileFd;
}

/**
 * Connects to the crackserver over localhost using the given port number
 */
void connect_to_server(char* portStr, int* sockfd) {
    struct addrinfo hints, *res;
    int err;
    // int portNum;

    // portNum = atoi(portStr);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // char portStr[10];
    // snprintf(portStr, sizeof(portStr), "%d", portNum);

    if ((err = getaddrinfo("localhost", portStr, &hints, &res))) {
        freeaddrinfo(res);
        fprintf(stderr, "crackclient: unable to connect to port %s\n", portStr);
        exit(3);
    }

    *sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (*sockfd == -1) {
        // perror("crackclient: socket");
        fprintf(stderr, "crackclient: unable to connect to port %s\n", portStr);
        exit(3);
    }

    if (connect(*sockfd, res->ai_addr, res->ai_addrlen)) {
    // if (connect(*sockfd, res->ai_addr, sizeof(struct sockaddr))) {
        close(*sockfd);
        // perror("Connecting");
        fprintf(stderr, "crackclient: unable to connect to port %s\n", portStr);
        exit(3);
    }

    freeaddrinfo(res);
}

/**
 * Sends a command up to the server
 */
void send_request(int sockfd, char* command) {
    if (write(sockfd, command, strlen(command)) == -1) {
        perror("crackclient: send");
        exit(3);
    }
}

/** 
 * Waits on the server to send back a response
*/
void await_response(int sockfd, char* response) {
    // Get and error check the response
    int numBytes = read(sockfd, response, BUFFER_SIZE - 1);
    if (numBytes == -1) {
        // TODO: Figure out what error this is
        perror("crackclient: read");
        exit(3);
    } else if (numBytes == 0) {
        // EOF on server socket (socket closed)
        fprintf(stderr, "crackclient: server connection terminated\n");
        exit(4);
    }
    response[numBytes] = '\0';
}

/**
 * Interpret the response of the server and display it on the console
*/
void show_response(char* response) {
    // Interpret the server response
    if (strcmp(response, ":invalid") == 0) {
        printf("Error in command\n");
    } else if (strcmp(response, ":failed") == 0) {
        printf("Unable to decrypt\n");
    } else {
        // Display the raw output from the server
        // printf("RES <%s>\n", response);
        printf("%s\n", response);
    }
}


/* -------------------------------------------------------------------------- */
/*                                MAIN FUNCTION                               */
/* -------------------------------------------------------------------------- */
/**
 * Main function - runs the password cracking client.
*/
int main(int argc, char* argv[]) {
    ArgSet args;
    FILE* input;
    int sockfd; // socket to be opened
    char* command;
    // char* response;

    // Check command line args
    parse_args(argc, argv, &args);

    // If jobfile is specified, parse it into JobSpec array
    if (args.jobFile != NULL) {
        input = open_jobfile(args.jobFile);
    } else {
        // If no jobfile specified, use stdin for input
        input = stdin;
    }

    // Connect to the server
    connect_to_server(args.portnum, &sockfd);

    // Send commands up to server and display results
    char response[BUFFER_SIZE]; // is there a max size for these???
    while ((command = read_line(input))) { // z
        if (!(command[0] == '\0' || command[0] == '#')) {
            // If not an empty line or comment

            // Remove trailing newline character from command
            command[strcspn(command, "\n")] = '\0';
            // printf("CMD <%s>\n", command);

            // Send command to the server
            send_request(sockfd, command);

            // Wait for response from the server
            await_response(sockfd, response);

            // Display the server's response
            show_response(response);

            // Clear the buffers
            free(command);
            memset(response, 0, sizeof(response));
        }
    }

    // Free all of our major data structures
    // ...
    
    return 0;
}

