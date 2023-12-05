// CSSE2310 Assignment 3: UQ Wordiply Tester
// by Alex Nicholson
// 29/04/2023

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <csse2310a3.h>

#define READ_END 0
#define WRITE_END 1
#define TEST_WORDIPLY 0
#define DEMO_WORDIPLY 1
#define UQCMP_OUT 2
#define UQCMP_ERR 3
#define PIPE_1 0
#define PIPE_2 1
#define PIPE_3 2
#define PIPE_4 3


/* --------------------------------- TYPES -------------------------------- */
typedef struct ArgSet {
    bool quiet;
    bool parallel;
    char* testProgram;
    char* jobFile;
} ArgSet;

typedef struct JobSpec {
    char* inputFile; // fd of input file to be given as command line input to 
    // test subject
    char* argString; // argv to be given to the test subject program
} JobSpec;

/* ----------------------------- DECLARATIONS ----------------------------- */
ArgSet parse_args(int argc, char* argv[]);
JobSpec** parse_jobfile(char* jobFile);
JobSpec* parse_jobfile_line(char* line);
void run_jobs(JobSpec** jobs, char* testProgram, bool parallel, bool quiet);
int* run_sequential(JobSpec** jobs, char* testProgram, bool quiet);
int* run_parallel(JobSpec** jobs, char* testProgram, bool quiet);
void close_pipes_except(int pipes[4][2], int exclude1, int exclude2);
void close_pipes(int pipes[4][2]);
void exec_test_wordiply(int pipes[4][2], char* inputFile, char* testProgram, 
        char** argv);
void exec_demo_wordiply(int pipes[4][2], char* inputFile, char** argv);
void exec_cmp_out(int pipes[4][2]);
void exec_cmp_err(int pipes[4][2]);
int run_job(JobSpec job, char* testProgram);

/* ------------------------------ DEFINITIONS ----------------------------- */
/**
 * Checks the command line args to see that they're not completely broken 
 * or invalid.
 */
