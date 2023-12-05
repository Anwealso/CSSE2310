// CSSE2310 Assignment 4: Brute-Force Password Cracking
// 
// server.c
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
#include <ctype.h>

#include <csse2310a3.h>
#include <csse2310a4.h>
#include <crypt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>


/* -------------------------------------------------------------------------- */
/*                                  CONSTANTS                                 */
/* -------------------------------------------------------------------------- */
#define MAX_CONNECTIONS 0 // Default maximum connections (no limit)


/* -------------------------------------------------------------------------- */
/*                                   STRUCTS                                  */
/* -------------------------------------------------------------------------- */
// Struct to hold the thread arguments
typedef struct ThreadArgs {
    int* fdPtr;
    char* dictFile;
} ThreadArgs;


/* -------------------------------------------------------------------------- */
/*                            FUNCTION DECLATATIONS                           */
/* -------------------------------------------------------------------------- */
void parse_args(int argc, char* argv[], int* maxconn, int* portNum, 
        char** dictionary);
int open_listen(int portNum);
void process_connections(int fdServer, int maxconn, char* dictionary);
// void* client_thread(void* fdPtr, char* dictFile);
void* client_thread(void* arg);
// char* capitalise(char* buffer, int len);
char* process_command(char* command, char* dictFile);
char* encrypt(char* plaintext, char* salt);
char* decrypt_password(const char* ciphertext, const char* dictfile);
char* crack(char* ciphertext, char* threadStr, char* dictfile);

/* -------------------------------------------------------------------------- */
/*                            FUNCTION DEFINITIONS                            */
/* -------------------------------------------------------------------------- */
/**
 * Parses the given command line arguments and checks them for error
 */
void parse_args(int argc, char* argv[], int* maxconn, int* portNum, 
        char** dictionary) {
    // Set default values
    *maxconn = MAX_CONNECTIONS;
    *portNum = 0;
    *dictionary = "/usr/share/dict/words";

    // Parse command line arguments
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--maxconn") == 0) {
            if (i + 1 < argc) {
                *maxconn = atoi(argv[i + 1]);
                i++;
            } else {
                fprintf(stderr, "Usage: crackserver [--maxconn connections] [--port portNum] [--dictionary filename]\n");
                exit(1);
            }
        } else if (strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                *portNum = atoi(argv[i + 1]);
                i++;
            } else {
                fprintf(stderr, "Usage: crackserver [--maxconn connections] [--port portNum] [--dictionary filename]\n");
                exit(1);
            }
        } else if (strcmp(argv[i], "--dictionary") == 0) {
            if (i + 1 < argc) {
                *dictionary = argv[i + 1];
                i++;
            } else {
                fprintf(stderr, "Usage: crackserver [--maxconn connections] [--port portNum] [--dictionary filename]\n");
                exit(1);
            }
        } else {
            fprintf(stderr, "Usage: crackserver [--maxconn connections] [--port portNum] [--dictionary filename]\n");
            exit(1);
        }
    }
}

/**
 * Listens on given port. Returns listening socket (or exits on failure)
*/
int open_listen(int portNum) {\
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    char portStr[10];
    
    if (portNum != 0 && (portNum < 1024 || portNum > 65535)) {
        fprintf(stderr,  "crackserver: unable to open socket for listening\n");
        exit(4);
    }

    // Convert portNum into string for addrinfo
    snprintf(portStr, sizeof(portStr), "%d", portNum);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;   // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;    // listen on all IP addresses

    int err;
    // if((err = getaddrinfo(NULL, portStr, &hints, &ai))) {
    if ((err = getaddrinfo("localhost", portStr, &hints, &ai))) {
        freeaddrinfo(ai);
        fprintf(stderr, "%s\n", gai_strerror(err));
        return 1;   // Could not determine address
    }

    // Create a socket and bind it to a port
    int listenfd = socket(AF_INET, SOCK_STREAM, 0); // 0=default protocol (TCP)

    // Allow address (port number) to be reused immediately
    int optVal = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optVal, 
            sizeof(int)) < 0) {
        perror("Error setting socket option");
        exit(1);
    }

    if(bind(listenfd, ai->ai_addr, sizeof(struct sockaddr)) < 0) {
        perror("Binding");
        return 3;
    }

    // Which port did we get?
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);
    if (getsockname(listenfd, (struct sockaddr*)&ad, &len)) {
        perror("sockname");
        return 4;
    }
    printf("crackserver: listening on port %u\n", ntohs(ad.sin_port));
    // TODO: Do we need to free 'sockaddr_in'  or 'ad' since they were memset/structs

    if(listen(listenfd, 10) < 0) {  // Up to 10 connection requests can queue
        perror("Listen");
        return 4;
    }

    // Have listening socket - return it
    return listenfd;
}

