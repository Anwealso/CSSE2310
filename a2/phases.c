/*
** phases.c
**      Contains the bomb phases.
*/

#include "secret.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PHASE_SIX_CONST 3
#define DEMO_ONE 10
#define DEMO_TWO 11

struct FortuneStruct
{
    char* data;
    int len;
    struct FortuneStruct* next;
};

static struct FortuneStruct* fortuneList = 0;

/* Functions that help us out with the phases */
int phase_zero_num(void);
int phase_one_base(void);
int phase_two_offset(void);
int phase_three_num(void);
int phase_six_depth(int);
struct FortuneStruct* phase_seven_start(void);
int phase_eight_num(void);
int phase_nine_num(void);
int D1(void);
int D2(void);
int new_number(void);
void stir(void);

char* password1;
extern char* phase5string;
extern char* fortunes[];

int t1[13] = { 3, 6, 9, 0, 2, 4, 8, 1, 5, 7, 1, 2, 3 };
int t2[13] = { 9, 6, 3, 8, 4, 2, 0, 7, 5, 1, 9, 8, 7 };
int t3[13] = { 2, 3, 5, 7, 1, 4, 6, 8, 9, 0, 4, 6, 5 };

extern int (*fns[10])(int);

char* fortunes[] = {
    "A bird in the hand is worth what it will bring.",
    "A conclusion is simply the place where someone got tired of thinking.",
    "A physicist is an atom's way of knowing about atoms. -- George Wald",
    "A radioactive cat has eighteen half-lives.",
    "A tautology is a thing which is tautological.",
    "An elephant is a mouse with an operating system.",
    "Any small object that is accidentally dropped will hide under a larger object",
    "Anything worth doing is worth overdoing",
    "Art is anything you can get away with. -- Marshall McLuhan.",
    "Ban the bomb. Save the world for conventional warfare.",
    "Be careful of reading health books, you might die of a misprint.-- Mark Twain",
    "Beware of low-flying butterflies.",
    "Blessed are the young for they shall inherit the national debt.",
    "Confidence is the feeling you have before you understand the situation.",
    "Conscience is a mother-in-law whose visit never ends. -- H. L. Mencken",
    "Dare to be naive. -- R. Buckminster Fuller",
    "Deliver yesterday, code today, think tomorrow.",
    "Don't change the reason, just change the excuses! -- Joe Cointment",
    "Don't get even -- get odd!",
    "Ducharme's Precept: Opportunity always knocks at the least opportune moment.",
    "Experience varies directly with equipment ruined.",
    "Fairy Tale, n.: A horror story to prepare children for the newspapers.",
    "Five is a sufficiently close approximation to infinity. -- Robert Firth",
    "For a man to truly understand rejection, he must first be ignored by a cat.",
    "Happiness is having a scratch for every itch. -- Ogden Nash",
    "Hard work may not kill you, but why take chances?",
    "History repeats itself. That's one thing wrong with history.",
    "How long a minute is depends on which side of the bathroom door you're on.",
    "How wonderful opera would be if there were no singers.",
    "I bet the human brain is a kludge. -- Marvin Minsky",
    "I doubt, therefore I might be.",
    "I refuse to have a battle of wits with an unarmed person.",
    "I used to think I was indecisive, but now I'm not so sure.",
    "I'm going to live forever, or die trying! -- Spider Robinson",
    "I'm prepared for all emergencies but totally unprepared for everyday life.",
    "I've enjoyed just about as much of this as I can stand.",
    "If I don't see you in the future, I'll see you in the pasture.",
    "If I had any humility I would be perfect. -- Ted Turner",
    "If all the world's a stage, I want to operate the trap door. -- Paul Beatty",
    "If at first you don't succeed, redefine success.",
    "If dolphins are so smart, why did Flipper work for television?",
    "If everything is coming your way then you're in the wrong lane.",
    "If time heals all wounds, how come the belly button stays the same?",
    "If two wrongs don't make a right, try three. -- Laurence J. Peter",
    "If we were meant to fly, we wouldn't keep losing our luggage.",
    "If you can't learn to do it well, learn to enjoy doing it badly.",
    "If you don't care where you are, then you ain't lost.",
    "If you have a procedure with 10 parameters, you probably missed some.",
    "If you keep anything long enough, you can throw it away.",
    "If you only have a hammer, you tend to see every problem as a nail.-- Maslow",
    "If you're right 90% of the time, why quibble about the remaining 3%?",
    "It was a book to kill time for those who liked it better dead.",
    "It's always darkest just before it gets pitch black.",
    "It's illegal in Wilbur, Washington, to ride an ugly horse.",
    "Laughter is the closest distance between two people. -- Victor Borge",
    "Let He who taketh the Plunge Remember to return it by Tuesday.",
    "Life is like a simile.",
    "Line Printer paper is strongest at the perforations.",
    "Lubarsky's Law of Cybernetic Entomology: There's always one more bug.",
    "Mother is the invention of necessity.",
    "Never call a man a fool. Borrow from him.",
    "New systems generate new problems.",
    "Nondeterminism means never having to say you are wrong.",
    "Nothing recedes like success. -- Walter Winchell",
    "Nuclear war can ruin your whole compile. -- Karl Lehenbauer",
    "Ogden's Law: The sooner you fall behind, the more time you have to catch up.",
    "One seldom sees a monument to a committee.",
    "Only adults have difficulty with childproof caps.",
    "Overload -- core meltdown sequence initiated.",
    "Paranoia is simply an optimistic outlook on life.",
    "Parker's Law: Beauty is only skin deep, but ugly goes clean to the bone.",
    "People will buy anything that's one to a customer.",
    "Pure drivel tends to drive ordinary drivel off the TV screen.",
    "Reality is for those who can't face Science Fiction.",
    "Really ?? What a coincidence, I'm shallow too!!",
    "Sattinger's Law: It works better if you plug it in.",
    "Save the whales. Collect the whole set.",
    "Smoking is one of the leading causes of statistics. -- Fletcher Knebel",
    "Spare no expense to save money on this one. -- Samuel Goldwyn",
    "Sweater, n.: A garment worn by a child when its mother feels chilly.",
    "The best cure for insomnia is to get a lot of sleep. -- W. C. Fields",
    "The best thing about growing older is that it takes such a long time.",
    "The light at the end of the tunnel is the headlight of an approaching train.",
    "The older a man gets, the farther he had to walk to school as a boy.",
    "The only way to get rid of a temptation is to yield to it. -- Oscar Wilde",
    "The opposite of a profound truth may well be another profound truth. -- Bohr",
    "The world is coming to an end. Please log off.",
    "There are three kinds of lies: Lies, Damn Lies, and Statistics. -- Disraeli",
    "There is no time like the pleasant.",
    "There's a fine line between courage and foolishness. Too bad it's not a fence",
    "Those who can, do. Those who can't, simulate.",
    "Time is an illusion; lunchtime, doubly so. -- Ford Prefect",
    "Time is nature's way of making sure that everything doesn't happen at once.",
    "To be intoxicated is to feel sophisticated but not be able to say it.",
    "To err is human, to moo bovine.",
    "To generalize is to be an idiot. -- William Blake",
    "Too clever is dumb. -- Ogden Nash",
    "Too much of everything is just enough. -- Bob Wier",
    "Universe, n.: The problem.",
    "Virginia law forbids bathtubs in the house; tubs must be kept in the yard.",
    "Wasting time is an important part of living.",
    "We can predict everything, except the future.",
    "We have met the enemy, and he is us. -- Walt Kelly",
    "Wethern's Law: Assumption is the mother of all screw-ups.",
    "What is a magician but a practising theorist? -- Obi-Wan Kenobi",
    "What the large print giveth, the small print taketh away.",
    "What this country needs is a good five cent microcomputer.",
    "What this world needs is a good five-dollar plasma weapon.",
    "What's another word for Thesaurus? -- Steven Wright",
    "Where there's a will, there's an Inheritance Tax.",
    "Which is worse: ignorance or apathy? Who knows? Who cares?",
    "Who messed with my anti-paranoia shot?",
    "Yes, but which self do you want to be?",
    "You can't make a program without broken egos.",
    "You cannot achieve the impossible without attempting the absurd.",
    "You cannot kill time without injuring eternity.",
    "You have the capacity to learn from mistakes. You'll learn a lot today.",
    "Your life would be very empty if you had nothing to regret.",
    "f u cn rd ths, u cn gt a gd jb n cmptr prgrmmng.",
    0
};

