The C source code was RESTORED by disassembling the original executable file OPTIM.COM from the Hi-Tech v3.09 compiler.

This file is compiled by Hi-Tech C compiler v3.09 and the resulting executable file performs all the functions provided. To compile, use the following command:

    cc -o optim.c

The created executable file almost completely matches the original image.

The OPTIM utility tries to perform 18 types of optimizations.

OPTIM has 5 options not described in the manual

-l - Prints additional information on each pass;

-n - Prints statistics: number of iterations and number of optimization types performed;

-r - disables register load optimisation;

-f - use inline frame initialisation;

-s - Unbuffered stdout.

Options are unknown to ordinary users and are not used when compiling a program using optimization. These options are probably intended for compiler support to find errors while performing optimization.

Mark Ogden took part in the work on the restoration of the source code of the optimization program in C language. He assigned names to the variables, gave the structures an understandable look, corrected some C functions to exclude labels in the function body, suggested how to interpret and fix several obscure places in the decompiled code, and found errors in the original optimization program.

Mark Ogden's contribution is very large. Without his participation, code restoration would be much slower.

Thanks to Mark Ogden for the effort and time spent on this thankless event. In fact, he is a co-author of the recovered code. 

This version includes support for windows and linux both 32 & 64 bit. It also fixes some bugs in the original code

For compilation to CP / M, the optim.c file is split into the following files:
optim1.h - define common data for a program

optim1.c - functions 1 to 30

part21.c - functions 31 to 49

part31.c - other functions

ctype1.c - Definitions of valid characters and their types in oprtmizer

initvar1.c - Definitions of uninitialized variables and arrays

To compile, you need to run the command

cc -o optim1.c part21.c part31.c ctype1.c initvar1.c

or execute

make

The optimizer program is compiled without diagnostic messages and an executable file is created.

To test the operation of the created executable file, you can create a file in assembly language without optimization using the command

cc -s optim.c

then perform optimization with additional information output

optim1 -n optim.as optim.asm

As a result, information about the performed optimizations will be displayed on the screen.

24K, 3 iterations
370 Redundant labels
499 Jumps to jumps
367 Stack adjustments
731 Temporary labels
683 Unref'ed labels
487 Unreachable code
49 Jumps to. +1
148 Skips over jumps
183 Common code seq's
15 Ex (sp), hl's used
87 Redundant operations
488 Redundant loads / stores
71 Simplified addresses
5 Xor a's used
5 Redundant ex de, hl's
46 Code motions

and the optimized code will be written to the optim.asm file. 

Andrey Nikitin 12.11.2021