/**
 * Processes client requests utilising multi-threading
*/
void process_connections(int fdServer, int maxconn, char* dictionary) {
    int fd;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;

    // Repeatedly accept connections and process data (capitalise)
    while(1) {
        fromAddrSize = sizeof(struct sockaddr_in);
        // Block, waiting for a new connection. (fromAddr will be populated
        // with address of client)
        fd = accept(fdServer, (struct sockaddr*)&fromAddr,  &fromAddrSize);
        if (fd < 0) {
            perror("Error accepting connection");
            exit(1);
        }
     
        // Turn our client address into a hostname and print out both 
        // the address and hostname as well as the port number
        char hostname[NI_MAXHOST];
        int error = getnameinfo((struct sockaddr*)&fromAddr, 
                fromAddrSize, hostname, NI_MAXHOST, NULL, 0, 0);
        if (error) {
            fprintf(stderr, "Error getting hostname: %s\n", 
                    gai_strerror(error));
        } else {
            printf("Accepted connection from %s (%s), port %d\n", 
                    inet_ntoa(fromAddr.sin_addr), hostname,
                    ntohs(fromAddr.sin_port));
        }

        // Send a welcome message to our client
        // dprintf(fd, "Welcome\n");

        int* fdPtr = malloc(sizeof(int));
        *fdPtr = fd;

        // Setup the args to pass into the client thread
        ThreadArgs* threadArgs = malloc(sizeof(struct ThreadArgs));
        threadArgs->fdPtr = fdPtr;
        threadArgs->dictFile = dictionary;

        pthread_t threadId;
        pthread_create(&threadId, NULL, client_thread, threadArgs);
        pthread_detach(threadId);
    }
}

/**
 * Encrypts a given plaintext password
*/
char* encrypt(char* plaintext, char* salt) {    
    // Error checking: Make sure salt is valid (alphanumeric only)
    for (int i = 0; i < strlen(salt); i++) {
        if (!isalnum(salt[i])) { // If non-alphanumeric char
            return ":invalid";
        }
    }

    // If the salt is good, do the encryption
    return crypt(plaintext, salt);
}

/**
 * Decrypt the given password
*/
char* decrypt_password(const char* ciphertext, const char* dictfile) {
    // Extract the salt from the ciphertext
    char salt[3];
    salt[0] = ciphertext[0];
    salt[1] = ciphertext[1];
    salt[2] = '\0';

    // Load the dictionary file
    FILE* dictionary = fopen(dictfile, "r");
    if (dictionary == NULL) {
        perror("Error opening dictionary file");
        return NULL;
    }

    // Buffer to store a dictionary word
    char word[256];

    // Structure for crypt_r()
    struct crypt_data data;
    data.initialized = 0;

    // Decrypt each word from the dictionary and compare with the ciphertext
    while (fgets(word, sizeof(word), dictionary) != NULL) {
        // Remove newline character from the word
        word[strcspn(word, "\n")] = '\0';

        // Encrypt the word with the salt using crypt_r()
        char* encrypted = crypt_r(word, salt, &data);

        // Compare the encrypted word with the ciphertext
        if (strcmp(encrypted, ciphertext) == 0) {
            // Found a match, return the decrypted password
            fclose(dictionary);
            return strdup(word);
        }
    }

    // No match found
    fclose(dictionary);
    return NULL;
}

