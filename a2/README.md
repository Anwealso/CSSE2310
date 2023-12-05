
# CSSE2310 Bomb Assignment Planning

## Useful GDB Commands

- Set breakpoint at offset from function start: 
    `b *(&error+184)`, or
    `b *(error+184)`

- View assembly lines around current:
    `layout asm`



## Stage Progress

- [ ] 1
- [ ] 2
- [ ] 3
- [ ] 4
- [ ] 5
- [ ] 6
- [ ] 7
- [ ] 8
- [ ] 9
- [ ] 10

## Notes

`secret_string_matches` <- This is the funciton that checks the strings and explodes the bomb. DO NOT RUN THIS LINE IN THE DEBUGGER UNLESS SURE OF SUBMITTED STRING

### Secret.h Notable Features
- Function to get next char from a random sequence of chars
- Function to append a value (char, int, string) to the current secret string (separates additions by a space from the existing string)
- Mute() and MuteFlip() to mute/unmute the appending of additional strings to the current secret string
- Resetting the secret string to an empty string

### Phases.c Notable Features

Data structs:

- #define PHASE_SIX_CONST 3
- #define DEMO_ONE 10
- #define DEMO_TWO 11

- struct FortuneStruct - a linked list strucvture with a 'data' string and 'len' int prop for each node
- FortuneStruct* fortuneList - initially set to 0 but gets initialised as phase_seven_start() in phase_seven

- fortunes - null terminated array of 119 strings containing 'fortune quotes / sentences' - most with authors listed separated by a double dash (`--`)
- words - null terminated array of 103 'word' strings - NOTE: ALL WORDS OF 14 LETTERS LONG




- int t1[13] = { 3, 6, 9, 0, 2, 4, 8, 1, 5, 7, 1, 2, 3 };
    - jumps 3, 3, 3, _reset_ (-9), 2, 2, 2, _reset_ (-7), 4, 2, _reset_ (-6), 1, 1, 1
int t2[13] = { 9, 6, 3, 8, 4, 2, 0, 7, 5, 1, 9, 8, 7 };
int t3[13] = { 2, 3, 5, 7, 1, 4, 6, 8, 9, 0, 4, 6, 5 };

3,9,6
0,2,4,8
7,5,1





123
987
456




_____ Functions _____

Demo:
- int D1(void) - funciton for demo 1
- int D2(void) - funciton for demo 2

General:
- int new_number(void) - weirdly declared but uninitialised function
- void stir(void) - another weirdly declared but uninitialised function

Phase specific functions:
- int phase_zero_num(void);
- int phase_one_base(void);
- int phase_two_offset(void);
- int phase_three_num(void);
- int phase_six_depth(int);
- struct FortuneStruct* phase_seven_start(void);
- int phase_eight_num(void);
- int phase_nine_num(void);


## Bomb Flow

1. Setup array of function pointers for allthe stage functions
2. Init the bomb
    - Does some random memory setting for noise or random num generator seeding???

3. 



## Working Space


### DEMO 1

DEMO_ONE = 10
secret string 10 = "<D1()> <D1()*8>"
secret string 10 = "121 968"


### DEMO 2

DEMO_TWO = 11
secret = ""
mute off (mute(0);)
append_to_secret_string(DEMO_TWO, words[D2()]);
mute on (mute(1);) 
...
mute off (muteflip();)
append_to_secret_string(DEMO_TWO, words[D2()]);
mute on (muteflip();)
...
mute off (muteflip();)
append_to_secret_string(DEMO_TWO, words[D2() + 1] + 3);



secret = ""
    append_to_secret_string(DEMO_TWO, words[D2()]);
secret = "<words[D2()]>"
    append_to_secret_string(DEMO_TWO, words[D2()]);
secret = "<words[D2()]> <words[D2()]>"
    append_to_secret_string(DEMO_TWO, words[D2() + 1] + 3);
secret = "<words[D2()]> <words[D2()]> <words[D2()+1]+3>"

D2 returns 0

secret = "abovementioned abovementioned rementioned"


### Phase Zero

secret = ""
secret = "teleconference"


### Phase One

starting j=171

secret = ""
secret = "171"
secret = "358 732 1106 1480 1854 2228"


