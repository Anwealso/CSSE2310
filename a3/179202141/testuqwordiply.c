// CSSE2310 Assignment 3: UQ Wordiply Tester
// by Alex Nicholson
// 29/04/2023

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
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
#define ERR_EXEC_FAIL 99
#define STDOUT_DIFF 1000
#define STDERR_DIFF 100
#define EXIT_DIFF 10
#define EXEC_FAIL 1


/* ------------------------------ GLOBAL VARS ----------------------------- */
int interrupted = false;

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
void sigint_handler(int sigNum);
ArgSet parse_args(int argc, char* argv[]);
JobSpec** parse_jobfile(char* jobFile);
JobSpec* parse_jobfile_line(char* line);
void report_result(int jobNumber, int resultCode);
void sleep_resume(int requestedInt);
int run_sequential(JobSpec** jobs, char* testProgram, bool quiet);
void await_job_results(pid_t* childPids, int* jobResults, int jobsRun);
int run_parallel(JobSpec** jobs, char* testProgram, bool quiet);
void close_pipes_except(int pipes[4][2], int exclude1, int exclude2);
void close_pipes(int pipes[4][2]);
void exec_test_wordiply(int pipes[4][2], char* inputFile, char* testProgram, 
        char** argv);
void exec_demo_wordiply(int pipes[4][2], char* inputFile, char** argv);
void exec_cmp_out(int pipes[4][2], int jobNumber, bool quiet);
void exec_cmp_err(int pipes[4][2], int jobNumber, bool quiet);
bool exec_job_children(int jobNumber, pid_t childPids[4], int pipes[4][2], 
        char* inputFile, char* testProgram, char** argv, bool quiet);
int await_job_child_results(int jobNumber, pid_t childPids[4], 
        int pipes[4][2], char* inputFile, char* testProgram, char** argv, 
        bool quiet);
int run_job(JobSpec job, char* testProgram, int jobNumber, bool quiet);
void free_jobs_array(JobSpec** jobs);

/* ------------------------------ DEFINITIONS ----------------------------- */
/**
 * Handles SIGINT signal
*/
void sigint_handler(int sigNum) {
    if (sigNum == 2) {
        // If SIGINT received
        interrupted = true;
    }
}

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
 * Reports the result of the test with an informative print out.
*/
void report_result(int jobNumber, int resultCode) {    
    if (((int)(resultCode / EXEC_FAIL)) % 10) {
        // Error execing job processes
        printf("Job %d: Unable to execute test\n", jobNumber);
        fflush(stdout);

    } else {
        if (((int)(resultCode / STDOUT_DIFF)) % 10) {
            printf("Job %d: Stdout differs\n", jobNumber);
            fflush(stdout);
        } else {
            printf("Job %d: Stdout matches\n", jobNumber);
            fflush(stdout);
        }
            
        if (((int)(resultCode / STDERR_DIFF)) % 10) {
            printf("Job %d: Stderr differs\n", jobNumber);
            fflush(stdout);
        } else {
            printf("Job %d: Stderr matches\n", jobNumber);
            fflush(stdout);
        }

        if (((int)(resultCode / EXIT_DIFF)) % 10) {
            printf("Job %d: Exit status differs\n", jobNumber);
            fflush(stdout);
        } else {
            printf("Job %d: Exit status matches\n", jobNumber);
            fflush(stdout);
        }

    }
}

/**
 * Sleeps for the given time requested (in seconds), resuming after being 
 * interrupted if needed.
*/
void sleep_resume(int requestedInt) {
    struct timespec requested = {.tv_sec = requestedInt, .tv_nsec = 0};
    struct timespec remaining;

    while (nanosleep(&requested, &remaining) == -1) {
        requested = remaining;
    }
}

