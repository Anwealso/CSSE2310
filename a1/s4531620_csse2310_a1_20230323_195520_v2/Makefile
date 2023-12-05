# Flags for compilation (adding warnings are always good)
CFLAGS = -pedantic -Wall -std=gnu99

# Flags for linking
LDFLAGS = -I /local/courses/csse2310/include

# Libraries to link with
LIBS = -L /local/courses/csse2310/lib -l csse2310a1

# The name of the source files
SOURCES = uqwordiply.c

# The name of the executable
EXE = uqwordiply

default: all

all:
	gcc -g $(CFLAGS) $(LDFLAGS) $(LIBS) $(SOURCES) -o $(EXE)