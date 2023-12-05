Reference code for parsing args and reading jobfiles

```c
/**
 * Checks the command line args to see that they're not completely broken 
 * or invalid.
 */
ArgSet parse_args(int argc, char* argv[]) {
    ArgSet args;
    args.quiet = false;
    args.parallel = false;
    bool error = false;
    int argIndex = 1;
    int longIndex = 0; // the number of value long we have found so far
    int valueIndex = 0; // the number of value args we have found so far

    // Screen the first 2 args as long args (e.g. --parallel)
    while ((error == false) && (longIndex < 2) && (argv[argIndex])) {
        if (strlen(argv[argIndex]) < 2 || !(argv[argIndex][0] == '-' && argv
                [argIndex][1] == '-')) {
            break;
        } else if ((strcmp(argv[argIndex], "--parallel") == 0) && 
                (args.parallel == false)) {
            args.parallel = true;
        } else if ((strcmp(argv[argIndex], "--quiet") == 0) && 
                (args.quiet == false)) {
            args.quiet = true;
        } else {
            error = true; // Invalid long argument
        }
        argIndex++;
        longIndex++;
    }

    // Screen the remaining 2 args as values (e.g. file.txt)
    while ((error == false) && (argv[argIndex])) {
        if (valueIndex > (1)) {
            error = true; // We have recieved too many args
        } else if (strlen(argv[argIndex]) >= 2 && (argv[argIndex][0] == '-' && 
                argv[argIndex][1] == '-')) {
            error = true; // This is a long arg in the wrong place
        } else if (valueIndex == 0) {
            args.testProgram = argv[argIndex];
        } else if (valueIndex == 1) {
            args.jobFile = argv[argIndex];
        }
        argIndex++;
        valueIndex++;
    }

    // If not exactly 2 extra values (testfile and jobfile), throw an error
    if ((error == true) || (valueIndex != 2)) {
        fprintf(stderr, "Usage: testuqwordiply [--quiet] [--parallel] "
                "testprogram jobfile\n");
        exit(2);
    }
    return args;
}

/**
 * Parses and error checks the jobfile, returning a list of JobSpec objects.
 */
JobSpec** parse_jobfile(char* jobFile) {
    FILE* jobfileFd;
    FILE* inputFile;
    char* line;
    int jobIndex = 0;
    int lineIndex = 1;
    JobSpec** jobs = (JobSpec**) malloc(sizeof(JobSpec*));

    // Open jobfile for reading
    jobfileFd = fopen(jobFile, "rb");
    // Jobfile can't be opened, return error
    if (!jobfileFd) {
        fprintf(stderr, "testuqwordiply: Unable to open job file \"%s\"\n", 
                jobFile);
        exit(3);
    }

    // Parse and error check each line
    while ((line = read_line(jobfileFd))) { // read_line returns NULL if EOF
        if (!(line[0] == '\0' || line[0] == '#')) {
            // If not a comment or empty line
            JobSpec* job = parse_jobfile_line(line);
            if (!job) { // If returned job is null
                fprintf(stderr, "testuqwordiply: syntax error on line %d of "
                        "\"%s\"\n", lineIndex, jobFile);
                exit(4);
            }
            if ((inputFile = fopen(job->inputFile, "r")) == NULL) {
                fprintf(stderr, "testuqwordiply: unable to open file \"%s\" "
                        "specified on line %d of \"%s\"\n", job->inputFile,
                        lineIndex, jobFile);
                exit(5);
            }
            fclose(inputFile);
            jobs[jobIndex] = job;
            jobIndex++;
            jobs = (JobSpec**) realloc(jobs, sizeof(JobSpec*) * jobIndex + 1);
        }
        free(line);
        lineIndex++;
    }
    fclose(jobfileFd);

    if (jobIndex == 0) {
        fprintf(stderr, "testuqwordiply: no jobs found in \"%s\"\n", jobFile);
        fflush(stdout);
        exit(6);
    }

    return jobs;
}

/**
 * Parses and error checks a single line of the jobfile, returning a single 
 * JobSpec object. (Note: arg 'line' is a newline terminated string)
 */
JobSpec* parse_jobfile_line(char* line) {
    JobSpec* job = (JobSpec*) malloc(sizeof(JobSpec));
    char** fields;
    int fieldIndex;

    // Do error checking on jobfile contents
    fields = split_line(line, ',');

    fieldIndex = 0;
    while (fields[fieldIndex]) {
        if (fieldIndex == 0) {
            if (!(strlen(fields[fieldIndex]) > 0)) {
                // An empty inputfile field counts as a syntax error
                return NULL;
            }
            // First field
            job->inputFile = strdup(fields[fieldIndex]);
        } else if (fieldIndex == 1) {
            // Second field
            job->argString = strdup(fields[fieldIndex]);
        }
        fieldIndex++;
    }

    // Raise error, if wrong number of fields
    if (fieldIndex != 2) {
        return NULL;
    }

    return job;
}
```