char* words[] = {
    "abovementioned",
    "aforementioned",
    "antiperspirant",
    "astrophysicist",
    "cardiovascular",
    "characteristic",
    "chromatography",
    "circumlocution",
    "circumstantial",
    "classification",
    "classificatory",
    "claustrophobia",
    "claustrophobic",
    "committeewoman",
    "comprehensible",
    "concessionaire",
    "congratulatory",
    "consanguineous",
    "conspiratorial",
    "contradistinct",
    "controvertible",
    "counterbalance",
    "counterexample",
    "differentiable",
    "diffractometer",
    "disciplinarian",
    "discriminatory",
    "extemporaneous",
    "featherbedding",
    "ferromagnetism",
    "handicraftsman",
    "histochemistry",
    "hydrochemistry",
    "implementation",
    "inapproachable",
    "incommensurate",
    "incommunicable",
    "incompressible",
    "inconsiderable",
    "incontrollable",
    "indecipherable",
    "indecomposable",
    "indestructible",
    "indeterminable",
    "indiscoverable",
    "indiscriminate",
    "infrastructure",
    "insuppressible",
    "insurmountable",
    "intelligentsia",
    "interferometer",
    "interpretation",
    "intramolecular",
    "irreconcilable",
    "irreproachable",
    "irreproducible",
    "macromolecular",
    "macroprocessor",
    "macrostructure",
    "morphophonemic",
    "multiplication",
    "multiplicative",
    "neuropathology",
    "nitroglycerine",
    "optoelectronic",
    "organometallic",
    "orthophosphate",
    "paralinguistic",
    "parallelepiped",
    "parapsychology",
    "phosphorescent",
    "physiochemical",
    "polysaccharide",
    "predisposition",
    "presentational",
    "prestidigitate",
    "presupposition",
    "proprioception",
    "proprioceptive",
    "psychoacoustic",
    "psychoanalysis",
    "psychoanalytic",
    "quintessential",
    "radioastronomy",
    "radiochemistry",
    "radiotelegraph",
    "radiotelephone",
    "reconnaissance",
    "representative",
    "septuagenarian",
    "servomechanism",
    "slaughterhouse",
    "staphylococcus",
    "superintendent",
    "teleconference",
    "teleprocessing",
    "tetrafluouride",
    "transcendental",
    "transformation",
    "transportation",
    "unidimensional",
    "unidirectional",
    "verisimilitude",
    0
};

