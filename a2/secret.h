/*
** secret.h
**      Stuff that the bomb squad isn't supposed to see.
**
**      Ooops...
*/

#ifndef SECRET_H
#define SECRET_H

#include <stdbool.h>

#define MAX_PHASES 10
#define MAX_LINE 80

// Warning - don't tamper with these or you may set off the booby trap.
//
// Current phase being defused - 0 to 9 (10 and 11 are the two demo phases)
extern int currentPhase;

bool initialise_bomb(); // Initialise the bomb - returns true if all done
int  get_phase(void);   // Ask the user which phase to solve and return it
bool is_bomb_solved(void);      // Returns true if whole bomb is solved
bool is_phase_solved(int phase); // Returns whether the given phase is solved 
char* read_line(void);  // Reads line of text from stdin - skips over
// blank lines
char next_rchar(void);  // Return next char in a random sequence of chars

/*
** Routines for managing the secret defusing strings...
*/

/* reset_secret_string()
 *      Resets the secret string to "" - the empty string. Any elements
 *      previously appended to the string are discarded.
 */
void reset_secret_string(void);

/* mute()
 *      Indicates whether calls to append_to_secret_string() are
 *      ignored (muted) or not. If the flag argument is true, then calls
 *      to append_to_secret_string() do not result in any changes to the
 *      secret string. If false, then calls to append_to_secret_string()
 *      DO result in changes. The initial state is unmuted.
 *
 * muteflip()
 *      Reverses the current mute state.
 */
void mute(int flag);
void muteflip(void);

/* append_to_secret_string()
 *      Appends the given value to the secret string. Handles various
 *      types of arguments - char, int or string (char*) depending on the
 *      current phase. If the secret string is not empty, we append a space
 *      to the current contents before appending the given value.
 *      Character arguments are treated like the %c argument to printf.
 *      Integer arguments are treated like the %d argument to printf.
 *      String arguments are treated like the %s argument to printf.
 */
void append_to_secret_string(int phase, ...);

/* secret_string_matches()
 *      Checks if the given string matches the secret string for the current
 *      phase. Returns if it matches. Bomb explodes if it doesn't and
 *      the function does not return.
 */
void secret_string_matches(char*);

#endif
