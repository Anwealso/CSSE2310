// CSSE2310 Assignment 1: UQ Wordiply Game
// by Alex Nicholson
// 04/03/2023

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <csse2310a1.h>

void check_args(int argc, char* argv[]);
char* get_starter_word(int argc, char* argv[]);
bool is_valid_word(char* string);
void to_upper(char* string);
char** get_dictionary(int argc, char* argv[]);
void print_welcome(const char* starterWord);
char* get_guess(int guessNum);
bool is_valid_guess(char* guess, char* starterWord, char** dictionary, 
        char** guesses);
int get_total_length(char** guesses);
char** get_longest_words(char** guesses);
char** get_longest_possible(char* starterWord, char** dictionary);
void print_results(char** guesses, char* starterWord, char** dictionary);

/* ------------------------------ DEFINITIONS ------------------------------ */
/**
 * Checks the command line args to see that they're not completely broken 
 * or invalid. Loads the command line argument values (or NULL if no value 
 * provided) into the given char pointers
 * 
 * Errors cases:
 *     - Wrong number of args (argc must be odd and >5)
 *     - Invalid args format (must have --flag followed by [value], 
 *          repeat...)
 *     - Invalid flag names (not one of: "--start", "--len", or 
 *          "--dictionary")
 *     - Invalid mix of flags (can only use --start OR --len flags)
 *     - Double usage of any flag
 */
void check_args(int argc, char* argv[]) {
    bool argsValid = true; // whether the command line args are valid
    char* prevFlag = NULL; // the previous flag
    char* flag; // the current flag we are checking

    if ((argc != 1) && (argc != 3) && (argc != 5)) {
        // Wrong number of args
        argsValid = false;
    }

    // Check if the flags are valid (flags are in the 1 and 3 positions)
    for (int i = 1; i < argc; i += 2) {
        flag = argv[i];

        if ((strcmp(flag, "--start") != 0) && (strcmp(flag, "--len") != 0) && 
                (strcmp(flag, "--dictionary") != 0)) {
            // Invalid flag name or invalid args format
            argsValid = false;
            break;
        }

        if (prevFlag) {
            if (strcmp(flag, prevFlag) == 0) {
                // Double use of flag
                argsValid = false;
                break;
            } else if ((strcmp(flag, "--start") == 0) && 
                    (strcmp(prevFlag, "--len") == 0)) {
                // Invalid mix of flags (can only use --start OR --len flags)
                argsValid = false;
                break;
            } else if ((strcmp(flag, "--len") == 0) && 
                    (strcmp(prevFlag, "--start") == 0)) {
                // Invalid mix of flags (can only use --start OR --len flags)
                argsValid = false;
                break;
            }
        }
        prevFlag = flag;
    }

    if (argsValid == false) {
        fprintf(stderr, "Usage: uqwordiply [--start starter-word | --len"
                " length] [--dictionary filename]\n");
        exit(1);
    }
}

/**
 * Gets the starter word and checks it.
 */
char* get_starter_word(int argc, char* argv[]) {
    char* starterWord = NULL;
    int starterLen;

    // Find the --start or --len flag (note: flags are in the 1 and 3 
    // positions)
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "--start") == 0) {
            starterWord = strdup(argv[i + 1]);
            starterWord[strcspn(starterWord, "\n")] = 0; // remove newline char

            // Check starter word is valid
            if (!(is_valid_word(starterWord)) || ((strlen(starterWord) != 3) 
                    && (strlen(starterWord) != 4))) {
                // If not a valid word (only letters), or not the right length,
                // i.e. len (3 or 4)
                fprintf(stderr, "uqwordiply: invalid starter word\n");
                exit(2);
            }

            // Convert to upper
            to_upper(starterWord);

        } else if (strcmp(argv[i], "--len") == 0) {
            starterLen = atoi(argv[i + 1]);
            if ((starterLen == 3) || (starterLen == 4)) {
                starterWord = strdup(get_wordiply_starter_word(starterLen));
            } else {
                // If len argument is invalid (not 3 or 4)
                fprintf(stderr, "Usage: uqwordiply [--start starter-word | "
                        "--len length] [--dictionary filename]\n");
                exit(1);
            }
        }
    }

    // If the starter word is still NULL, then use get_wordiply_starter_word(0)
    if (!starterWord) {
        starterWord = strdup(get_wordiply_starter_word(0));
    }

    return starterWord;
}

/**
 * Checks whether the string is a valid word (noly letter chars)
 */
bool is_valid_word(char* string) {
    for (int i = 0; i < strlen(string); i++) {
        if (string[i] < 65 || string[i] > 122) {
            // Char is not an upper or lower case letter
            return false;
        }
    }

    return true;
}

/**
 * Converts all letters in a string to upper case (ignores non-letter 
 * chars)
 */
void to_upper(char* string) {
    for (int i = 0; i < strlen(string); i++) {
        if (string[i] >= 97 && string[i] <= 122) {
            // Char is a lower case letter, so convert to upper
            string[i] = string[i] - 32;
        }
    }
}

