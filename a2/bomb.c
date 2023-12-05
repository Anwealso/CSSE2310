/***************************************************************************
 * Dr. Evil's Insidious Bomb, Version 2.310
 * Copyright 2021, Dr. Evil Incorporated. All rights reserved.
 *
 * LICENSE:
 *
 * Dr. Evil Incorporated (the PERPETRATOR) hereby grants you (the
 * VICTIM) explicit permission to use this bomb (the BOMB).  This is a
 * time limited license, which expires on the death of the VICTIM.
 * The PERPETRATOR takes no responsibility for damage, frustration,
 * insanity, bug-eyes, carpal-tunnel syndrome, loss of sleep, or other
 * harm to the VICTIM.  Unless the PERPETRATOR wants to take credit,
 * that is.  The VICTIM may not distribute this bomb source code to
 * any enemies of the PERPETRATOR.  No VICTIM may debug,
 * reverse-engineer, run "strings" on, decompile, decrypt, or use any
 * other technique to gain knowledge of and defuse the BOMB.  BOMB
 * proof clothing may not be worn when handling this program.  The
 * PERPETRATOR will not apologize for the PERPETRATOR's poor sense of
 * humor.  This license is null and void where the BOMB is prohibited
 * by law.
 ***************************************************************************/

#include <stdio.h>
#include "secret.h"
#include "phases.h"
#include <stdbool.h>

 /*
  * Note to self: Remember to erase this file so my victims will have no
  * idea what is going on, and so they will all blow up in a
  * spectacularly fiendish explosion. -- Dr. Evil
  */

void (*phases[MAX_PHASES])(char*) = {
        phase_zero, phase_one, phase_two, phase_three, phase_four,
        phase_five, phase_six, phase_seven, phase_eight, phase_nine };
void (*demoPhases[])(char*) = { demo_one, demo_two };

int main(int argc, char* argv[]) {
    char* input;

    /* Note to self: remember to port this bomb to Windows and put a
     * fantastic GUI on it. */

    printf("\nWelcome to my fiendish little bomb. You have 10 phases with\n");
    printf("which to blow yourself up. Have a nice day!\n\n");

    /* Do some secret stuff that makes the bomb harder to defuse. */
    bool isBombSolved = initialise_bomb();

    while (!isBombSolved) {
        printf("Enter one of\n");
        printf("  - the phase number to solve (0 to 9, must be unsolved),\n");
        printf("  - demo1 or demo2 to enter demo phase 1 or 2, or\n");
        printf("  - status - to show the current status of all phases\n");
        printf("  - quit - to quit the bomb\n");
        currentPhase = get_phase();
        if (!is_phase_solved(currentPhase)) {
            input = read_line();
            switch (currentPhase) {
            case 10: /* Demo phase 1 */
            case 11: /* Demo phase 2 */
                demoPhases[currentPhase - 10](input);
                break;
            default:
                phases[currentPhase](input);
                break;
            }
        }
        else if (currentPhase >= 0 && currentPhase <= 9) {
            printf("Phase %d has been solved already\n", currentPhase);
        }
        isBombSolved = is_bomb_solved();
    }
    printf("\nCongratulations - you succeeded in defusing all the phases.\n");
    return 0;
}