/**
 * Cracks a given encrypted password, recovering the original plaintext
*/
char* crack(char* ciphertext, char* threadStr, char* dictfile) {
    int threads;
    char* response;

    // Error checking: Make sure the given thread count is valid
    for (int i = 0; i < strlen(threadStr); i++) {
        if (!isdigit(threadStr[i])) { // If non-digit char (not 0-9)
            return ":invalid";
        }
    }
    // Error checking: Make sure the given thread count is >=1 and <=50
    threads = atoi(threadStr);
    if (threads < 1 || threads > 50) {
        // Client asked for invalid number of threads
        return ":invalid";
    }
    
    // Do the cracking
    if ((response = decrypt_password(ciphertext, dictfile))) {
        return response;
    }

    // If we failed to crack
    return ":failed"; 
}

/**
 * Process the command string recieved from the client
*/
char* process_command(char* command, char* dictFile) {
    printf("Client command: %s\n", command);
    fflush(stdout);

    char* response;
    char** fields;
    int fieldc;

    fields = split_line(command, ' ');

    // Count the number of fields
    fieldc = 0;
    while (fields[fieldc]) {
        fieldc++;
    }
    if (fieldc != 3) {
        return "error in command";
    }

    // Execute the clients command (either crypt or crack)
    // capitalise(buffer, numBytesRead);
    if (strcmp(fields[0], "crypt") == 0) {
        // Encrypt plaintext
        response = encrypt(fields[1], fields[2]);
    } else if (strcmp(fields[0], "crack") == 0) {
        // Crack the ciphertext
        response = crack(fields[1], fields[2], dictFile);
    } else {
        // Invalid command
        return ":invalid";
    }

    return response;
}

/**
 * The single-threaded function for processing a single client request
*/
// void* client_thread(void* fdPtr, char* dictFile) {
void* client_thread(void* arg) {
    ThreadArgs* threadArgs = (struct ThreadArgs*)arg;
    int* fdPtr = threadArgs->fdPtr;
    char* dictFile = threadArgs->dictFile;

    char* response;
    printf("In client thread\n");

    // Extract the file descriptor
    int fd = *(int*) fdPtr;
    free(fdPtr);

    int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    ssize_t numBytesRead;

    // Repeatedly read data arriving from client and send back responses
    while((numBytesRead = read(fd, buffer, BUFFER_SIZE)) > 0) {
    // while ((request = read_line(stream))) {
        // Process the command sent by the client and get response
        response = process_command(buffer, dictFile);

        // Send the result back to the client
        write(fd, response, strlen(response));

        // Clear the buffer so we don't get overwriting
        memset(buffer, 0, sizeof(buffer));
    }

    // If we get here we have hit a read error or EOF - client disconnected
    close(fd);

    free(fdPtr);
    free(threadArgs);

    // TODO: Remove
    // Print a message to server's stdout
    printf("Done with client\n");
    fflush(stdout);

    return NULL;
}

/**
//  * Function to capitalise a string (in place)
// */
// char* capitalise(char* buffer, int len) {
//     int i;

//     for(i=0; i<len; i++) {
//         buffer[i] = (char)toupper((int)buffer[i]);
//     }
//     return buffer;
// }

/* -------------------------------------------------------------------------- */
/*                                MAIN FUNCTION                               */
/* -------------------------------------------------------------------------- */
/**
 * Main function - runs the password cracking server.
*/
int main(int argc, char* argv[]) {
    int maxconn, portNum;
    char* dictionary;

    parse_args(argc, argv, &maxconn, &portNum, &dictionary);

    int fdServer = open_listen(portNum);

    process_connections(fdServer, maxconn, dictionary);

    return 0;
}