/**
 * Gets the dict and checks it.
 */
char** get_dictionary(int argc, char* argv[]) {
    FILE* dictFile;
    int maxWordLen = 50;
    char lineBuffer[maxWordLen + 2];
    char* dictFilename = NULL;
    char** dictionary = (char**) malloc(sizeof(char*) * 1);
    int numItems = 0; // the current number of items in our dict array

    // Find the --dictionary flag (note: flags are in the 1 and 3 positions)
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "--dictionary") == 0) {
            dictFilename = argv[i + 1];
            break;
        }
    }
    // If no dict file specified, then use get_wordiply_starter_word(0)
    if (!dictFilename) {
        dictFilename = "/usr/share/dict/words";
    }

    // Open the dictionary file for reading
    dictFile = fopen(dictFilename, "rb");
    // If the file couldn't be opened, return an error
    if (!dictFile) {
        fprintf(stderr, "uqwordiply: dictionary file \"%s\" cannot be "
                "opened\n", dictFilename);
        exit(3);
    }

    // Now load in all the words from the dictionary
    while (fgets(lineBuffer, maxWordLen + 2, dictFile)) {
        // If string has a \n, convert it to a \0  (NULL char)
        lineBuffer[strcspn(lineBuffer, "\n")] = 0;

        // Check for EOF
        if (strlen(lineBuffer) == 0) {
            break;
        }

        // Convert to upper case
        to_upper(lineBuffer);

        // Check if dict word has any illegal characters
        bool legal = true;
        for (int i = 0; i < strlen(lineBuffer); i++) {
            if (lineBuffer[i] < 65 || lineBuffer[i] > 90) {
                // Char is a lower case letter, so convert to upper
                legal = false;
                break;
            }
        }

        if (legal == true) {
            dictionary[numItems] = strdup(lineBuffer);
            numItems++;
            // Expand the array (to prep space for the future next elem)
            dictionary = (char**) realloc(dictionary, (numItems + 1) * 
                    sizeof(char*));
            // Add a null pointer as the last elem in the list
            dictionary[numItems] = NULL;
        }
    }

    return dictionary;
}

/**
 * Prints the welcome message.
 */
void print_welcome(const char* starterWord) {
    printf("Welcome to UQWordiply!\n");
    printf("The starter word is: %s\n", starterWord);
    printf("Enter words containing this word.\n");
}

/**
 * Gets the users guess from stdin
 */
char* get_guess(int guessNum) {
    char* buffer = NULL;
    int size_read;
    size_t len;

    // Prompt the user for a guess
    printf("Enter guess %d:\n", guessNum);

    // Read in the user input
    size_read = getline( & buffer, & len, stdin);

    // Check if stdin has been closed
    if (size_read == -1) {
        return NULL;
    }

    // Get rid of newline char
    buffer[strcspn(buffer, "\n")] = 0;

    return buffer;
}

/**
 * Checks that the guess contains the starter word and is a valid 
 * dictionary word (also should check word is not an EOF). Also prints out 
 * the reason that the choice is invalid in a "try
 * again" message.
 * 
 * Error cases:
 *  - Guesses must contain only letters - try again.
 *  - Guesses must contain the starter word - try again.
 *  - Guesses can’t be the starter word - try again.
 *  - Guess not found in dictionary - try again.
 *  - You’ve already guessed that word - try again.
 */
bool is_valid_guess(char* guess, char* starterWord, char** dictionary, 
        char** guesses) {
    bool matched; // whether the guess matches a dict entry
    int dict_index;
    bool duplicate; // whether the guess is a duplicate of a previous guess
    int guesses_index;

    if (strlen(guess) == 0) {
        // If user input empty
        printf("Guesses must contain the starter word - try again.\n");
        return false;
    } else if (!is_valid_word(guess)) {
        printf("Guesses must contain only letters - try again.\n");
        return false;
    }

    // Convert the guess to all uppercase
    to_upper(guess);

    if (!strstr(guess, starterWord)) {
        printf("Guesses must contain the starter word - try again.\n");
        return false;
    } else if (strcmp(guess, starterWord) == 0) {
        printf("Guesses can't be the starter word - try again.\n");
        return false;
    }

    matched = false;
    dict_index = 0;
    while (dictionary[dict_index] != NULL) {
        // Check the guess against each item in the dict to see if it matches
        if (strcmp(guess, dictionary[dict_index]) == 0) {
            matched = true;
            break;
        }
        dict_index++;
    }
    // If no dict matches, return false
    if (matched == false) {
        printf("Guess not found in dictionary - try again.\n");
        return false;
    }

    guesses_index = 0;
    duplicate = false;
    while (guesses[guesses_index] != NULL) {
        // Check the guess against each item in the dict to see if it matches
        if (strcmp(guess, guesses[guesses_index]) == 0) {
            duplicate = true;
            break;
        }
        guesses_index++;
    }
    // If no dict matches, return false
    if (duplicate == true) {
        printf("You've already guessed that word - try again.\n");
        return false;
    }

    return true;
}