ArgSet parse_args(int argc, char* argv[]) {
    ArgSet args;
    args.quiet = false;
    args.parallel = false;
    int argIndex = 1; // The number of args (longs or values) we have found so 
    // far. Note that since the program naem is in argv[0], we start looking 
    // for our args at argv[1]
    int longIndex; // the number of value long we have found so far
    int valueIndex; // the number of value args we have found so far
    int maxLongArgs = 2;
    int maxValueArgs = 2;

    // Screen the first 2 args as long args (e.g. --parallel)
    longIndex = 0;
    while (longIndex < maxLongArgs && argv[argIndex]) {
        if (strlen(argv[argIndex]) < 2 || !(argv[argIndex][0] == '-' && argv
                [argIndex][1] == '-')) {
            // If its shorter than 2 chars or its first 2 chars are not --, 
            // then it's not a long arg, so move on to value args
            break;
        } else if (strcmp(argv[argIndex], "--parallel") == 0) {
            args.parallel = true;
        } else if (strcmp(argv[argIndex], "--quiet") == 0) {
            args.quiet = true;
        } else {
            // Invalid long argument
            fprintf(stderr, "Usage: testuqwordiply [--quiet] [--parallel] " 
                    "testprogram jobfile\n");
            exit(2);
        }
        argIndex++;
        longIndex++;
    }

    // Screen the remaining 2 args as values (e.g. file.txt)
    valueIndex = 0;
    while (argv[argIndex]) {
        if (valueIndex > (maxValueArgs - 1)) {
            // We have recieved too many args
            fprintf(stderr, "Usage: testuqwordiply [--quiet] [--parallel] " 
                    "testprogram jobfile\n");
            exit(2);
        } else if (strlen(argv[argIndex]) >= 2 && (argv[argIndex][0] == '-' && 
                argv[argIndex][1] == '-')) {
            // This is a long arg in the wrong place (args format error)
            fprintf(stderr, "Usage: testuqwordiply [--quiet] [--parallel] " 
                    "testprogram jobfile\n");
            exit(2);
        // // TODO: Check whether this errer boundary is needed
        // } else if (argv[argIndex][0] == ' ') {
        //     // This is an arg that starts with a space
        //     fprintf(stderr,"Usage: testuqwordiply [--quiet] [--parallel] "
        //         "testprogram jobfile\n");
        //     exit(2);
        } else if (valueIndex == 0) {
            args.testProgram = argv[argIndex];
        } else if (valueIndex == 1) {
            args.jobFile = argv[argIndex];
        }
        argIndex++;
        valueIndex++;
    }

    // If not exactly 2 extra values (testfile and jobfile), throw an error
    if (valueIndex != maxValueArgs) {
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
    int jobIndex;
    int lineIndex;
    JobSpec** jobs = (JobSpec**) malloc(sizeof(JobSpec) * 1);

    // Open jobfile for reading
    jobfileFd = fopen(jobFile, "rb");
    // Jobfile can't be opened, return error
    if (!jobfileFd) {
        fprintf(stderr, "testuqwordiply: Unable to open job file \"%s\"\n", 
                jobFile);
        exit(3);
    }

    // Parse and error check each line
    jobIndex = 0;
    lineIndex = 1;
    while ((line = read_line(jobfileFd))) { // read_line returns NULL if EOF
        if (!(line[0] == '\0' || line[0] == '#')) {
            // If not a comment or empty line
            JobSpec* job = parse_jobfile_line(line);

            // If returned job is null, the line has invalid syntax
            if (!job) {
                fprintf(stderr, "testuqwordiply: syntax error on line %d of "
                        "\"%s\"\n", lineIndex, jobFile);
                exit(4);
            }
            // Check that the inputFile is openable for reading
            if ((inputFile = fopen(job->inputFile, "r")) == NULL) {
                fprintf(stderr, "testuqwordiply: unable to open file \"%s\" "
                        "specified on line %d of \"%s\"\n", job->inputFile,
                        lineIndex, jobFile);
                exit(5);
            } else {
                fclose(inputFile);
            }

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
            // TODO: Check if this is actually the correct behaviour
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

/**
 * Runs the list of jobs described by the jobspec using the specified parallel 
 * or sequential strategy and prints the results.
 */
void run_jobs(JobSpec** jobs, char* testProgram, bool parallel, bool quiet) {
    int* results;

    if (parallel) {
        results = run_parallel(jobs, testProgram, quiet);
    } else {
        results = run_sequential(jobs, testProgram, quiet);
    }

    // Print a summary of the results???
    printf("===== Test Results =====\n");
    int jobIndex = 0;
    while (results[jobIndex]) {
        printf("Test %d: %d\n", jobIndex, results[jobIndex]);
        exit(1);
    }
}

/**
 * Runs the list of given jobspecs using a sequential strategy and returns the 
 * results.
*/
int* run_sequential(JobSpec** jobs, char* testProgram, bool quiet) {
    int jobsRun;
    int* jobResults = (int*) malloc(sizeof(int)); // Dynamically allocated list
        // of the results of each job (indices correspond with childPids)

    jobsRun = 0;
    while (jobs[jobsRun]) {
        // Run the job
        printf("Starting job %d\n", jobsRun + 1);
        fflush(stdout);
        int result = run_job(*jobs[jobsRun], testProgram);
        
        // Sleep for 2s
        sleep(2);

        // [WIP] Report the result of the job
        jobResults[jobsRun] = result;
        jobsRun++;
        jobResults = (int*) realloc(jobResults, sizeof(int) * jobsRun + 1);
    }

    return jobResults;
}

/**
 * Runs the list of given jobspecs using a parallel strategy and returns the 
 * results.
*/
int* run_parallel(JobSpec** jobs, char* testProgram, bool quiet) {
    int jobsRun;
    int* jobResults = (int*) malloc(sizeof(int)); // Dynamically allocated list
        // of the results of each job (indices correspond with childPids)
    pid_t childPid;
    pid_t wpid;
    pid_t* childPids = (pid_t*) malloc(sizeof(pid_t)); // Dynamically allocated
        // list of child pids (in jobfile test order)
    int childrenReturned;
    int status;
    
    // Run all of the jobs
    jobsRun = 0;
    while (jobs[jobsRun]) {
        if ((childPid = fork()) == 0) {
            // printf("Hello from Child!\n");
            // printf("My parent's pid is: %d\n", getppid());   
            printf("Starting job %d\n", jobsRun + 1);
            fflush(stdout);
            int result = run_job(*jobs[jobsRun], testProgram);
            exit(result); // This transmits the result to the parent
    
        } else {
            // printf("Hello from Parent!\n");
            // Continue to keep spinning up children ...
        }

        childPids[jobsRun] = childPid;
        jobsRun++;
        childPids = (pid_t*) realloc(childPids, sizeof(pid_t) * jobsRun + 1);
        jobResults = (int*) realloc(jobResults, sizeof(int) * jobsRun + 1);
    }

    // Sleep for 2s
    sleep(2);

    // Wait on children to collect results
    childrenReturned = 0;
    while (childrenReturned != jobsRun) {
        wpid = wait(&status);

        // Record the return status in jobResults
        int jobIndex = 0; // the index of the job that this process ran
        while (true) {
            if (wpid == childPids[jobIndex]) {
                jobResults[jobIndex] = status;
                break;
            } else {
                jobIndex++;
                // If we get into an infinite loop in the program this is 
                // probably broken
            }
        }

        childrenReturned++;
    }

    return jobResults;
}

/**
 * Closes all the opened pipe file descriptors
*/
void close_pipes_except(int pipes[4][2], int exclude1, int exclude2) {
    // Close all the pipe file descriptors
    for (int pipeIndex = 0; pipeIndex < 4; pipeIndex++) {
        for (int endIndex = 0; endIndex < 2; endIndex++) {
            int fd = pipes[pipeIndex][endIndex];
            if (!(fd == exclude1 || fd == exclude2)) {
                // If not one of the excluded fds, close it
                close(fd);
            }
        }
    }
}

/**
 * Closes all the opened pipe file descriptors
*/
void close_pipes(int pipes[4][2]) {
    // Close all the pipe file descriptors
    for (int i = 0; i < 4; i++) {
        printf("Closed[%d, %d] ", pipes[i][WRITE_END], pipes[i][READ_END]);
        close(pipes[i][WRITE_END]);
        close(pipes[i][READ_END]);
    }
}

/**
 * Execs the test program
*/
void exec_test_wordiply(int pipes[4][2], char* inputFile, char* testProgram, 
        char** argv) {
    int inputFd;

    printf("TEST PROGRAM (%s) - pid:%d, ppid:%d\n", testProgram, getpid(), 
            getppid());
    fflush(stdout);

    argv[0] = testProgram;
    inputFd = open(inputFile, O_RDONLY);

    dup2(inputFd, STDIN_FILENO);
    dup2(pipes[PIPE_1][WRITE_END], STDOUT_FILENO);
    dup2(pipes[PIPE_2][WRITE_END], STDERR_FILENO);
    close_pipes(pipes);

    execvp(testProgram, argv);
    printf("Failed to exec testwordiply");
    exit(1);
}

/**
 * Execs the demo wordiply program
*/
void exec_demo_wordiply(int pipes[4][2], char* inputFile, char** argv) {
    int inputFd;

    printf("DEMO PROGRAM (%s) - pid:%d, ppid:%d\n", "demo-uqwordiply", 
            getpid(), getppid());
    fflush(stdout);

    argv[0] = "demo-uqwordiply";
    inputFd = open(inputFile, O_RDONLY);

    dup2(inputFd, STDIN_FILENO);
    dup2(pipes[PIPE_3][WRITE_END], STDOUT_FILENO);
    dup2(pipes[PIPE_4][WRITE_END], STDERR_FILENO);
    close_pipes(pipes);

    execvp("demo-uqwordiply", argv);
    printf("Failed to exec demowordiply");
    exit(1);
}

/**
 * Execs the stdout compaing uqcmp
*/
void exec_cmp_out(int pipes[4][2]) {
    printf("UQCMP_OUT - pid:%d, ppid:%d\n", getpid(), getppid());
    fflush(stdout);

    dup2(pipes[PIPE_1][READ_END], 3);
    dup2(pipes[PIPE_3][READ_END], 4);
    close_pipes_except(pipes, 3, 4);

    execlp("uqcmp", "uqcmp", "cmp_out", NULL);
    printf("Failed to exec cmp_out");
    exit(1);
}

/**
 * Execs the stderr compaing uqcmp
*/
void exec_cmp_err(int pipes[4][2]) {
    printf("UQCMP_ERR - pid:%d, ppid:%d\n", getpid(), getppid());
    fflush(stdout);

    dup2(pipes[PIPE_2][READ_END], 3);
    dup2(pipes[PIPE_4][READ_END], 4);
    close_pipes_except(pipes, 3, 4);

    execlp("uqcmp", "uqcmp", "cmp_err", NULL);
    printf("Failed to exec cmp_err");
    exit(1);
}

/**
 * Runs a single testing job as described by the given jobSpec and returns the 
 * exit value.
*/
int run_job(JobSpec job, char* testProgram) {
    pid_t childPids[4];
    // pid_t wpid;
    // int childrenReturned;
    // int status = 0;
    int pipes[4][2];
    int numTokens;
    char** argv;

    // Arrange our argv in the standard format
    argv = split_space_not_quote(job.argString, &numTokens);
    argv = (char**) realloc(argv, sizeof(char*) * numTokens + 2);
    memmove(argv + 1, argv, numTokens * sizeof(char*));
    argv[numTokens + 1] = NULL;

    /* --------------------------- CREATE PIPES --------------------------- */
    for (int i = 0; i < 4; i++) {
        if (pipe(pipes[i]) < 0) {
            // TODO: Fix and check if this error is reporting properly
            printf("Error creating pipes\n");
            exit(-1);
        }
    }

    /* ------------------------- CREATE PROCESSES ------------------------- */
    // Exec the required processes
    if ((childPids[TEST_WORDIPLY] = fork()) == 0) {
        exec_test_wordiply(pipes, job.inputFile, testProgram, argv);
    }

    if ((childPids[DEMO_WORDIPLY] = fork()) == 0) {
        exec_demo_wordiply(pipes, job.inputFile, argv);        
    }

    if ((childPids[UQCMP_OUT] = fork()) == 0) {
        exec_cmp_out(pipes);        
    }

    if ((childPids[UQCMP_ERR] = fork()) == 0) {
        exec_cmp_err(pipes);
    }

    exit(0);
    // TODO: Update this to return the correct value
    return -1;
}

/* ------------------------------ MAIN ------------------------------ */
int main(int argc, char* argv[]) {
    
    // Check and gather the input args
    ArgSet args = parse_args(argc, argv);

    // Null terminated array containing a spec for each job
    JobSpec** jobs = parse_jobfile(args.jobFile);

    int i = 0;
    while (jobs[i]) {
        printf("JOB %d - inputFile: <%s>, argString: <%s>\n", i + 1, 
                jobs[i]->inputFile, jobs[i]->argString);
        i++;
    }

    run_jobs(jobs, args.testProgram, args.parallel, args.quiet);

    return 0;
}