/**
 * Runs the list of given jobspecs using a sequential strategy and returns the 
 * results.
*/
int run_sequential(JobSpec** jobs, char* testProgram, bool quiet) {
    int jobsRun = 0;
    int testsPassed = 0;
    int result = 0;

    while (jobs[jobsRun] && !interrupted) {
        printf("Starting job %d\n", jobsRun + 1);
        fflush(stdout);

        // Run the job
        result = run_job(*jobs[jobsRun], testProgram, jobsRun + 1, quiet);
        
        // Sleep for 2s
        sleep_resume(2);

        // TODO: Add SIGKILL sending so it kills all 4 children for each job

        // Report the result of the job
        report_result(jobsRun + 1, result);
        if (result == 0) {
            testsPassed++;
        }

        jobsRun++;
    }

    // TODO: Check that this works with test interruption enabled
    printf("testuqwordiply: %d out of %d tests passed\n", testsPassed, 
            jobsRun);
    fflush(stdin);

    // Return 0 if all the tests passed, else return 1
    if (testsPassed == jobsRun) {
        return 0;
    } else {
        return 1;
    }
}

/**
 * Waits for and collects all child job results into jobResults
*/
void await_job_results(pid_t* childPids, int* jobResults, int jobsRun) {
    int childrenReturned = 0;
    pid_t wpid;
    int status;
    int jobIndex; // the current jobIndex we are checking for pid match

    // TODO: Check that this works with test interruption enabled
    while (childrenReturned < jobsRun) {
        wpid = wait(&status);

        // Search to find the matching jobIndex for this child and save its
        // status in jobResults
        jobIndex = 0;
        while (true) {
            if (wpid == childPids[jobIndex]) {
                jobResults[jobIndex] = WEXITSTATUS(status);
                break;
            } else {
                jobIndex++;
                // If we get into an infinite loop in the program this is 
                // probably broken
            }
        }
        childrenReturned++;
    }
}

/**
 * Execs jobs for parallel execution
*/
int exec_jobs(JobSpec** jobs, char* testProgram, pid_t* childPids, 
        int* jobResults, bool quiet) {
    int jobsRun = 0;
    int result;
    pid_t childPid;

    while (jobs[jobsRun] && !interrupted) {
        printf("Starting job %d\n", jobsRun + 1);
        fflush(stdout);

        if ((childPid = fork()) == 0) {
            result = run_job(*jobs[jobsRun], testProgram, jobsRun + 1, 
                    quiet);
            exit(result); // This transmits the result to the parent
        }

        childPids[jobsRun] = childPid;
        jobsRun++;
        childPids = (pid_t*) realloc(childPids, sizeof(pid_t) * (jobsRun + 1));
        jobResults = (int*) realloc(jobResults, sizeof(int) * (jobsRun + 1));
    }

    return jobsRun;
}

/**
 * Runs the list of given jobspecs using a parallel strategy and returns the 
 * results.
*/
int run_parallel(JobSpec** jobs, char* testProgram, bool quiet) {
    int jobsRun;
    int* jobResults = (int*) malloc(sizeof(int));
    pid_t* childPids = (pid_t*) malloc(sizeof(pid_t));
    int testsPassed = 0;

    // Run all of the jobs
    jobsRun = exec_jobs(jobs, testProgram, childPids, jobResults, quiet);

    // Sleep for 2s
    sleep_resume(2);
    
    // TODO: Fix this so it kills all 4 children for each job
    // Send sigkill to all child processes
    for (int jobIndex = 0; jobIndex < jobsRun; jobIndex++) {
        // Send SIGKILL to the child
        kill(childPids[jobIndex], SIGKILL);
    }

    // Wait on children to collect results
    await_job_results(childPids, jobResults, jobsRun);

    // Report all of the job results in order
    for (int jobIndex = 0; jobIndex < jobsRun; jobIndex++) {
        // Report the result of the job
        report_result(jobIndex + 1, jobResults[jobIndex]);
        if (jobResults[jobIndex] == 0) {
            testsPassed++;
        }
    }

    // Free our child results and pids now we're done with them
    free(jobResults);
    free(childPids);

    // TODO: Check that this works with test interruption enabled
    printf("testuqwordiply: %d out of %d tests passed\n", testsPassed, 
            jobsRun);
    fflush(stdin);

    // Return 0 if all the tests passed, else return 1
    if (testsPassed == jobsRun) {
        return 0;
    } else {
        return 1;
    }
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

    argv[0] = testProgram;
    inputFd = open(inputFile, O_RDONLY);

    dup2(inputFd, STDIN_FILENO);
    dup2(pipes[PIPE_1][WRITE_END], STDOUT_FILENO);
    dup2(pipes[PIPE_2][WRITE_END], STDERR_FILENO);
    close(inputFd);
    close_pipes(pipes);

    execvp(testProgram, argv);
}