void demo_one(char* input)
{
    int num = D1();
    reset_secret_string();
    append_to_secret_string(DEMO_ONE, num);
    append_to_secret_string(DEMO_ONE, 8 * num);
    secret_string_matches(input);
}

void demo_two(char* input)
{
    reset_secret_string();
    mute(0);
    append_to_secret_string(DEMO_TWO, words[D2()]);
    mute(1);
    append_to_secret_string(DEMO_TWO, words[D2()] + 1);
    muteflip();
    append_to_secret_string(DEMO_TWO, words[D2()]);
    muteflip();
    append_to_secret_string(DEMO_TWO, words[D2() + 1] + 2);
    muteflip();
    append_to_secret_string(DEMO_TWO, words[D2() + 1] + 3);
    secret_string_matches(input);
}


void phase_zero(char* input)
{
    int wordNum;
    reset_secret_string();
    mute(0);
    wordNum = phase_zero_num();
    append_to_secret_string(0, words[wordNum]);
    secret_string_matches(input);
}

void phase_one(char* input)
{
    int i, j;
    j = phase_one_base();
    reset_secret_string();
    mute(0);
    for (i = 1; i < 14; i++)
    {
        mute(i % 2);
        append_to_secret_string(1, j);
        j += 187;
    }
    secret_string_matches(input);
}