### Phase Two

offset = 4
secret = ""
    append_to_secret_string(2, --password1 + offset++);
secret = "lly ?? What a coincidence, I'm shallow too!!"


5:28pm
### Phase Three

secret = ""

On last round of loop (i= 5066):

iterations = 5067
num = 4182

secret = "4182"


5:36
### Phase Four

secret = ""

secret = "987 2584 10946 46368 196418 832040"


5:42
...
~7:58
### Phase Five

secret = ""
secret = "stuvwxyzabcde"
secret = "stuvwxyzabcde incommunicable"


8:06
### Phase Six

secret = ""
phase_six_depth(depth) = 1
PHASE_SIX_CONST = 3
depth = 24

(char)(48 + 3 * 24 % 10) = '2'
depth = 25;
secret = "2"


b 385
condition phase_six_depth(depth)
- use this to work out the last time that the string gets wiped (lets call this LAST_RESET_DEPTH)

Hits:
- 24, 38, 62, 

b 389
condition depth > LAST_RESET_DEPTH
- use this to track all the final mods that happen to the secret string after the final reset

depths:
62, 63, 64, 65, 66, 67, 68, 69, 70, 71

secret string char ascii numbers:
- 54, 57, 50, 53, 56, 49, 52, 55, 48, 51

secret string
- "6 9 2 5 8 1 4 7 0 3"


8:56
### Phase Seven

=== secret = ""

FortuneStruct* fortuneList = phase_seven_start() <- a pointer to a FortuneStruct (basically the list is represented by a pointet to the first node)

fortuneList = fortuneList->next = (struct FortuneStruct *) 0x60d7d0

=== secret = "116"

    for (int i = input[0]; i < input[0] + 8; ++i) {
        ...

b 411
condition ((i % 2 == 0) && ())


odd start
1, 3, 5, 7

even start
0, 2, 4, 6

THIS WILL RUN 4 TIMES:
    fortuneList = fortuneList->next;
    muteflip();
    append_to_secret_string(7, i % 2 + 52 % 2);


fortuneList = fortuneList->next->next->next->next;

at even 2 and 4 (out of 1, 2, 3, 4th in series)
therefore add 1, 1 (out of 1, 1, 1, 1)

=== secret = "116 1 1"

Now onto line 416
print (fortuneList -> next -> next -> next -> next -> len * 3) = 147

=== secret = "116 1 1 147"

Now for line 436
print (int)((fortuneList->next->data)[8])

=== secret = "116 1 1 147 101"


10:02
### Phase Eight

=== secret = ""

offset = 8

b 446
condition (!(i%7))       <-- this will tell us when the char changes

b 447      (the line with "append_to_secret_string(8, next_rchar());")


Need to play out how the program will execute

    if (i is a multiple of 7)
        next_rchar();
        append_to_secret_string(8, next_rchar());

    else, do nothing, skip to next i


Essentially we are performing:
    next_rchar();
    append_to_secret_string(8, next_rchar());
For every multiple of 7 i from 8 to <74


b phase_eight
r
b 447
print (char)next_rchar()


i=14 - 
    next_rchar(), next_rchar() = '_'
i=21 - 
    next_rchar(), next_rchar() = ??? // MUTED
i=28 - 
    next_rchar(), next_rchar() = 's'
i=35 - 
    next_rchar(), next_rchar() = ??? // MUTED
i=42 - 
    next_rchar(), next_rchar() = 'P'
i=49 - 
    next_rchar(), next_rchar() = ??? // MUTED
i=56 - 
    next_rchar(), next_rchar() = 'Z'
i=63 - 
    next_rchar(), next_rchar() = ??? // MUTED
i=70 - 
    next_rchar(), next_rchar() = '9'


=== secret = "_ s P Z 9"


10:46
### Phase Nine

b phase_nine
b 462
print (int)fns[t1[t2[t3[i]]]](i)

=== secret = ""

phase_nine_num() = 9

i=1 - 6
i=2 - 8
i=3 - 7
i=4 - 4
i=5 - 2
i=6 - 3
i=7 - 3
i=8 - 8
i=9 - 6

=== secret = "6 8 7 4 2 3 3 8 6"

11:06pm