/**
 * Execs the demo wordiply program
*/
void exec_demo_wordiply(int pipes[4][2], char* inputFile, char** argv) {
    int inputFd;

    argv[0] = "demo-uqwordiply";
    inputFd = open(inputFile, O_RDONLY);

    dup2(inputFd, STDIN_FILENO);
    dup2(pipes[PIPE_3][WRITE_END], STDOUT_FILENO);
    dup2(pipes[PIPE_4][WRITE_END], STDERR_FILENO);
    close(inputFd);
    close_pipes(pipes);

    execvp("demo-uqwordiply", argv);
}

/**
 * Execs the stdout compaing uqcmp
*/
void exec_cmp_out(int pipes[4][2], int jobNumber, bool quiet) {
    char outStr[30]; // header string to print to console - max string length
    // should be 23 (incl '\0'), since the int_max = 2147483647 (10 digits)
    int fdNull; // fd to /dev/null

    dup2(pipes[PIPE_1][READ_END], 3);
    dup2(pipes[PIPE_3][READ_END], 4);
    close_pipes_except(pipes, 3, 4);

    if (quiet) {
        // TODO: Check if this should be RD or WR only (we desire to read EOF)
        fdNull = open("/dev/null", O_WRONLY);
        dup2(fdNull, STDOUT_FILENO);
        dup2(fdNull, STDERR_FILENO);
        close(fdNull);
    }

    sprintf(outStr, "Job %d stdout", jobNumber);
    execlp("uqcmp", "uqcmp", outStr, (char*) NULL);
}

/**
 * Execs the stderr compaing uqcmp
*/
void exec_cmp_err(int pipes[4][2], int jobNumber, bool quiet) {
    char outStr[30]; // header string to print to console - max string length
    // should be 23 (incl '\0'), since the int_max = 2147483647 (10 digits)
    int fdNull; // fd to /dev/null

    dup2(pipes[PIPE_2][READ_END], 3);
    dup2(pipes[PIPE_4][READ_END], 4);
    close_pipes_except(pipes, 3, 4);

    if (quiet) {
        // TODO: Check if this should be RD or WR only (we desire to read EOF)
        fdNull = open("/dev/null", O_WRONLY);
        dup2(fdNull, 1);
        dup2(fdNull, 2);
        close(fdNull);
    }

    sprintf(outStr, "Job %d stderr", jobNumber);
    execlp("uqcmp", "uqcmp", outStr, (char*) NULL);
}

/**
 * Execs the child processes for a job
*/
bool exec_job_children(int jobNumber, pid_t childPids[4], int pipes[4][2], 
        char* inputFile, char* testProgram, char** argv, bool quiet) {
    // Exec the required processes
    if ((childPids[TEST_WORDIPLY] = fork()) == 0) {
        exec_test_wordiply(pipes, inputFile, testProgram, argv);
        exit(ERR_EXEC_FAIL);
    }
    if ((childPids[DEMO_WORDIPLY] = fork()) == 0) {
        exec_demo_wordiply(pipes, inputFile, argv);
        exit(ERR_EXEC_FAIL);
    }
    if ((childPids[UQCMP_OUT] = fork()) == 0) {
        exec_cmp_out(pipes, jobNumber, quiet);
        exit(ERR_EXEC_FAIL);
    }
    if ((childPids[UQCMP_ERR] = fork()) == 0) {
        exec_cmp_err(pipes, jobNumber, quiet);
        exit(ERR_EXEC_FAIL);
    }
    if ((childPids[TEST_WORDIPLY] < 0) || (childPids[DEMO_WORDIPLY] < 0) || 
            (childPids[UQCMP_OUT] < 0) || (childPids[UQCMP_ERR] < 0)) {
        return true; // In parent but a fork failed, so return
    }
    return false;
}