<!-- ----------------------------- NEW PROMPT ------------------------------ -->


Crackclient Specifications:
"""
crackclient behaviour 95 If an incorrect number of command line arguments are provided then crackclient should emit the following 96
message (terminated by a newline) to stderr and exit with status 1: 97 Usage: crackclient portnum [jobfile]
If the correct number of arguments is provided, then further errors are checked for in the order below. 98 Job file error 99
If a job file is specified, and crackclient is unable to open it for reading, crackclient shall emit the following 100 message (terminated by newline) to stderr and exit with status 2: 101
crackclient: unable to open job file "jobfile"
where jobfile is replaced by the name of the specified file. Note that the file name is enclosed in double quote 102 characters. 103
3 Version 1.2
Connection error 104 If crackclient is unable to connect to the server on the specified port (or service name) of localhost, it shall 105
emit the following message (terminated by a newline) to stderr and exit with status 3: 106 crackclient: unable to connect to port N
where N should be replaced by the argument given on the command line. (This may be a non-numerical string.) 107 crackclient runtime behaviour 108
Assuming that the commandline arguments are without errors, crackclient is to perform the following actions, 109 in this order, immediately upon starting: 110
• connect to the server on the specified port number (or service name) – see above for how to handle a 111 connection error; 112
• read commands either from the jobfile, or stdin, and process them as described below; and 113
• when EOF is received on the input stream (job file or stdin), crackclient shall close any open network 114
connections, and terminate with exit status 0. 115 If the network connection to the server is closed (e.g. crackclient detects EOF on the socket), then 116
crackclient shall emit the following message to stderr and terminate with exit status 4: 117 crackclient: server connection terminated
Note that crackclient need only detect EOF on the socket when it is reading from the socket. It will 118 only do this after it has read a line from the jobfile (or stdin), sent that to the server, and is then awaiting 119 a response. It is not expected that crackclient will detect EOF on the network socket while it is waiting to 120 read from stdin. 121
crackclient shall interpret its input commands as follows: 122
• Blank lines (i.e. those lines containing no characters), and those beginning with the # character (comment 123
lines), shall be ignored 124
• All other lines shall be sent unaltered to the server (no error checking is required or to be performed) 125 Upon sending a command to the server, crackclient shall wait for a single line reply, and interpret it as follows: 126
• Response ":invalid" → emit the following text to stdout: 127
     Error in command
• Response ":failed" → emit the following text to stdout: 128 Unable to decrypt
• Otherwise, the raw output received from the server shall be output to stdout.
"""



Please write a C program called crackclient.c that follows the given Crackclient Specifications - make sure to use the previously provided reference  C code for parsing args and reading jobfiles in your answer. Document the code in great detail using inline comments. Make sure to structure the program according to the following pseudocode, implementing each dot point as its own function with the name given. You may split functionality up into additional sub-routines if needed.

## Main Function
### Init:
- Check args (check_args())
- (if jobfile specifierd) Parse jobfile into array and check for errors (parse_jobfile())
- Connect to server (connect_to_server())

### Loop:
- Read nextline of commands from the jobfile/stdin
- Send commands to the server (send_request())
- Wait for reply (await_response())
- Display the servers response (show_results())
- Check if EOF from user (stdin or jobfile) or from server and terminate appropriately (exit status 0 for client eof, 4 for server eof)