/**
 * Get the sum of the lengths of all of the guessed words
 */
int get_total_length(char** guesses) {
    int total_length = 0;
    int i = 0;

    while (guesses[i] != NULL) {
        total_length = total_length + strlen(guesses[i]);
        i++;
    }

    return total_length;
}

/**
 * Get the longest words out of those guessed
 */
char** get_longest_words(char** guesses) {
    char** longest_words = (char**) malloc(sizeof(char*) * 5);
    int longest_length = 0;
    int num_longest = 0; // number of words currently tied for longest
    int i = 0;

    // Check through all of the guesses to find the longest ones
    while (guesses[i] != NULL) {
        if (strlen(guesses[i]) > longest_length) {
            // Clear the longest_words array
            free(longest_words);
            longest_words = (char**) malloc(sizeof(char*) * 5);
            num_longest = 0;
            longest_length = strlen(guesses[i]);
        }

        if (strlen(guesses[i]) >= longest_length) {
            // Add this guess to the longest_words array
            longest_words[num_longest] = guesses[i];
            num_longest++;
        }
        i++;
    }

    if (num_longest < 5) {
        // Put a NULL after the last item as an end marker if we didn't fill 
        // the array
        longest_words[num_longest] = NULL;
    }

    return longest_words;
}

/**
 * Get the longest possible word in the dictionary containing the starter word
 */
char** get_longest_possible(char* starterWord, char** dictionary) {
    char** longest_possible = (char**) malloc(sizeof(char*) * 1);
    int longest_length = 0;
    int num_longest = 0; // number of words currently tied for longest
    int i = 0;

    // Check through all of the guesses to find the longest ones
    while (dictionary[i]) {
        // Check if this dictionary word contains the starter word
        if (strstr(dictionary[i], starterWord)) {

            if (strlen(dictionary[i]) > longest_length) {
                // Clear the longest_possible array
                free(longest_possible);
                num_longest = 0;
                longest_possible = (char**) malloc(sizeof(char*) * 1);
                longest_length = strlen(dictionary[i]);
            }

            if (strlen(dictionary[i]) >= longest_length) {
                // Add this guess to the longest_words array
                longest_possible[num_longest] = dictionary[i];
                num_longest++;

                // Expand the array
                longest_possible = (char**) realloc(longest_possible, 
                        sizeof(char*) * (num_longest + 1));
                // NULL terminate the list
                longest_possible[num_longest] = NULL;
            }
        }
        i++;
    }

    return longest_possible;
}

/**
 * Prints the summary results data at the end of the game.
 */
void print_results(char** guesses, char* starterWord, char** dictionary) {
    int total_length = get_total_length(guesses);
    char** longest_words = get_longest_words(guesses);
    char** longest_possible = get_longest_possible(starterWord, dictionary);
    int i;

    // Print a blank line
    printf("\n");

    // Print all of this info to the console...
    printf("Total length of words found: %d\n", total_length);

    // TODO: fix memory overwriting of longest_words
    printf("Longest word(s) found:\n");
    i = 0;
    while ((longest_words[i] != NULL) && (i < 5)) {
        printf("%s (%ld)\n", longest_words[i], strlen(longest_words[i]));
        i++;
    }
    free(longest_words);

    printf("Longest word(s) possible:\n");
    i = 0;
    while (longest_possible[i] != NULL) {
        printf("%s (%ld)\n", longest_possible[i], strlen(longest_possible[i]));
        i++;
    }
    free(longest_possible);
}

/* ------------------------------ MAIN ------------------------------ */
int main(int argc, char* argv[]) {
    char* starterWord;
    char** dictionary;
    int guesses_used;
    char* guess;
    char* guesses[5];

    // Check command line args
    check_args(argc, argv);

    // Get game inputs
    starterWord = get_starter_word(argc, argv);
    dictionary = get_dictionary(argc, argv);

    // Print the welcome message
    print_welcome(starterWord);

    // Play game loop
    memset(guesses, '\0', sizeof(guesses)); // initialise guesses as NULL
    guesses_used = 0;
    while (guesses_used < 5) {
        do {
            guess = get_guess(guesses_used + 1);
            if (guess == NULL) break; // stdin closed, so move to end screen
        } while (!is_valid_guess(guess, starterWord, dictionary, guesses));

        if (guess == NULL) {
            break; // stdin has been closed, so move to the end screen
        } else {
            // Now we have a valid guess, so add it to the guesses array
            guesses[guesses_used] = guess;
            guesses_used++;
        }
    }

    // If no valid guesses entered
    if (!guesses[0]) {
        exit(4);
    }

    // Now the user has either used up all of their guesses or sent 
    // an EOF, so end the game and read their results
    print_results(guesses, starterWord, dictionary);

    // Free any remaining allocated memory
    free(starterWord);
    free(dictionary);

    return 0;
}