/**
 * Execs the child processes for a job
*/
int await_job_child_results(int jobNumber, pid_t childPids[4], 
        int pipes[4][2], char* inputFile, char* testProgram, char** argv, 
        bool quiet) {
    int childrenReturned = 0;
    int status; // child's status
    bool exitedNormally; // if child exited normally
    pid_t wpid;
    int exitCode; // child exit code
    int testProgramStatus = 0; // exit status of the test program
    int demoProgramStatus = 0; // exit status of the demo program
    bool compareStatus = false; // if statuses should be compared this round
    int returnStatus = 0; // what status this function will return

    while (childrenReturned < 4) {
        wpid = wait(&status);
        exitedNormally = WIFEXITED(status);
        exitCode = WEXITSTATUS(status);

        if (exitCode == ERR_EXEC_FAIL) { // Child failed to exec program
            return EXEC_FAIL;
        }

        if (wpid == childPids[TEST_WORDIPLY]) {
            testProgramStatus = exitCode;
            if (!exitedNormally || (compareStatus && 
                    (testProgramStatus != demoProgramStatus))) {
                returnStatus = returnStatus + EXIT_DIFF;
            }
            compareStatus = true;
        } else if (wpid == childPids[DEMO_WORDIPLY]) {
            demoProgramStatus = exitCode;
            if (!exitedNormally || (compareStatus && 
                    (testProgramStatus != demoProgramStatus))) {
                returnStatus = returnStatus + EXIT_DIFF;
            }
            compareStatus = true;
        } else if (wpid == childPids[UQCMP_OUT]) {
            if (exitCode != 0) {
                returnStatus = returnStatus + STDOUT_DIFF;
            }
        } else if (wpid == childPids[UQCMP_ERR]) {
            if (exitCode != 0) {
                returnStatus = returnStatus + STDERR_DIFF;
            }
        }
        childrenReturned++;
    }

    return returnStatus;
}

/**
 * Runs a single testing job as described by the given jobSpec and returns the 
 * exit value.
*/
int run_job(JobSpec job, char* testProgram, int jobNumber, bool quiet) {
    pid_t childPids[4];
    int pipes[4][2];
    int numTokens; // num args in argv list
    char** argv;
    int returnStatus = 0; // what status this function will return

    // Arrange our argv in the standard format
    argv = split_space_not_quote(job.argString, &numTokens);
    argv = (char**) realloc(argv, sizeof(char*) * numTokens + 2);
    memmove(argv + 1, argv, numTokens * sizeof(char*));
    argv[numTokens + 1] = NULL;

    // Create pipes
    for (int i = 0; i < 4; i++) {
        if (pipe(pipes[i]) < 0) {
            return EXEC_FAIL;
        }
    }

    // Create processes
    bool exec_err = exec_job_children(jobNumber, childPids, pipes, 
            job.inputFile, testProgram, argv, quiet);
    if (exec_err) {
        return EXEC_FAIL;
    }

    // Close any remaining pipe openings in parent
    close_pipes(pipes);

    // Collect the results from the cmp stdout and stdin processes
    returnStatus = await_job_child_results(jobNumber, childPids, pipes, 
            job.inputFile, testProgram, argv, quiet);

    return returnStatus;
}

/**
 * Frees up the allocated memory for the jobs list and each of its contained 
 * job pointers.
*/
void free_jobs_array(JobSpec** jobs) {
    int jobsIndex = 0;
    
    // Free each of the contained Job pointers
    while (jobs[jobsIndex]) {
        free(jobs[jobsIndex]);
        jobsIndex++;
    }
    // Free the list pointer itself
    free(jobs);
}

/**
 * Main function - runs the uqwordiply tester.
*/
int main(int argc, char* argv[]) {
    int result; // 0 is all the tests passed, 1 otherwise
    signal(SIGINT, sigint_handler); // Register signal handler for sigint
    
    // Check and gather the input args
    ArgSet args = parse_args(argc, argv);

    // Null terminated array containing a spec for each job
    JobSpec** jobs = parse_jobfile(args.jobFile);

    // Run the jobs in the desired mode
    if (args.parallel) {
        result = run_parallel(jobs, args.testProgram, args.quiet);
    } else {
        result = run_sequential(jobs, args.testProgram, args.quiet);
    }

    // Free all of our major data structures
    free_jobs_array(jobs);

    return result;
}

