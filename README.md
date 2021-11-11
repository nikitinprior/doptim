The C source code was RESTORED by disassembling the original executable file OPTIM.COM from the Hi-Tech v3.09 compiler.

This file is compiled by Hi-Tech C compiler v3.09 and the resulting executable file performs all the functions provided. To compile, use the following command:

    cc -o optim.c

The created executable file almost completely matches the original image.

The OPTIM utility tries to perform 18 types of optimizations.

OPTIM has 5 options not described in the manual:
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

Andrey Nikitin 11.11.2021