void phase_two(char* input)
{
    int offset = phase_two_offset();
    reset_secret_string();
    mute(0);
    append_to_secret_string(2, --password1 + offset++);
    secret_string_matches(input);
}

void phase_three(char* input)
{
    int i;
    int iterations;
    int num;

    mute(0);
    iterations = phase_three_num();
    for (i = 0; i < iterations; i++) {
        reset_secret_string();
        num = phase_three_num();
        append_to_secret_string(3, num--);
        num = 0;
    }
    secret_string_matches(input);
}

void phase_four(char* input)
{
    int j;
    int numbers_added = 0;

    reset_secret_string();
    mute(0);
    while (numbers_added != 6)
    {
        j = new_number();
        if (j % 7 == 0 || j % 2 == 0) { append_to_secret_string(4, j); numbers_added++; }
    }
    secret_string_matches(input);
}

void phase_five(char* input)
{
    reset_secret_string();
    mute(0);
    append_to_secret_string(5, phase5string);
    append_to_secret_string(5, phase5string);
    secret_string_matches(input);
}

void phase_six(char* input)
{
    static int depth = 24;

    if (phase_six_depth(depth)) {
        reset_secret_string();
        mute(0);
    }
    /* Append character to secret string (with space between chars) */
    append_to_secret_string(6, (char)(48 + PHASE_SIX_CONST * depth % 10));
    depth += 1;
    if (depth / PHASE_SIX_CONST / 24 == 0) {
        phase_six(input);
        return;
    }
    secret_string_matches(input);
}

void phase_seven(char* input)
{
    /*
    ** Build up list of fortune structures
    */
    fortuneList = phase_seven_start();

    reset_secret_string();
    mute(0);
    fortuneList = fortuneList->next;
    append_to_secret_string(7, fortuneList->next->next->next->len + 52);
    for (int i = input[0]; i < input[0] + 8; ++i) {
        if (i % 2) {
            fortuneList = fortuneList->next;
            muteflip();
            append_to_secret_string(7, i % 2 + 52 % 2);
        }
    }
    append_to_secret_string(
        7,
        fortuneList
        ->
        next
        ->
        next
        ->
        next
        ->
        next
        ->
        len
        *
        3
    )
        ;
    mute(0);
    fortuneList = fortuneList->next->next->next;
    append_to_secret_string(7,
        (int)((fortuneList->next->data)[8]));
    secret_string_matches(input);
}

void phase_eight(char* input)
{
    int offset = phase_eight_num();
    reset_secret_string();
    mute(0);
    for (int i = offset; i < 75; i++) {
        if (!(i % 7)) { next_rchar(); }
        else continue;
        append_to_secret_string(8, next_rchar());
        muteflip();
    }
    secret_string_matches(input);
}

void phase_nine(char* input)
{
    int i;

    reset_secret_string();
    mute(0);

    for (i = 1; i <= phase_nine_num(); i++)
    {
        append_to_secret_string(9, fns[t1[t2[t3[i]]]](i));
    }
    secret_string_matches(input);
}

int fn0(int i)
{
    stir();
    return t1[i];
}

int fn1(int i)
{
    stir();
    return fn0(t2[i]);
}

int fn2(int i)
{
    stir();
    if (i != 6 && fns[i] != fn2)
    {
        i %= 10;
        return fns[i](i);
    }
    else
    {
        return 0;
    }
}

int fn3(int i)
{
    stir();
    return abs(9 - i);
}

int fn4(int i)
{
    stir();
    return (i * i * i) % 10;
}

int fn5(int i)
{
    stir();
    return t3[t3[t3[i]]];
}

int fn6(int i)
{
    stir();
    return 6;
}

int fn7(int i)
{
    stir();
    if (i < 5)
    {
        return 5;
    }
    return i - 3;
}

int fn8(int i)
{
    stir();
    return((i + strlen("Are you confused yet?")) % 10);
}

int fn9(int i)
{
    stir();
    return i >> 1;
}
