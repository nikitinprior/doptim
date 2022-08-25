/*
 * HI-TECH Software Optimization Program.
 *
 * File optim.c created 14.08.2021, last modified 11.11.2021.
 *
 * The C source code was RESTORED by disassembling the original executable
 * file OPTIM.COM from the Hi-Tech v3.09 compiler.
 *
 * This file with some warning messages is compiled by Hi-Tech C compiler
 * v3.09 and the resulting executable file performs all the functions
 * provided. To compile, use the following command:
 *
 *    cc -o optim.c
 *
 * The created executable file almost completely matches the original image.
 *
 * The OPTIM utility tries to perform 18 types of optimizations.
 *
 * OPTIM has 5 options not described in the manual:
 *    -l - Prints additional information on each pass;
 *    -n - Prints statistics: number of iterations and number of
 *         optimization types performed;
 *    -r - disables register load optimisation
 *    -f - use inline frame initialisation
 *    -s - Unbuffered stdout;
 *
 * Options are unknown to ordinary users and are not used when compiling a
 * program using optimization. These options are probably intended for
 * compiler support to find errors while performing optimization.
 *
 * Not a commercial goal of this laborious work is to popularize
 * among potential fans of 8-bit computers the old HI-TECH C compiler
 * V3.09 (HI-TECH Software) and extend its life, outside of the
 * CP/M environment (Digital Research, Inc), for full operation in a
 * Unix-like operating system UZI-180 without using the CP/M emulator.
 *
 * This version includes support for windows and linux both
 * 32 & 64 bit. It also fixes some bugs in the original code
 *
 *	Andrey Nikitin 16.10.2021
 *  Mark Ogden 11.11.2021
 */
#include "stdio.h"
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#if defined(__STDC__) || defined(__STDC_VERSION__)
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#else
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned char uint8_t;
#ifndef bool
#define bool char
#define true 1
#define false 0
#endif
#define const
#endif

#ifndef INT_MAX
#define INT_MAX 32767 /* max for int */
#endif
/* instructions only used in the tables*/
#define I_ADC  0x88 /*   Add */
#define I_ADD  0x80 /* */
#define I_BIT  0x40 /*   Tests if the specified bit is set */
#define I_DJNZ 0x10 /* */
#define I_JR   0x18 /* */
#define I_LDIR 0xB0 /*   Load, Inc., Repeat */
#define I_RESB 0x80 /*   Reset bit */
#define I_RL   0x10 /*   rl */
#define I_RLA  0x17 /*   rla */
#define I_RLC  0    /*   rlc */
#define I_RLCA 7    /*   rlca */
#define I_RLD  0x6F /*   rld */
#define I_RR   0x18 /*   rr */
#define I_RRA  0x1F /*   rra */
#define I_RRC  8    /*   rrc */
#define I_RRCA 0xF  /*   rrca */
#define I_RRD  0x67 /*   rrd */
#define I_RST  0xC7 /* */
#define I_SBC  0x98 /*   Subtract */
#define I_SETB 0xC0 /*   Set bit */
#define I_SLA  0x20 /*   sla */
#define I_SLL  0x20 /*   sll */
#define I_SRA  0x28 /*   sra */
#define I_SRL  0x38 /*   srl */
/* instructions used in the code */
#define I_AND  0xA0 /*   Logical AND */
#define I_CCF  0x3F /*   Complement Carry Flag */
#define I_CP   0xB8 /*   Compare */
#define I_CPL  0x2F /*   Complement */
#define I_DI   0xF3 /*   Disable Interrupts */
#define I_EI   0xFB /*   Enable Interrupts */
#define I_EXX  0xD9 /*   Exchange */
#define I_HALT 0x76
#define I_NEG  0x44 /*   Negates the accumulator */
#define I_NOP  0    /*   No operation */
#define I_OR   0xB0 /*   Logical inclusive OR */
#define I_POP  0xC1 /*   pop */
#define I_PUSH 0xC5 /*   push */
#define I_SCF  0x37 /*   Set Carry Flag */
#define I_SUB  0x90 /*   sub */
#define I_XOR  0xA8 /*   xor */
/* special shared instruction */
#define SI_DEC 5 /*   Decrement */
#define SI_INC 4 /*   Increment */

/* the precedence values
   P_NA is for not applicable
*/
#define P_ADD  5 /* */
#define P_AND  4 /* */
#define P_CPAR 1 /* */
#define P_DIV  6 /* */
#define P_EQU  2 /* */
#define P_GE   2 /* */
#define P_HI   8 /* */
#define P_LE   2 /* */
#define P_LOW  8 /* */
#define P_MOD  6 /* */
#define P_MUL  6 /* */
#define P_NA   0 /* */
#define P_NOT  8 /* */
#define P_OPAR 9 /* */
#define P_OR   3 /* */
#define P_RES  7 /* */
#define P_SHL  6 /* */
#define P_SHR  6 /* */
#define P_SUB  5 /* */
#define P_XOR  3 /* */

/****************************************************************
 *	Token definitions
 *
 *	Symbol	  Code	Description
 */
enum tType {
    T_INVALID = 0,
    T_SIMPLE,        /* 1  one byte instruction: */
    T_TWOBYTE,       /* 2  two byte instruction: */
    T_3,             /* 3  Unknown: */
    T_INCDEC,        /* 4  Unknown: */
    T_5,             /* 5  Unknown */
    T_SHIFT,         /* 6  Unknown: */
    T_BIT,           /* 7  Bit operations */
    T_JP,            /* 8  Absolute jumps to address */
    T_JR,            /* 9  Relative jumps to address */
    T_DJNZ,          /* 0xA  Dec., Jump Non-Zero */
    T_CALL,          /* 0xB  Call */
    T_RET,           /* 0xC  Uunknown */
    T_RST,           /* 0xD  Restart Commands */
    T_0xE,           /* 0xE  Unknown */
    T_0xF,           /* 0xF  Unknown */
    T_STK,           /* 0x10  Stack operation: */
    T_EX,            /* 0x11  Exchange */
    T_CARR,          /* 0x12  With Carry: */
    T_CADD,          /* 0x13  Add */
    T_LD,            /* 0x14  Load */
    T_UPLUS,         /* 0x15  unary + */
    T_UMINUS,        /* 0x16  unary - */
    T_NOT,           /* 0x17  Bitwise complement */
    T_HI,            /* 0x18  Hi byte of operand */
    T_LOW,           /* 0x19  Low byte of operand */
    T_RES,           /* 0x1A  .res. */
    T_MARKER,        /* 0x1B  used to demark types */
    T_PLUS,          /* 0x1C  Addition */
    T_MINUS,         /* 0x1D  Subtraction */
    T_MUL,           /* 0x1E  Multiplication */
    T_DIV,           /* 0x1F  Divison */
    T_MOD,           /* 0x20  Modulus */
    T_SHR,           /* 0x21  Shift right */
    T_SHL,           /* 0x22  Shift left */
    T_AND,           /* 0x23  Bitwise AND */
    T_OR,            /* 0x24  Bitwise or */
    T_XOR,           /* 0x25  Exclusive or */
    T_EQ,            /* 0x26  Equality */
    T_LE,            /* 0x27  Signed greater than */
    T_GE,            /* 0x28  Signed less than */
    T_ULE,           /* 0x29  Unigned greater than */
    T_UGE,           /* 0x2A  Unigned less than */
    T_OPAR,          /* 0x2B  Open parenthesis */
    T_CPAR,          /* 0x2C  Closing parenthesis */
    T_LASTOP,        /* 0x2D  used to demark end of ops */
    T_FWD,           /* 0x2E  Local refernce forward */
    T_BWD,           /* 0x2F  Local reference backward */
    T_SYMBOL = 0x31, /* 0x31  Symbol */
    T_CONST,         /* 0x32  Constants */
    T_STRING,        /* 0x33  String of characters */
    T_PSCT,          /* 0x34  Psect */
    T_GLB,           /* 0x35  Global */
    T_COMM,          /* 0x36  Comma */
    T_DEFW,          /* 0x37  Definition word */
    T_DEFL,          /* 0x38  Definition label */
    T_DEFB,          /* 0x39  Definition byte */
    T_DEFM,          /* 0x3A  Definition a message */
    T_DEFS,          /* 0x3B  Memory reservation */
    T_DEFF,          /* 0x3C  Definition real */
    T_EQU,           /* 0x3D  Set value of symbol */

    T_EOL = 0x40, /* 0x40  end of line		';' */
    T_COLN,       /* 0x41  Label separator	':' */

    T_REG = 0x46, /* 0x46  Registers */
    T_COND,       /* 0x47  Condition code: */

    T_INDEXED = 0x64, /* 0x64  (IX / IY) */
    T_ADDRREF,        /* 0x65  (addr expression) */
    T_REGREF          /* 0x66  (reg) */
};
/****************************************************************
 *	Definitions of structures
 *
 * Unfortunately, the members of the structures have been assigned
 * uninformative names. Perhaps there are extra structures. The
 * links between structures and are wrong.
 ****************************************************************/

;

typedef struct {
    char *str;
    int tType;
    int aux;
} operator_t;

typedef struct _list {
    struct _list *pNext;
} list_t;

typedef struct _term {
    int val; /* term's numeric value */
    union {
        struct _sym *pSym;         /* base symbol */
        struct _operand *pOperand; /* or rest of expression */
    } p;
} term_t;

typedef struct _operand {
    uint8_t tType;
    uint8_t aux;
    term_t term;
} operand_t;

typedef struct _inst {
    char *opCode;
    uint8_t tType;
    int aux;
    struct _inst *pNext;
    struct _inst *pAlt;
    union {
        struct {
            operand_t *lhs; /* used for operands */
            operand_t *rhs;
        } o;
        struct { /* used for symbols */
            struct _sym *pSym;
            int symId;
        } s;
    } u;
} inst_t;

typedef struct _sym {
    char *label;
    union {
        inst_t *pInst;       /* is either a pointer to the instruction associated with label */
        operand_t *pOperand; /* or the constant associated with the label */
    } p;
} sym_t;

typedef union {
    char *pChar;
    sym_t *pSym;
    int16_t i;
} YYSTYPE;

/*
 * to avoid lots of u.o. and u.s. used some #defines to simpilfy
 */
#define iLhs      u.o.lhs /* used in inst */
#define iRhs      u.o.rhs
#define iPSym     u.s.pSym
#define iSymId    u.s.symId
#define tPSym     p.pSym /* used in term*/
#define tPInst    p.pInst
#define sPOperand p.pOperand /* used in sym_t && term_t*/
#define oPSym     term.tPSym /* used in operand */
#define oPOperand term.sPOperand
#define oVal      term.val

/****************************************************************
 *	Descriptions of uninitialized variables and arrays
 ****************************************************************/
char yyline[80];                  /* 6f00	Working buffer */
int charsLeft;                    /* 6f50	Length string in input buffer */
char *ptr_inbuf;                  /* 6f52	Pointer to input buffer */
int yytype;                       /* 6f54 */
char inp_buf[80];                 /* 6f56	Input buffer */
bool key_l;                       /* 6fa6	Prints additional information on each pass */
bool key_n;                       /* 6fa7	Prints statistics */
bool key_f;                       /* 6fa8	The action is not clear */
bool key_s;                       /* 6fa9	Key_s = 1 Unbuffered stdout */
int num_warn;                     /* 6faa	Number of errors */
bool key_r;                       /* 6fac */
operator_t const *tableBase;      /* 6fad	Pointer to operators[] */
char *yytext;                     /* 6faf	ok */
YYSTYPE yylval;                   /* 6fb1	Contains different types of data */
int symbolId;                     /* 6fb3	??? used only in sub_0ca2 */
sym_t *gPs;                       /* 6fb5	??? */
inst_t *gPi;                      /* 6fb7 */
/* moved to static in function */ /* int word_6fb9; /* 6fb9	??? */
bool hasChanged;                  /* 6fbb */
bool usesIXorIY;                  /* 6fbc */
int optimiseCounters[18];         /* 6fbd	Array of counters types of optimizations */
enum {
    O_RED_LAB = 0,   /*	  6fbd //  0 Redundant labels */
    O_JMP_TO_JMP,    /*	  6fbf //  1 Jumps to jumps */
    O_STK_ADJUST,    /*	  6fc1 //  2 Stack adjustments */
    O_TMP_LAB,       /*	  6fc3 //  3 Temporary labels */
    O_UNREF_LAB,     /*	  6fc5 //  4 Unref'ed labels */
    O_UNREACH_LAB,   /*	  6fc7 //  5 Unreachable code */
    O_JMP_TO_PLUS1,  /*	  6fc9 //  6 Jumps to .+1 */
    O_SKIP_OVER_JMP, /*	  6fcb //  7 Skips over jumps */
    O_CMN_CODE_SEQ,  /*	  6fcd //  8 Common code seq's */
    O_RED_EXX,       /*	  6fcf //  9 Redundant exx's */
    O_EX_SPHL,       /*	  6fd1 // 10 Ex (sp),hl's used */
    O_RED_OPS,       /*	  6fd3 // 11 Redundant operations */
    O_RED_LD,        /*	  6fd5 // 12 Redundant loads/stores */
    O_SIMPLE_ADDR,   /*	  6fd7 // 13 Simplified addresses */
    O_XOR_A,         /*	  6fd9 // 14 Xor a's used */
    O_RED_EX_DEHL,   /*	  6fdb // 15 Redundant ex de,hl's */
    O_CODE_MOTIONS,  /*	  6fdd // 16 Code motions */
    O_LOOPS_INV      /*	  6fdf // 17 Loops inverted */
};

list_t *freeOperandList; /* 6fe1	struct size 6 */
inst_t *seq1;            /* 6fe3 */
inst_t *seq2;            /* 6fe5  */
inst_t *freeInstList;    /* 6fe7 */
char psect;              /* 6fe9	Program section */
int cur_psect;           /* 6fea	Current program section */
int expectCond;          /* 6fec	ok */
inst_t *word_6fee;       /* 6fee */
int tokType;             /* 6ff0	Token value */
inst_t *switchVectors;   /* 6ff2 */
/* static item_t termTmp; // 6ff4	[4] */
inst_t *root;            /* 6ff8 */
int cntOperand;          /* 6ffa	??? used only in sub_39a3 */
inst_t *word_6ffc;       /* 6ffc */
jmp_buf jmpbuf;          /* 6ffe	[4] */
                         /* 7002	[4] */
int hlDelta;             /* 7006 hl inc/dec from original load */
operand_t regValues[19]; /* 7008 */
char *alloct;            /* 707a	ok */
char *name_fun;          /* 707c	Function name */
                         /* 707e */
                         /* 707f */
list_t *freeItemList;    /* 7080 */
char *allocs;            /* 7082	ok */
char *programBreak;      /* 7084	ok */
/* char       * arry_7086[311];	//7086	[622] */
/* 72f4 */
int asdf;
/****************************************************************
 *	Initialized variables
 ****************************************************************/

#define _Z 0  /* 0000 0000 */
#define _U 1  /* 0000 0001 */
#define _L 2  /* 0000 0010 */
#define _D 4  /* 0000 0100 */
#define _X 8  /* 0000 1000 */
#define _S 16 /* 0001 0000 */

/*
 *	Definitions of valid characters and their types in oprtmizer
 */
char ccClass[] = { /* 62cc */
                   _Z,      _Z,      _Z,      _Z,      _Z,      _Z,      _Z,      _Z,      _S,      _S,      _Z,
                   _Z,      _S,      _S,      _Z,      _Z,      _Z,      _Z,      _Z,      _Z,      _Z,      _Z,
                   _Z,      _Z,      _Z,      _Z,      _Z,      _Z,      _Z,      _Z,      _Z,      _Z,      _S,
                   _Z,      _Z,      _Z,      _L,      _Z,      _Z,      _Z,      _Z,      _Z,      _Z,      _Z,
                   _Z,      _Z,      _Z,      _Z,      _D | _X, _D | _X, _D | _X, _D | _X, _D | _X, _D | _X, _D | _X,
                   _D | _X, _D | _X, _D | _X, _Z,      _Z,      _Z,      _Z,      _Z,      _L,      _Z,      _U | _X,
                   _U | _X, _U | _X, _U | _X, _U | _X, _U | _X, _U,      _U,      _U,      _U,      _U,      _U,
                   _U,      _U,      _U,      _U,      _U,      _U,      _U,      _U,      _U,      _U,      _U,
                   _U,      _U,      _U,      _Z,      _Z,      _Z,      _Z,      _L,      _Z,      _L | _X, _L | _X,
                   _L | _X, _L | _X, _L | _X, _L | _X, _L,      _L,      _L,      _L,      _L,      _L,      _L,
                   _L,      _L,      _L,      _L,      _L,      _L,      _L,      _L,      _L,      _L,      _L,
                   _L,      _L,      _Z,      _Z,      _Z,      _Z,      _Z
};

#define ISALPHA(c)  (ccClass[c] & (_U | _L))
#define ISUPPER(c)  (ccClass[c] & _U)
#define ISDIGIT(c)  (ccClass[c] & _D)
#define ISXDIGIT(c) (ccClass[c] & _X)
#define ISSPACE(c)  (ccClass[c] & _S)
#define ISALNUM(c)  (ccClass[c] & (_U | _L | _D))

/*
 * Determining the condition number
 * (its offset in the conditions[offset] table) */

#define COND_NZ     1 /* 	anz, fnz, lnz,	nz */
#define COND_Z      2 /* 	az,  fz,  lz,	z */
#define COND_LGE    3 /* 	          lge,	nc */
#define COND_LLT    4 /* 	          llt */
/*#define COND_PO	5 // 			po */
/*#define COND_PE	6 // 			pe */
#define COND_GE     7 /* 	age, fge,       p */
#define COND_LT     8 /* 	alt, flt,       m */

char *conditions[] = {
    0,    /* 63db	 0 */
    "nz", /* 63dd	 1 COND_NZ */
    "z",  /* 63df	 2 COND_Z */
    "nc", /* 63e1	 3 COND_LGE */
    "c",  /* 63e3	 4 COND_LLT */
    "po", /* 63e5	 5 COND_PO */
    "pe", /* 63e7	 6 COND_PE */
    "p",  /* 63e9	 7 COND_GE */
    "m"   /* 63eb	 8 COND_LT */
};

/*
 *	Determining register number
 *	(its offset in regs[offset] table)
 */

#define REG_B       0x00
#define REG_C       0x01
#define REG_D       0x02
#define REG_E       0x03
#define REG_H       0x04
#define REG_L       0x05
#define REG_F       0x06
#define REG_A       0x07
#define REG_I       0x08
#define REG_R       0x09 /* Memory Refresh Register */
#define REG_BC      0x0A
#define REG_DE      0x0B
#define REG_HL      0x0C
#define REG_SP      0x0D
#define REG_AF      0x0E
#define REG_AF1     0x0F
#define REG_IX      0x10
#define REG_IY      0x11
#define REG_TRACKER 0x12

char *regs[] = {
    "b",   /* 63ed  0 REG_B */
    "c",   /* 63ef  1 REG_C */
    "d",   /* 63f1  2 REG_D */
    "e",   /* 63f3  3 REG_E */
    "h",   /* 63f5  4 REG_H */
    "l",   /* 63f7  5 REG_L */
    "",    /* 63f9  6 */
    "a",   /* 63fb  7 REG_A */
    "i",   /* 63fd  8 REG_I */
    "r",   /* 63ff  9 REG_R	 Memory Refresh Register */
    "bc",  /* 6401 10 REG_BC */
    "de",  /* 6403 11 REG_DE */
    "hl",  /* 6405 12 REG_HL */
    "sp",  /* 6407 13 REG_SP */
    "af",  /* 6409 14 REG_AF */
    "af'", /* 640b 15 REG_AF1 */
    "ix",  /* 640d 16 REG_IX */
    "iy"   /* 640f 17 REG_IY */
};

operator_t operators[] = {
    /* 6411 */
    { "&", T_AND, P_AND },         /*   0   0	Bitwise AND */
    { "(", T_OPAR, P_OPAR },       /*   1   1	Open parenthesis */
    { ")", T_CPAR, P_CPAR },       /*   2   2	Closing parenthesis */
    { "*", T_MUL, P_MUL },         /*   3   3	Multiplication */
    { "+", T_PLUS, P_ADD },        /*   4   4	Addition */
    { ",", T_COMM, P_NA },         /*   5   5	Comma */
    { "-", T_MINUS, P_SUB },       /*   6   6	Subtraction */
    { ".and.", T_AND, P_AND },     /*   7   7	Bitwise AND */
    { ".high.", T_HI, P_HI },      /*   8   8	Hi byte of operand */
    { ".low.", T_LOW, P_LOW },     /*   9   9	Low byte of operand */
    { ".mod.", T_MOD, P_MOD },     /*  10   a	Modulus */
    { ".not.", T_NOT, P_NOT },     /*  11   b	Bitwise complement */
    { ".or.", T_OR, P_OR },        /*  12   c	Bitwise or */
    { ".res.", T_RES, P_RES },     /*  13   d */
    { ".shl.", T_SHL, P_SHL },     /*  14   e	Shift left */
    { ".shr.", T_SHR, P_SHR },     /*  15   f	Shift right */
    { ".xor.", T_XOR, P_XOR },     /*  16   10	Exclusive or */
    { "/", T_DIV, P_DIV },         /*  17   11	Divison */
    { ":", T_COLN, P_NA },         /*  18   12	Label separator */
    { "<", T_GE, P_GE },           /*  19   13	Signed less than */
    { "=", T_EQ, P_EQU },          /*  20   14	Equality */
    { ">", T_LE, P_LE },           /*  21   15	Signed greater than */
    { "\\", T_NOT, P_NOT },        /*  22   16	Bitwise complement */
    { "^", T_OR, P_OR },           /*  23   17	Bitwise or */
    { "a", T_REG, REG_A },         /*  24   18	Register */
    { "adc", T_CARR, I_ADC },      /*  25   19 Add with Carry */
    { "add", T_CADD, I_ADD },      /*  26   1a	Add */
    { "af", T_REG, REG_AF },       /*  27   1b	Register */
    { "af'", T_REG, REG_AF1 },     /*  28   1c	Register */
    { "age", T_COND, COND_GE },    /*  29   1d	Condition code Arithmetic greater or equal */
    { "alt", T_COND, COND_LT },    /*  30   1e	Condition code Arithmetic less than */
    { "and", T_3, I_AND },         /*  31   1f Logical AND */
    { "anz", T_COND, COND_NZ },    /*  32   20	Condition code */
    { "az", T_COND, COND_Z },      /*  33   21	Condition code */
    { "b", T_REG, REG_B },         /*  34   22	Register b */
    { "bc", T_REG, REG_BC },       /*  35   23	Register bc */
    { "bit", T_BIT, I_BIT },       /*  36   24 Tests if the specified bit is set */
    { "c", T_REG, REG_C },         /*  37   25	Register c */
    { "call", T_CALL, P_NA },      /*  38   26	Call */
    { "ccf", T_SIMPLE, I_CCF },    /*  39   27 Complement Carry Flag */
    { "cp", T_3, I_CP },           /*  40   28 Compare */
    { "cpl", T_SIMPLE, I_CPL },    /*  41   29 Complement */
    { "d", T_REG, REG_D },         /*  42   2a	Register d */
    { "de", T_REG, REG_DE },       /*  43   2b	Register de */
    { "dec", T_INCDEC, SI_DEC },   /*  44   2c Decrement */
    { "defb", T_DEFB, P_NA },      /*  45   2d	Definition byte */
    { "deff", T_DEFF, P_NA },      /*  46   2e	Definition real */
    { "defl", T_DEFL, P_NA },      /*  47   2f	Definition label */
    { "defm", T_DEFM, P_NA },      /*  48   30	Definition a message */
    { "defs", T_DEFS, P_NA },      /*  49   31	memory reservation */
    { "defw", T_DEFW, P_NA },      /*  50   32	Definition word */
    { "di", T_SIMPLE, I_DI },      /*  51   33 Disable Interrupts */
    { "djnz", T_DJNZ, I_DJNZ },    /*  52   34	Dec., Jump Non-Zero */
    { "e", T_REG, REG_E },         /*  53   35	Register */
    { "ei", T_SIMPLE, I_EI },      /*  54   36 Enable Interrupts */
    { "equ", T_EQU, P_NA },        /*  55   37	set value of symbol */
    { "ex", T_EX, P_NA },          /*  56   38	Exchange */
    { "exx", T_SIMPLE, I_EXX },    /*  57   39 Exchange */
    { "fge", T_COND, COND_GE },    /*  58   3a	Condition code ge */
    { "flt", T_COND, COND_LT },    /*  59   3b	Condition code lt */
    { "fnz", T_COND, COND_NZ },    /*  60   3c	Condition code nz */
    { "fz", T_COND, COND_Z },      /*  61   3d	Condition code z */
    { "global", T_GLB, P_NA },     /*  62   3e	Global */
    { "h", T_REG, REG_H },         /*  63   3f	Register h */
    { "hl", T_REG, REG_HL },       /*  64   40	Register hl */
    { "inc", T_INCDEC, SI_INC },   /*  65   41 Increment */
    { "ix", T_REG, REG_IX },       /*  66   42	Register ix */
    { "iy", T_REG, REG_IY },       /*  67   43	Register iy */
    { "jp", T_JP, P_NA },          /*  68   44	Absolute jumps to the address */
    { "jr", T_JR, I_JR },          /*  69   45	Relative jumps to the address */
    { "l", T_REG, REG_L },         /*  70   46	Register l */
    { "ld", T_LD, P_NA },          /*  71   47	Load */
    { "ldir", T_TWOBYTE, I_LDIR }, /*  72   48 Load, Inc., Repeat */
    { "lge", T_COND, COND_LGE },   /*  73   49	Condition code Logical greater or equal */
    { "llt", T_COND, COND_LLT },   /*  74   4a	Condition code Logical less than */
    { "lnz", T_COND, COND_NZ },    /*  75   4b	Condition code nz */
    { "lz", T_COND, COND_Z },      /*  76   4c	Condition code z */
    { "m", T_COND, COND_LT },      /*  77   4d	Condition code Arithmetic less than */
    { "nc", T_COND, COND_LGE },    /*  78   4e	Condition code lge */
    { "neg", T_SIMPLE, I_NEG },    /*  79   4f Negates the accumulator */
    { "nop", T_SIMPLE, I_NOP },    /*  80   50 No operation */
    { "nz", T_COND, COND_NZ },     /*  81   51	Condition code */
    { "or", T_3, I_OR },           /*  82   52 Logical inclusive OR */
    { "p", T_COND, COND_GE },      /*  83   53	Condition code Arithmetic greater or equal */
    { "pop", T_STK, I_POP },       /*  84   54	Stack operation pop */
    { "psect", T_PSCT, P_NA },     /*  85   55	Psect */
    { "push", T_STK, I_PUSH },     /*  86   56	Stack operation push */
    { "r", T_REG, REG_R },         /*  87   57	Register r */
    { "res", T_BIT, I_RESB },      /*  88   58 Reset bit */
    { "rl", T_SHIFT, I_RL },       /*  89   59 06 */
    { "rla", T_SIMPLE, I_RLA },    /*  90   5a 01 */
    { "rlc", T_SHIFT, I_RLC },     /*  91   5b 06 */
    { "rlca", T_SIMPLE, I_RLCA },  /*  92   5c 01 */
    { "rld", T_TWOBYTE, I_RLD },   /*  93   5d 02 */
    { "rr", T_SHIFT, I_RR },       /*  94   5e 06 */
    { "rra", T_SIMPLE, I_RRA },    /*  95   5f 01 */
    { "rrc", T_SHIFT, I_RRC },     /*  96   60 06 */
    { "rrca", T_SIMPLE, I_RRCA },  /*  97   61 01 */
    { "rrd", T_TWOBYTE, I_RRD },   /*  98   62 02 */
    { "rst", T_RST, I_RST },       /*  99   63	Restart Commands */
    { "sbc", T_CARR, I_SBC },      /* 100   64 Subtract with Carry */
    { "scf", T_SIMPLE, I_SCF },    /* 101   65 Set Carry Flag */
    { "set", T_BIT, I_SETB },      /* 102   66 Set bit */
    { "sla", T_SHIFT, I_SLA },     /* 103   67 06 */
    { "sll", T_SHIFT, I_SLL },     /* 104   68 06 */
    { "sp", T_REG, REG_SP },       /* 105   69	Register sp */
    { "sra", T_SHIFT, I_SRA },     /* 106   6a 06 */
    { "srl", T_SHIFT, I_SRL },     /* 107   6b 06 */
    { "sub", T_3, I_SUB },         /* 108   6c 03 */
    { "xor", T_3, I_XOR },         /* 109   6d 03 */
    { "z", T_COND, COND_Z }        /* 110   6e	Condition code */
};

#define NOPERATORS (sizeof(operators) / sizeof(operators[0]))

char *opt_msg[] = {
    /* 68a3 */
    "Redundant labels",       /*  0	 0 */
    "Jumps to jumps",         /*  1	 1 */
    "Stack adjustments",      /*  2	 2 */
    "Temporary labels",       /*  3	 3 */
    "Unref'ed labels",        /*  4	 4 */
    "Unreachable code",       /*  5	 5 */
    "Jumps to .+1",           /*  6	 6 */
    "Skips over jumps",       /*  7	 7 */
    "Common code seq's",      /*  8	 8 */
    "Redundant exx's",        /*  9	 9 */
    "Ex (sp),hl's used",      /* 10	 a */
    "Redundant operations",   /* 11	 b */
    "Redundant loads/stores", /* 12	 c */
    "Simplified addresses",   /* 13	 d */
    "Xor a's used",           /* 14	 e */
    "Redundant ex de,hl's",   /* 15	 f */
    "Code motions",           /* 16	10 */
    "Loops inverted"          /* 17	11 */
};

#define NOPTIM (sizeof(opt_msg) / sizeof(opt_msg[0]))

int ccSwap[] = { /* 68c7 */
                 0, 2, 1, 4, 3, 6, 5, 8, 7
};

/*
 *	psect definitions
 */
#define TEXT   1
#define DATA   2
#define BSS    3
#define SWDATA 4

char *psectNames[] = {
    /* 6a59 */
    "",     /*  0 */
    "text", /*  1 */
    "data", /*  2 */
    "bss",  /*  3 */
    "data"  /*  4 */
};

struct {
    char hiReg;
    char loReg;
} regHiLoMap[] = {
    { 0, 0 },         /* 6c50	REG_B */
    { 0, 0 },         /* 6c52	REG_C */
    { 0, 0 },         /* 6c54	REG_D */
    { 0, 0 },         /* 6c56	REG_E */
    { 0, 0 },         /* 6c58	REG_H */
    { 0, 0 },         /* 6c5a	REG_L */
    { 0, 0 },         /* 6c5c	REG_F */
    { 0, 0 },         /* 6c5e	REG_A */
    { 0, 0 },         /* 6c60	REG_I */
    { 0, 0 },         /* 6c62	REG_R */
    { REG_B, REG_C }, /* 6c64	REG_BC */
    { REG_D, REG_E }, /* 6c66	REG_DE */
    { REG_H, REG_L }, /* 6c68	REG_HL */
};

struct {
    operand_t *pHiRegVal;
    operand_t *pLoRegVal;
} regHiLoValMap[] = {
    /*		;6c6a */
    { &regValues[REG_BC], NULL },             /* 0x0744 0x0000	;6c6a REG_B */
    { &regValues[REG_BC], NULL },             /* 0x7044 0x0000	;6c6e REG_C */
    { &regValues[REG_DE], NULL },             /* 0x704a 0x0000	;6c72 REG_D */
    { &regValues[REG_DE], NULL },             /* 0x704a 0x0000	;6c76 REG_E */
    { &regValues[REG_HL], NULL },             /* 0x7050 0x0000	;6c7a REG_H */
    { &regValues[REG_HL], NULL },             /* 0x7050 0x0000	;6c7e REG_L */
    { &regValues[REG_AF], NULL },             /* 0x705c 0x0000	;6c82 REG_F */
    { &regValues[REG_AF], NULL },             /* 0x705c 0x0000	;6c86 REG_A */
    { NULL, NULL },                           /* 0x0000 0x0000	;6c8a REG_I */
    { NULL, NULL },                           /* 0x0000 0x0000	;6c8e REG_R */
    { &regValues[REG_B], &regValues[REG_C] }, /* 0x7008 0x700e	;6c92 REG_BC */
    { &regValues[REG_D], &regValues[REG_E] }, /* 0x7014 0x701a	;6c96 REG_DE */
    { &regValues[REG_H], &regValues[REG_L] }, /* 0x7020 0x7026	;6c9a REG_HL */
    { NULL, NULL },                           /* 0x0000 0x0000	;6c9e REG_SP */
    { &regValues[REG_A], &regValues[REG_F] }, /* 0x7032 0x702c	;6ca2 REG_AF */
    { NULL, NULL },                           /* 0x0000 0x0000	;6ca6 REG_AF1 */
    { NULL, NULL },                           /* 0x0000 0x0000	;6caa REG_IX */
    { NULL, NULL }                            /* 0x0000 0x0000	;6cae REG_IY */
};

int regTestMasks[] = {
    0x01,  /* 6cb2 REG_B */
    0x02,  /* 6cb4 REG_C */
    0x04,  /* 6cb6 REG_D */
    0x08,  /* 6cb8 REG_E */
    0x10,  /* 6cba REG_H */
    0x20,  /* 6cbc REG_L */
    0x40,  /* 6cbe REG_F */
    0x80,  /* 6cc0 REG_A */
    0x00,  /* 6cc2 REG_I */
    0x00,  /* 6cc4 REG_R */
    0x03,  /* 6cc6 REG_BC */
    0x0C,  /* 6cc8 REG_DE */
    0x30,  /* 6cca REG_HL */
    0x00,  /* 6ccc REG_SP */
    0xC0,  /* 6cce REG_AF */
    0x00,  /* 6cd0 REG_AF1 */
    0x100, /* 6cd2 REG_IX */
    0x200, /* 6cd4 REG_IY */
};

/****************************************************************
 * Prototype functions are located in sequence of being in
 * original binary image of OPTIM.COM
 *
 * ok++ - Full binary compatibility with code in original file;
 * ok+  - Code generated during compilation differs slightly,
 *        but is logically correct;
 ****************************************************************/
int strtoi(char const *fmt, int p2); /*  1 sub_013d ok++ */
char *ptr_token();                   /*  2 sub-020f ok++ */
int const_value();                   /*  3 sub_0289 ok+ */
int get_token();                     /*  4 sub_03c7 ok+ */
int get_line();                      /*  5 sub_0758 ok+ */
void clr_len_inbuf();                /*  6 sub_07aa ok++ */
int main(int, char **);              /*  7 sub_07b1 ok+ */
#if defined(__STDC__) || defined(__STDC_VERSION__)
void _Noreturn pr_error(char const *fmt, ...);
void pr_warning(char const *fmt, ...);
void pr_message(char const *fmt, va_list args);
#else
void pr_error();   /*  8 sub_0941 ok++ */
void pr_warning(); /*  9 sub_096f ok++ */
void pr_message(); /* 10 sub_0994 ok++ */
#endif
int find_token(char const *, operator_t const *, int);   /* 11 sub_09d0 ok++ */
int num_token(char const *);                             /* 12 sub_0a97 ok++ */
void pr_token(inst_t const *);                           /* 13 sub_0ab2 ok++ */
operand_t *allocOperand();                               /* 16 sub_0ba7 ok++ */
inst_t *allocInst(inst_t *);                             /* 17 sub_0be2 ok++ */
inst_t *syntheticLabel(inst_t *);                        /* 18 sub_0ca2 ok++ */
void optimise();                                         /* 19 sub_0ce4 ok++ */
void chkIXYUsage();                                      /* 20 sub_0e67 ok++ */
void sub_0ed1();                                         /* 21 sub_0ed1 ok++ */
bool sub_1071(inst_t *);                                 /* 22 sub_1071 ok++ */
void sub_122f();                                         /* 23 sub_122f		-- */
bool sub_1369(operand_t const *);                        /* 24 sub_1369 ok++ */
void removeInstruction(inst_t *);                        /* 25 sub_1397 ok++ */
inst_t *getNextRealInst(inst_t *);                       /* 26 sub_140b ok++ */
bool operandsSame(operand_t const *, operand_t const *); /* 27 sub_142f ok++ */
bool instructionsSame(inst_t const *, inst_t const *);   /* 28 sub_14ac ok++ */
void removeLabelRef(sym_t *);                            /* 29 sub_153d ok++ */
void sub_15ad();                                         /* 30 sub_15ad ok++ */
bool sub_1795();                                         /* 31 sub_1795 ok++ */
bool sub_1aec();                                         /* 32 sub_1aec ok++ */
bool sub_1b86();                                         /* 33 sub_1b86 ok++ */
bool sub_1c67();                                         /* 34 sub_1c67 ok++ */
bool sub_1d94();                                         /* 35 sub_1d94 ok++ */

void sub_1ec1(); /* 36 sub_1ec1 */
bool sub_23c1(); /* 37 sub_23c1 */
bool sub_24c0(); /* 38 sub_24c0 ok++ */
bool sub_29c3(); /* 39 sub_29c3 Problem */
bool sub_2bdb(); /* 40 sub_2bdb */
bool sub_2d3b(); /* 41 sub_2d3b */
bool sub_2ef8(); /* 42 sub_2ef8 ok++ */
bool sub_3053(); /* 42asub_3053 strange code */

void swapHLDE();                                    /* 43 sub_31ee ok++ */
void pr_psect(int psect);                           /* 44 sub_328a ok++ */
int num_psect(char const *);                        /* 45 sub_32bf ok++ */
term_t *evalExpr();                                 /* 46 sub_3313 ok++ */
void exp_err();                                     /* 47 sub_3595 ok++ */
void uconv(int, term_t *);                          /* 48 sub_359e ok+ */
void bconv(int, term_t *, term_t const *);          /* 49 sub_3630 ok++ */
void rel_err();                                     /* 50 sub_384d ok++ */
operand_t *evalOperand();                           /* 51 sub_3856 ok+ */
void oper_err();                                    /* 52 sub_398e ok++ */
void getOperands(inst_t *);                         /* 53 sub_39a3 ok++ */
void loadFunction();                                /* 54 sub_3a15 ok	-+ */
bool sub_4000(inst_t const *);                      /* 55 sub_4000 ok+ */
void sub_404d();                                    /* 56 sub_404d ok++ */
void pr_instruction(inst_t *);                      /* 57 sub_420a ok	-- */
void sub_436e(operand_t const *);                   /* 58 sub_436e ok++ */
void sub_44b2(operand_t const *);                   /* 59 sub_44b2 ok+ */
void sub_4544(int);                                 /* 60 sub_4544 ok+ */
void sub_4601();                                    /* 61 sub_4601 ok+ */
bool sub_4625(inst_t const *);                      /* 62 sub_4625 ok+ */
bool sub_4682(operand_t const *);                   /* 63 sub_4682 ok++ */
int sub_46b1(operand_t const *, int);               /* 64 sub_46b1 ok++ */
int sub_475c(operand_t const *, int);               /* 65 sub_475c ok++ */
int sub_47a2(operand_t const *, int);               /* 66 sub_47a2 ok++ */
bool sub_47e0(int, inst_t const *, inst_t const *); /* 67 sub_47e0 ok+ */
sym_t *allocItem();                                 /* 68 sub_4c33 ok+ */
void freeSymbol(sym_t *);                           /* 69 sub_4c6b ok+ */
int hash_index(char const *, int);                  /* 70 sub_4cab ok++ */
sym_t *lookupSym(char const *);                     /* 71 sub_4cf0 ok	-- */
sym_t *allocBlankSym();                             /* 72 sub_4da7 ok++ */
void resetHeap();                                   /* 73 sub_4dbf ok++ */
void freeHashtab();                                 /* 74 sub_4e20 ok++ */
void *alloc_mem(int);                               /* 75 sub_4e2d ok+ */

void *sbrk(int);
int brk(void *);
void heapchk(void const *p);
#ifdef _DEBUG
#define HEAP(p) heapchk(p)
#else
#define HEAP(p)
#endif

/* common code sequences */

#define PEEKCH()       (charsLeft > 0 ? *ptr_inbuf : '\n')
#define GETCH()        (--charsLeft >= 0 ? *ptr_inbuf++ : get_line())

#define logOptimise(n) (optimiseCounters[n]++, hasChanged = true)

/**************************************************************************
 1	strtoi	sub_013d	ok++ (PMO)	Used in const_value
 *	Converts a numeric string, in various bases, to a integer
 **************************************************************************/

int strtoi(char const *s, int base) {
    int val;   /* number */
    int digit; /* digit */

    val = 0;
    while (*s) {
        val *= base;
        if (ISDIGIT(*s)) {
            digit = *(s++) - '0';
        } else {
            if (ISXDIGIT(*s)) {
                digit = (ISUPPER(*s) ? (char)(*s | 0x20) : (char)*s) - ('a' - 10);
            }
        }
        /* #pragma warning(suppress : 6001) /* function only called after verified there is a digit */
        if (digit >= base) {
            pr_warning("Digit out of range");
            return 0;
        }
        val += digit;
    }
    return val;
}

/**************************************************************************
 2	ptr_token	sub_020f	ok++ (PMO)
 *	Returns a pointer to a token.
 **************************************************************************/
char *ptr_token() {
    register char *s;

    s = yyline;
    while (PEEKCH() != 0 && PEEKCH() != '\n')
        *s++ = GETCH();

    *s        = '\0';
    charsLeft = 0;
    return yyline;
}

/**************************************************************************
 3	const_value	sub_0289	ok++ (PMO)
 *	Rerurn integer value numeric string in various bases
 *
 **************************************************************************/
int const_value() {
    register char *pc;
    uint8_t base;

    pc   = yyline + 1;
    base = 0;
    while (ISXDIGIT(PEEKCH()))
        *pc++ = GETCH();

    switch (PEEKCH()) { /* m17: */
    case 'H':
    case 'h':
        base = 16;
        /* #pragma warning(suppress : 6269) /* deference ignored - original code did the dreference */
        GETCH();
        break;

    case 'O':
    case 'o':
    case 'Q':
    case 'q':
        base = 8;
        /* #pragma warning(suppress : 6269) /* deference ignored - original code did the dreference */
        GETCH();
        break;
    default:
        if (pc[-1] == 'f' || pc[-1] == 'b') {
            yytype   = *--pc == 'f' ? T_FWD : T_BWD;
            *pc      = '\0';
            yylval.i = (int16_t)strtoi(yyline, 10);
            return yytype;
        }
        if (pc[-1] == 'B') {
            pc--;
            base = 2;
        }
        break;
    }

    *pc = '\0';
    if (base == 0)
        base = 10;
    yylval.i = (int16_t)strtoi(yyline, base);
    return T_CONST;
}

/**************************************************************************
 4	get_token	sub_03c7	ok++ (PMO)
 **************************************************************************/
int get_token() {
    int c;
    register char *pc;

    for (;;) {
        pc = yyline;

        switch (c = GETCH()) {
        case -1:
            return -1;
        case '\t':
        case '\f':
        case ' ':
            continue;
        case 0:
        case ';':
            return T_EOL;
        case '\'': /* Single quote (apostrophe) */
            while (PEEKCH() && PEEKCH() != '\'')
                *pc++ = GETCH();
            if (PEEKCH() == '\'')
                /* #pragma warning(suppress : 6269) /* deference ignored - original code did the dreference */
                GETCH();
            else
                pr_warning("Unterminated string");
            *pc          = '\0';
            yylval.pChar = yyline;
            return T_STRING;

        case '.':
            *pc++ = c;
            while (ISALPHA(PEEKCH()))
                *pc++ = GETCH();
            if (PEEKCH() == '.') /* m27: */
                *pc++ = GETCH();
            break;
        case ',':
            return T_COMM; /* ","						//m31: */

        case ':':
            return T_COLN; /*":"						//m32: */

        case '(':
            yylval.i = P_OPAR; /* m33: */
            return T_OPAR;     /* "(" */

        case ')':
            yylval.i = P_CPAR; /* m34: */
            return T_CPAR;     /* ")" */

        case '+':
            yylval.i = P_ADD; /* m35: */
            return T_PLUS;    /* "+" */

        case '-':
            yylval.i = P_SUB; /* m36: */
            return T_MINUS;   /* "-" */

        case '*':
            if (ptr_inbuf == inp_buf + 1) {
                printf("%s\n", inp_buf);
                charsLeft = 0;
                continue;
            }
        default:
            *pc++ = c;                    /* m50: */
            if (ISALPHA(c)) {             /* goto m46; */
                while (ISALNUM(PEEKCH())) /* m41: */
                    *pc++ = GETCH();
                if (pc == yyline + 2) {
                    if (PEEKCH() == '\'')
                        *pc++ = GETCH();
                }
            } else if (ISDIGIT(c))
                return const_value();
            break;
        }
        while (ISSPACE(PEEKCH()))
            /* #pragma warning(suppress : 6269) /* deference ignored - original code did the dreference */
            GETCH();

        *pc = '\0';
        if (yyline[0] == '_' || ISDIGIT(pc[-1]) || ((yytype = num_token(yyline)) == -1)) {
            if (ISALPHA(yyline[0])) { /* m57: */
                yylval.pSym = lookupSym(yyline);
                return T_SYMBOL;
            }
            pr_warning("Lexical error"); /* m58: */
        } else
            return yytype; /* m59: */
    }
}

/**************************************************************************
 5	get_line	sub_0758	ok++ (PMO)
 *	Optimiser eliminates a jump otherwise code is identical
 **************************************************************************/
/* PMO this code contains a bug. If strlen returns a 0 it accesses inp_buf[-1]
 * and could overwrite with a 0, this could occur if there is a '\0' in the input stream
 * it is also not clear whether a blank line i.e. one with just a '\n' should
 * return with a '\0' as it currently does or try another line
 *
 * Note needs fixing for linux as '\r' will not be removed on input
 *
 * The compiler can generate comment lines > 80 chars, this causes this function
 * to split the line which may cause the optimiser to get confused
 * a simple option would be to extend the size of inp_buf alternatively as the only
 * long lines appear to be comment ones. A better option would be to detect for a missing '\n'
 * and junk the rest of the line
 *
 * WARNING: The Hitech fgets contains a bug in that if line read has length >= size
 * it will write a 0 at buf[size], for optim this is the -l option flag
 *
 */
#ifdef CPM
int get_line() {
    for (;;) {
        if (fgets(inp_buf, sizeof(inp_buf), stdin) == NULL)
            return -1;
        charsLeft = (int)strlen(ptr_inbuf = inp_buf);
        if (inp_buf[charsLeft - 1] == '\n') /* If penultimate character is */
            inp_buf[charsLeft - 1] = '\0';  /* '\n' then replace it with 0 */
        if (--charsLeft >= 0) {             /* got something */
            return *ptr_inbuf++;            /* otherwise, return first character of string */
        }
    }
}

#else
#define CPMEOF 0x1A
int get_line() {
    int c;

    charsLeft = 0;
    while ((c = getchar()) != '\n' && c != EOF && c != CPMEOF)
        if (charsLeft < sizeof(inp_buf) - 1 && c != '\r')
            inp_buf[charsLeft++] = c;
    if (c == CPMEOF)
        while ((c = getchar()) != EOF)
            ;
    inp_buf[charsLeft] = '\0';
    ptr_inbuf          = inp_buf;
    return charsLeft || c != EOF ? *ptr_inbuf++ : EOF;
}
#endif
/**************************************************************************
 6	clr_len_inbuf	sub_0758	ok++ (PMO)
 **************************************************************************/
void clr_len_inbuf() {
    charsLeft = 0;
}

/**************************************************************************
 7	main	sub_07b1	ok++ (PMO)
 in the switch statement the location of the code blocks for 's' and default
 are swapped by the optimiser, otherwise the code is identical
 **************************************************************************/
int main(int argc, char **argv) {

    --argc, ++argv; /* would be cleaner as a for loop */
    while (0 < argc && argv[0][0] == '-') {
        switch (argv[0][1]) {
        case 'N':
        case 'n':
            key_n = true;
            break; /* Enables statistics printing */

        case 'L':
        case 'l':
            key_l = true;
            break; /* Enables printing of additional information */

        case 'R':
        case 'r':
            key_r = true;
            break; /* */

        case 'F':
        case 'f':
            key_f = true;
            break; /* */

        case 's':
            key_s = true;
            break; /* Unbuffered stdout */
        default:
            pr_error("Illegal switch %c", argv[0][1]);
            break;
        }
        ++argv, --argc;
    }
    if (argc > 0) {
        if (!freopen(*argv, "r", stdin))
            pr_error("Can't open %s", *argv);

        if (argc > 1 && !freopen(*(argv + 1), "w", stdout))
            pr_error("Can't create %s", *(argv + 1));
    }
    if (key_s)
        setbuf(stdout, 0); /* Unbuffered stdout (Turns off buffering) */

    optimise();

    if (fclose(stdout) == -1) {
        pr_warning("Error closing output file");
        ++num_warn;
    }
    exit(num_warn != 0);
}

/**************************************************************************
 8	pr_error	sub-0941	ok++ (PMO)
 **************************************************************************/
#ifdef CPM
void pr_error(fmt, p2, p3) char *fmt;
{

    pr_message(fmt, p2, p3);
    fclose(stdout);
    exit(1);
}

/**************************************************************************
 9	pr_warning	sub_096f	ok++ (PMO)
 **************************************************************************/
void pr_warning(fmt, p2, p3) char *fmt;
{
    pr_message(fmt, p2, p3);
    ++num_warn;
}

/**************************************************************************
 10	pr_message	sub_0994	ok++ (PMO)
 **************************************************************************/
void pr_message(fmt, p2, p3) char *fmt;
{

    fprintf(stderr, "optim: ");
    fprintf(stderr, fmt, p2, p3);
    fputc('\n', stderr);
}

#else
void pr_error(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    pr_message(fmt, args);
    va_end(args);
    exit(1);
}

void pr_warning(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    pr_message(fmt, args);
    va_end(args);
    ++num_warn;
}

void pr_message(char const *fmt, va_list args) {

    fprintf(stderr, "optim: ");
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
}

#endif

/**************************************************************************
 11	find_token	sub_09d0	ok++ (PMO)	Used in num_token
 Minor difference after strcmp. the code sequence
 original           new
 ld (ix-1),l        ld a,l
 ld a,l             ld (ix-1),a
 **************************************************************************/
int find_token(register char const *str, operator_t const *p2, int p3) {
    char cmp;
    uint8_t high, low, mid;

    tableBase = p2;
    low       = 0;
    high      = p3 - 1;
    do {
        mid = (high + low) / 2;
        cmp = strcmp(str, tableBase[mid].str);
        if (cmp == 0) {
            yylval.i = tableBase[mid].aux;
            yytext   = tableBase[mid].str;
            return tableBase[mid].tType;
        }
        if (cmp < 0)
            high = mid - 1;
        else
            low = mid + 1;
    } while (high >= low);
    return -1;
}

/**************************************************************************
 12	num_token	sub_0a97	ok++ (PMO)	Used in
 **************************************************************************/
int num_token(char const *fmt) {

    return find_token(fmt, operators, NOPERATORS);
}

/**************************************************************************
 13	pr_token	sub_0ab2	ok++ (PMO)
 Only difference is the new code jumps to a location that cleans up
 stack after a call before jumping to cret. The direct jp cret also works
 **************************************************************************/
void pr_token(register inst_t const *ilist) {
    operator_t const *po;

    if (ilist->opCode) {
        printf("%s", ilist->opCode);
        return; /* m1: */
    }

    switch (ilist->tType) { /* m2: */
    case T_JP:
        printf("jp");
        return;
    case T_CALL:
        printf("call");
        return;
    case T_RET:
        printf("ret");
        return;
    }

    po = operators + NOPERATORS; /*    PMO should not be + 1 */
    do {
        if (--po < operators) { /* m3: */
            pr_error("Can't find op");
            return;
        }
    } while ((ilist->tType != po->tType) || (po->aux != ilist->aux)); /* m8: */

    printf("%s", po->str);
    return;
}

/**************************************************************************
 14	sub_0b6a	ok++ (PMO)
 **************************************************************************/
void freeOperand(register operand_t *po) {

    if (!po)
        return;
    ((list_t *)po)->pNext = freeOperandList;
    freeOperandList       = (list_t *)po;
}

/**************************************************************************
 15	sub_0b8b	ok++ (PMO)
 **************************************************************************/
void freeInst(register inst_t *ilist) {

    ilist->pAlt     = freeInstList;
    freeInstList = ilist;
}
/**************************************************************************
 16	sub_0ba7	ok++ (PMO)
 **************************************************************************/
operand_t *allocOperand() {
    register operand_t *ilist;

    if (freeOperandList) {
        ilist = (operand_t *)freeOperandList;
        HEAP(ilist);
        freeOperandList = ((list_t *)ilist)->pNext;
        ilist->tType = ilist->aux = 0;
        ilist->oVal           = 0;
        ilist->oPSym          = NULL;
        return ilist;
    }
    return (operand_t *)alloc_mem(sizeof(operand_t));
}

/**************************************************************************
 17	sub_0be2	ok++ (PMO)
 **************************************************************************/
inst_t *allocInst(register inst_t *ilist) {
    inst_t *l1;
    HEAP(ilist->pNext);
    if ((l1 = freeInstList)) {
        HEAP(l1);
        freeInstList = l1->pAlt;
        l1->tType = l1->aux = 0;
        l1->iLhs = l1->iRhs = NULL;
        l1->opCode          = NULL;
    } else {
        l1 = alloc_mem(sizeof(inst_t));
    }
    l1->pNext = ilist->pNext;
    l1->pAlt  = ilist;
    if (ilist->pNext) {
        HEAP(ilist->pNext);
        ilist->pNext->pAlt = l1;
    }
    HEAP(l1);
    return ilist->pNext = l1;
}

/**************************************************************************
 18	sub_0ca2	ok++ (PMO)
 **************************************************************************/
inst_t *syntheticLabel(register inst_t *ilist) {
    ilist                = allocInst(ilist);
    ilist->iPSym         = allocBlankSym();
    ilist->iSymId        = ++symbolId;
    ilist->iPSym->tPInst = ilist;
    ilist->tType          = T_SYMBOL;
    return ilist;
}

/**************************************************************************
19	optimise	sub-0ce4	ok++ (PMO)
Apart from bug fix change, the code is the same
**************************************************************************/
void optimise() {
    int l1;
    int l2;
    int iteration;
    char *l4;
    bool l5;
    char *l6;

    l6 = l4   = sbrk(0); /* PMO bug fix set l6 */
    iteration = 0;
    while (!feof(stdin)) { /* not eof */
        freeInstList    = NULL;
        freeOperandList = NULL;
        resetHeap();    /* fun_73 */
        loadFunction(); /* fun_54 */
        freeHashtab();  /* fun_74 */
        sub_0ed1();     /* fun_21 */
        chkIXYUsage();  /* fun_20 */
        sub_122f();     /* fun_23 */

        l2 = 0;
        do {
            if (iteration < ++l2)
                iteration = l2;

            if (key_l) {
                printf("***** Pass %d\n", l2);
                sub_404d();
            }
            l5 = false;
            do {
                hasChanged = false; /* m5: */
                sub_15ad();
                l5 |= hasChanged;
            } while (hasChanged);
            do {
                hasChanged = false; /* m6: */
                sub_1ec1();
            } while (hasChanged);

        } while ((l5 |= hasChanged));

        sub_404d();

        if (l6 < (char *)sbrk(0))
            l6 = sbrk(0);
    } /* end while */
    if (!key_n)
        return;
    /* fclose(stdout); This statement removed as fclose(stdout) is also done in main */
    fprintf(stderr, "%dK, %d iterations\n", ((int)(l6 - l4) + 0x3ff) / 0x400, iteration);

    for (l1 = 0; l1 < NOPTIM; l1++)
        if (optimiseCounters[l1] != 0)
            fprintf(stderr, "%d %s\n", optimiseCounters[l1], opt_msg[l1]);
}

/**************************************************************************
 20	sub_0e67	ok++ (PMO)	used in optimise
 **************************************************************************/
void chkIXYUsage() {
    register inst_t const *ilist;

    usesIXorIY = false;
    for (ilist = root; ilist; ilist = ilist->pNext) {
        if (ilist->tType != T_SYMBOL &&
            ((ilist->iLhs && ilist->iLhs->tType == T_INDEXED) || (ilist->iRhs && ilist->iRhs->tType == T_INDEXED) ||
             (ilist->iLhs && ilist->iLhs->tType == T_REG && ilist->iLhs->aux >= REG_IX))) {
            usesIXorIY = true;
            return;
        }
    }
}

/**************************************************************************
 21	sub_0ed1	ok++ (PMO)	used in optimise
 **************************************************************************/
void sub_0ed1() {
    inst_t *pi2;
    operand_t *po;
    bool newLabel; /* Flag of the sub_0ca2 call */
    register inst_t *pi1;

    for (pi1 = word_6ffc; pi1; pi1 = pi1->pAlt) {
        if (pi1->tType == T_CONST) {
            newLabel = false;
            logOptimise(O_TMP_LAB); /* 6fc3 opt_msg[3] = "Temporary labels" */
            for (pi2 = pi1->pAlt; pi2; pi2 = pi2->pAlt) {
                if (pi2->tType == T_CONST && pi2->aux == pi1->aux)
                    break;
                if ((po = pi2->iLhs) && po->tType == T_FWD && po->oVal == pi1->aux) {
                    if (!newLabel) {
                        syntheticLabel(pi1);
                        newLabel = true;
                    }
                    po->tType      = T_CONST;
                    po->oPOperand = pi1->pNext->iLhs;
                    po->oVal      = 0;
                }
            }
            if (pi1->aux < REG_BC) { /* m7: */
                if (!newLabel)
                    syntheticLabel(pi1);
                for (pi2 = pi1->pNext; pi2; pi2 = pi2->pNext) {
                    if ((po = pi2->iLhs) && po->tType == T_BWD && po->oVal == pi1->aux) {
                        po->tType      = T_CONST;
                        po->oPOperand = pi1->pNext->iLhs;
                        po->oVal      = 0;
                        break;
                    }
                }
            }
            pi1 = pi1->pNext; /* m12: */
            removeInstruction(pi1->pAlt);
        }
    }
}

/**************************************************************************
 22	sub_1071	ok++ (PMO)	used in sub_15ad
 **************************************************************************/
bool sub_1071(register inst_t *ilist) {

    inst_t *pi1;
    inst_t *pi2;
    inst_t *pi3;

    if (ilist->tType != T_JP || ilist->aux != 0 || ilist->iLhs->tType != T_CONST)
        return false;

    pi1 = ilist->iLhs->oPSym->p.pInst;
    while (pi1->tType == T_SYMBOL)
        pi1 = pi1->pAlt;

    if (pi1 == ilist) {
        removeInstruction(ilist);
        logOptimise(O_JMP_TO_PLUS1); /* 6fc9 opt_msg[6] = "Jumps to .+1" */
        return false;
    }
    if (pi1->tType != T_JP || pi1->aux != 0)
        return false;

    for (pi3 = pi1->pNext; pi3->tType && (pi3->tType != T_JP || pi3->aux != 0); pi3 = pi3->pNext)
        ;

    if (pi3 == pi1->pNext || pi3->tType == T_INVALID || pi3 == ilist)
        return false;

    ilist->pNext->pAlt  = pi3;
    pi1->pNext->pAlt = ilist;

    pi3->pNext->pAlt = pi1;

    pi2              = pi1->pNext;
    pi1->pNext       = pi3->pNext;
    pi3->pNext       = ilist->pNext;
    ilist->pNext        = pi2;
    logOptimise(O_CODE_MOTIONS); /* 6fdd opt_msg[16] = "Code motions" */
    removeInstruction(ilist);
    return logOptimise(O_JMP_TO_PLUS1); /* 6fc9 opt_msg[6] = "Jumps to .+1" */
}

/**************************************************************************
 23	sub_122f	ok++ (PMO)	Used in: optimise
 **************************************************************************/
void sub_122f() {
    operand_t *po;
    register inst_t *ilist;

    for (ilist = root->pNext; ilist; ilist = ilist->pNext) /* set initial values for symbols */
        if (ilist->tType == T_SYMBOL)
            if (ilist->iPSym->label[0] == '_') /* check for public name */
                ilist->aux = INT_MAX;
            else
                ilist->aux = 0;

    for (ilist = root->pNext; ilist; ilist = ilist->pNext) { /* update reference counts */
        if (ilist->tType == T_JP || ilist->tType == T_DJNZ) {
            if ((po = ilist->iLhs) && po->tType == T_CONST && po->oPSym && po->oPSym->p.pInst)
                po->oPSym->p.pInst->aux++;
        }
    }
    for (ilist = switchVectors; ilist; ilist = ilist->pNext) { /* do the same for the jump tables */
        if (ilist->tType == T_DEFW) {
            if (ilist->iLhs && (po = ilist->iLhs)->tType == T_CONST && po->oPSym && po->oPSym->p.pInst)
                po->oPSym->p.pInst->aux++;
        }
    }
}

/**************************************************************************
 24	sub_1369	ok++ (PMO)
 **************************************************************************/
bool sub_1369(register operand_t const *ilist) {

    return ilist->tType == T_CONST || ilist->tType == T_INDEXED || (ilist->tType == T_REGREF && ilist->aux == REG_HL);
}

/**************************************************************************
 25	sub_1397	ok++ (PMO)
 **************************************************************************/
/* note there are occasions when pi is accessed after this is called so
 *  freeInst has to preserve at least pi->pNext
 */
void removeInstruction(register inst_t *ilist) {

    if (ilist->tType == T_JP && ilist->iLhs->tType == T_CONST && ilist->iLhs->oPSym)
        removeLabelRef(ilist->iLhs->oPSym);

    ilist->pAlt->pNext = ilist->pNext;
    ilist->pNext->pAlt = ilist->pAlt;
    if (ilist->tType != T_SYMBOL) {
        freeOperand(ilist->iLhs);
        freeOperand(ilist->iRhs);
    }
    freeInst(ilist);
}

/**************************************************************************
 26	sub_140b	ok++ (PMO)
 **************************************************************************/
inst_t *getNextRealInst(register inst_t *ilist) {

    while (ilist->tType == T_SYMBOL)
        ilist = ilist->pNext;
    return ilist;
}

/**************************************************************************
 27	sub_142f	ok++ (PMO)
 **************************************************************************/
bool operandsSame(register operand_t const *po1, operand_t const *po2) {

    if (!po1 && !po2)
        return true;
    if (!po1 || !po2)
        return false;

    if (po1->tType != po2->tType || po1->aux != po2->aux || po1->tType == T_INVALID)
        return false;

    return po1->oPSym == po2->oPSym && po1->oVal == po2->oVal;
}

/**************************************************************************
 28	sub_14ac	ok++ (PMO)
 **************************************************************************/
bool instructionsSame(register inst_t const *pi1, inst_t const *pi2) {

    if (pi1->tType == T_INVALID || pi1->tType == T_SYMBOL || pi2->tType == T_INVALID || pi2->tType == T_SYMBOL)
        return false;

    if (pi1->tType != pi2->tType || pi1->aux != pi2->aux)
        return false;

    return operandsSame(pi1->iLhs, pi2->iLhs) && operandsSame(pi1->iRhs, pi2->iRhs);
}

/**************************************************************************
 29	sub_153d	ok++ (PMO)
 **************************************************************************/
void removeLabelRef(register sym_t *ps) {
    inst_t *ilist;

    if (!(ilist = ps->p.pInst))
        return;
    if (ilist->aux == 0)
        pr_error("Refc == 0");
    if (--ilist->aux != 0)
        return;
    removeInstruction(ilist);
    ps->p.pInst = NULL;
    freeSymbol(ps);
    logOptimise(O_UNREF_LAB); /* 6fc5 opt_msg[4] = "Unref'ed labels" */
}

/**************************************************************************
 30	sub_15ad	ok++	(PMO) Used in optimize
 * code optimised over original as noted below, otherwise identical
 * 1) for loop iteration expression moved
 * 2) code to increment an optiminse counter & set hasChanged = true shared
 * 3) sub_1d94() == 0, test removed as code falls through in either case
 **************************************************************************/
void sub_15ad() {
    for (gPi = root; gPi; gPi = gPi->pNext) {
        if (!sub_1795() && gPi->tType == T_JP && !sub_1aec() && !sub_1b86()) {
            if (gPi->iLhs->tType == T_CONST && gPi->iLhs->oPSym->p.pInst == gPi->pNext) {
                removeInstruction(gPi);
                logOptimise(O_JMP_TO_PLUS1); /* 6fc9 opt_msg[6] = "Jumps to .+1" */
            } else {
                if (gPi->aux == 0) { /* 1648 / 164B */
                    while (gPi->pNext->tType != T_INVALID && gPi->pNext->tType != T_SYMBOL) {
                        removeInstruction(gPi->pNext);
                        logOptimise(O_UNREACH_LAB); /* 6fc7 opt_msg[5] = "Unreachable code" */
                    }
                    if (gPi->iLhs->tType != T_REGREF && gPi->iLhs->oPSym->p.pInst && sub_1071(gPi))
                        continue;
                }
                if (gPi->iLhs->tType == T_CONST && (gPi->pNext->tType == T_CALL || gPi->pNext->tType == T_JP) &&
                    gPi->pNext->aux == 0 && gPi->iLhs->oPSym->p.pInst == gPi->pNext->pNext) {
                    gPi->pNext->aux = ccSwap[gPi->aux]; /* swap condition code */
                    removeInstruction(gPi);
                    logOptimise(O_SKIP_OVER_JMP); /* 6fcb opt_msg[7] = "Skips over jumps" */
                } else if (sub_1c67() || !sub_1d94())
                    ;
            }
        }
    }
}
/**************************************************************************
 31	sub_1795	ok++ (PMO)
 **************************************************************************/
bool sub_1795() {
    register inst_t *ilist;
    static int stackAdjust;

    if (gPi->tType != T_LD)
        return false;
    if (gPi->iLhs->tType != T_REG || gPi->iLhs->aux != REG_SP)
        return false;
    if ((ilist = gPi->pAlt)->tType != T_CADD || (ilist = ilist->pAlt)->tType != T_LD)
        return false;
    if (ilist->iLhs->tType != T_REG || ilist->iLhs->aux != REG_HL || ilist->iRhs->tType != T_CONST)
        return false;
    if (ilist->iRhs->oPOperand)
        pr_error("Funny stack adjustment");
    stackAdjust = ilist->iRhs->oVal;
    ilist          = gPi->pNext;
    if (ilist->tType == T_SIMPLE && ilist->aux == I_EXX)
        ilist = ilist->pNext;
    for (; ilist->tType != T_CALL && ilist->tType != T_JP && ilist->tType != T_STK && (ilist->tType != T_EX || ilist->iLhs->aux != REG_SP);
         ilist = ilist->pNext)
        ;

    if (stackAdjust > 0 && usesIXorIY && ilist->aux == 0 && sub_4000(ilist)) {
        removeInstruction(gPi->pAlt->pAlt);
        removeInstruction(gPi->pAlt);
        gPi = gPi->pAlt;
        removeInstruction(gPi->pNext);
        if (gPi->pNext->tType == T_SIMPLE && gPi->pNext->aux == I_EXX && gPi->tType == T_SIMPLE &&
            gPi->aux == I_EXX) { /* exx */
            removeInstruction(gPi->pNext);
            removeInstruction(gPi);
        }
        return logOptimise(O_STK_ADJUST); /* 6fc1 opt_msg[2] = "Stack adjustments" m7: */
    }
    ilist = gPi->pAlt->pAlt;
    if (stackAdjust < 0)
        stackAdjust = -stackAdjust;
    if (ilist->pAlt->tType == T_SIMPLE && ilist->pAlt->aux == I_EXX)
        stackAdjust -= 2;

    if (stackAdjust > 8 || ilist->iRhs->oVal < 0)
        return false;

    logOptimise(O_STK_ADJUST); /* 6fc1 opt_msg[2] = "Stack adjustments" */

    stackAdjust = ilist->iRhs->oVal;
    ilist          = ilist->pAlt;
    removeInstruction(ilist->pNext->pNext);
    removeInstruction(ilist->pNext);
    removeInstruction(gPi);
    gPi = ilist;

    while (stackAdjust != 0) {
        gPi             = allocInst(gPi);
        gPi->iLhs       = allocOperand();
        gPi->iLhs->tType = T_REG;
        if (1 < stackAdjust) {
            gPi->iLhs->aux = REG_BC;
            gPi->tType      = T_STK;
            stackAdjust -= 2;
            gPi->aux = I_POP; /* "pop" */
        } else {
            gPi->iLhs->aux = REG_SP;
            gPi->tType      = T_INCDEC; /* Decrement, Increment */
            --stackAdjust;
            gPi->aux = SI_INC;
        }
    }
    if (gPi->pNext->tType == T_SIMPLE && gPi->pNext->aux == I_EXX && ilist->tType == T_SIMPLE && ilist->aux == I_EXX) {
        removeInstruction(gPi->pNext);
        removeInstruction(ilist);
    }
    return true;
}

/**************************************************************************
 32	sub_1aec	ok++ (PMO)
 **************************************************************************/
bool sub_1aec() {
    register inst_t *ilist;

    if (gPi->iLhs->tType != T_REGREF) {
        if ((gPs = gPi->iLhs->oPSym)->p.pInst) {
            ilist = getNextRealInst(gPs->p.pInst);
            ilist = ilist->pAlt;
            if (gPs->p.pInst != ilist /*->i_7*/) {
                gPi->iLhs->oPOperand = ilist->iLhs;
                removeLabelRef(gPs);
                ++ilist->aux;                     /* safe const change */
                return logOptimise(O_RED_LAB); /* 6fbd opt_msg[0] = "Redundant labels" */
            }
        }
    }
    return false;
}

/**************************************************************************
 33	sub_1b86	ok++ (PMO)
 **************************************************************************/
bool sub_1b86() {
    register inst_t *ilist;

    if (gPi->tType == T_JP || gPi->tType == T_CALL) {
        if ((gPs = gPi->iLhs->oPSym) && (ilist = gPs->p.pInst)) {
            ilist = getNextRealInst(ilist);
            if (ilist->tType == T_JP && (ilist->aux == 0 || ilist->aux == gPi->aux) && ilist->iLhs->oPSym != gPs) {
                removeLabelRef(gPs);
                gPs              = ilist->iLhs->oPSym;
                gPi->iLhs->oPSym = gPs;
                if (gPs->p.pInst)
                    ++gPs->p.pInst->aux;
                return logOptimise(O_JMP_TO_JMP); /* 6fbf opt_msg[1] = "Jumps to jumps" */
            }
        }
    }

    return false;
}
/**************************************************************************
 34	sub_1c67	ok++ (PMO)
 **************************************************************************/
bool sub_1c67() {
    register inst_t *ilist;
    if (gPi->aux == 0) {
        for (ilist = gPi->pNext; ilist; ilist = ilist->pNext) {
            if (instructionsSame(ilist, gPi)) {
                seq1 = gPi;
                seq2 = ilist;
                /* match chains of instructions */
                while (instructionsSame(seq2->pAlt, seq1->pAlt)) {
                    seq1 = seq1->pAlt;
                    seq2 = seq2->pAlt;
                }
                if (seq1 != gPi) { /* we matched some common code */
                    seq1              = syntheticLabel(seq1->pAlt);
                    seq2              = allocInst(seq2->pAlt);
                    seq2->iLhs        = allocOperand();
                    seq2->tType        = T_JP;
                    seq2->iLhs->tType  = T_CONST;
                    seq2->iLhs->oPSym = seq1->iPSym;
                    ++seq1->aux;
                    removeInstruction(ilist);
                    return logOptimise(O_CMN_CODE_SEQ); /* 6fcd opt_msg[8] = "Common code seq's" */
                }
            }
        }
    }
    return false;
}

/**************************************************************************
 35	sub_1d94	ok++ (PMO)
 **************************************************************************/
bool sub_1d94() {
    register inst_t *ilist;

    if (gPi->aux != 0 && (ilist = gPi->iLhs->oPSym->p.pInst)) {
        for (seq1 = gPi; sub_4625(ilist->pNext) && instructionsSame(seq1->pNext, ilist->pNext); ilist = ilist->pNext) {
            HEAP(ilist->pNext);
            HEAP(seq1->pNext);
            seq1 = seq1->pNext;
        }
        if (seq1 != gPi) {
            if (ilist->pNext->tType == T_SYMBOL)
                ilist = ilist->pNext;
            else
                ilist = syntheticLabel(ilist);
            seq1                  = allocInst(seq1);
            seq1->iLhs            = allocOperand();
            seq1->iLhs->tType      = T_CONST;
            seq1->iLhs->oPOperand = ilist->iLhs;
            seq1->tType            = gPi->tType;
            seq1->aux             = gPi->aux;
            ++ilist->aux;
            removeInstruction(gPi);
            return logOptimise(O_CMN_CODE_SEQ); /* 6fcd opt_msg[8] = "Common code seq's" */
        }
    }
    return false;
}

/**************************************************************************
 36	sub_1ec1	ok++ (PMO)
 **************************************************************************/
void sub_1ec1() {
    int l1;
    sub_4601();
    for (gPi = root; gPi; gPi = gPi->pNext) {
        switch (gPi->tType) {
        case T_LD:
            if (!key_r) {
                if (sub_24c0())
                    continue;
                if (gPi->iLhs->tType == T_REG && gPi->iLhs->aux == REG_A && sub_4682(gPi->iRhs) &&
                    !sub_47e0(REG_F, gPi->pNext, gPi)) {
                    gPi->iRhs   = NULL;
                    gPi->tType   = T_3;
                    gPi->aux    = I_XOR; /* xor */
                    gPi->opCode = NULL;
                    logOptimise(O_XOR_A); /* 6fbf opt_msg[14] = "xor a's used" */
                }
            }
            break;
        case T_SYMBOL:
            for (; gPi->tType == T_SYMBOL && gPi->aux == 0; gPi = gPi->pNext) {
                removeInstruction(gPi);
                logOptimise(O_UNREF_LAB); /* 6fc5 opt_msg[4] = "Unref'ed labels" */
            }
            /* fall through */
        case T_TWOBYTE:
        case T_CALL:
            sub_4601();
            break;
        case T_EX:
            if (gPi->iLhs->tType == T_REG) {
                if (sub_23c1())
                    continue;
                swapHLDE();
            } else
                sub_4544(gPi->iRhs->aux);
            break;
        case T_SIMPLE:
            switch (gPi->aux) {
            case I_NOP:
            case I_SCF:
            case I_CCF:
            case I_HALT:
            case I_DI:
            case I_EI:
                break;
            case I_EXX:
                sub_4601();
                break;
            default:
                sub_4544(REG_AF);
                break;
            }
            break;
        case T_BIT:
            if (gPi->aux != 0x40)
                if (gPi->iRhs->tType == T_REG)
                    sub_4544(gPi->iRhs->aux);
                else if ((l1 = sub_46b1(gPi->iRhs, 0)) != -1)
                    sub_4544(l1);
            break;
        caseCommon:
        case T_CARR:
            if (gPi->iLhs->tType == T_REG && !sub_47e0(gPi->iLhs->aux, gPi->pNext, gPi)) {
                if ((gPi->tType == T_INCDEC && gPi->iLhs->aux >= REG_BC) || !sub_47e0(REG_F, gPi->pNext, gPi)) {
                    removeInstruction(gPi);
                    logOptimise(O_RED_OPS); /* 6fd3 opt_msg[11] = "Redundant operations" */
                    continue;
                }
            }

            /* fall through */ /* m25 */
        case T_SHIFT:
        case T_0xE:
            if (gPi->iLhs->tType == T_REG)
                sub_4544(gPi->iLhs->aux);
            else
                sub_44b2(gPi->iLhs);
            break;
        case T_CADD:
            if (!sub_47e0(gPi->iLhs->aux, gPi->pNext, gPi)) {
                removeInstruction(gPi);
                logOptimise(O_RED_OPS); /* 6fd3 opt_msg[11] = "Redundant operations" */
                continue;
            } else if (gPi->iRhs->tType == T_CONST && abs(gPi->iRhs->oVal) == 1 && !gPi->iRhs->oPOperand) {
                gPi->tType   = T_INCDEC;
                gPi->aux    = gPi->iRhs->oVal != 1 ? SI_DEC : SI_INC;
                gPi->opCode = NULL;
                freeOperand(gPi->iRhs);
                gPi->iRhs = NULL;
                logOptimise(O_SIMPLE_ADDR); /* 6fd7 opt_msg[13] = "Simplified addresses" */
            } else
                goto caseCommon;
            /* fall through */
        case T_INCDEC:
            if (gPi->iLhs->tType != T_REG)
                goto caseCommon;
            if (gPi->iLhs->aux != REG_HL)
                goto caseCommon;

            if (sub_2ef8())
                continue;
            break;
        case T_DJNZ:
            sub_4544(0);
            break;
        case T_STK:
            if (gPi->aux == I_PUSH) { /* push */
                if (gPi->iLhs->aux == REG_IY && (l1 = sub_46b1(&regValues[17], REG_IY)) != -1 && l1 != REG_IY) {
                    gPi->iLhs->aux = l1;
                    logOptimise(O_SIMPLE_ADDR); /* 6fd7 opt_msg[13] = "Simplified addresses" */
                }
                /* check for pop */
                /* OTIMISER: fails to optimise the gPi->pNext-aux below */
                if (gPi->pNext->tType == T_STK && gPi->pNext->aux == I_POP && gPi->iLhs->aux == gPi->pNext->iLhs->aux) {
                    removeInstruction(gPi->pNext);
                    removeInstruction(gPi);
                    logOptimise(O_RED_OPS); /* 6fd3 opt_msg[11] = "Redundant operations" */
                } else
                    break;
            } else {
                sub_4544(gPi->iLhs->aux); /* m39: */
                if (gPi->pAlt->tType == T_STK && gPi->pAlt->aux == I_PUSH) {
                    regValues[gPi->iLhs->aux] = regValues[gPi->pAlt->iLhs->aux];
                    if (gPi->iLhs->aux == REG_IY && gPi->pNext->tType == T_STK && gPi->pNext->iLhs->aux == REG_IY) {
                        gPi->pNext->iLhs->aux = gPi->pAlt->iLhs->aux;
                        logOptimise(O_SIMPLE_ADDR); /* 6fd7 opt_msg[13] = "Simplified addresses" */
                    }
                }
                break;
            }
            continue;
        case T_3:
            if (sub_2d3b())
                continue;
            break;
        }
        sub_2bdb();
    }
}

/**************************************************************************
 37	sub_23c1	ok++ (PMO)
 **************************************************************************/
bool sub_23c1() {
    register inst_t *ilist;

    if ((ilist = gPi->pNext)->tType == T_STK && ilist->aux == I_PUSH && ilist->iLhs->aux == gPi->iLhs->aux &&
        !sub_47e0(gPi->iLhs->aux, ilist->pNext, gPi) && !sub_47e0(gPi->iRhs->aux, ilist, gPi)) {
        ilist->iLhs->aux = gPi->iRhs->aux;
        removeInstruction(gPi);
        gPi = ilist;
    } else if (gPi->pNext->tType == T_EX && gPi->pNext->iLhs->tType == T_REG) {
        removeInstruction(gPi->pNext);
        removeInstruction(gPi);
    } else
        return false;
    return logOptimise(O_RED_EX_DEHL); /* 6fdb opt_msg[15] = "Redundant ex de,hl's" */
}

/**************************************************************************
 38	sub_24c0	ok++ (PMO)
 **************************************************************************/
bool sub_24c0() {
    inst_t const *l1;
    operand_t *l2;
    int reg;

    if (sub_3053())
        return true;
    if (gPi->iLhs->tType == T_REG &&
        (gPi->iRhs->tType == T_INDEXED || gPi->iRhs->tType == T_ADDRREF || gPi->iRhs->tType == T_CONST)) {
        if (sub_29c3())
            return true;
    } else if (gPi->iRhs->tType == T_CONST && gPi->iLhs->tType == T_INDEXED) {
        if ((reg = sub_46b1(gPi->iRhs, REG_A)) != -1) {
            gPi->iRhs->tType = T_REG; /* m6: */
            gPi->iRhs->aux  = reg;
            return logOptimise(O_SIMPLE_ADDR); /* 6fd7 opt_msg[13] = "Simplified addresses" */
        }
        sub_44b2(gPi->iLhs);
    } else if (gPi->iRhs->tType == T_REG && (gPi->iLhs->tType == T_INDEXED || gPi->iLhs->tType == T_ADDRREF)) {
        if (operandsSame(gPi->iLhs, &regValues[gPi->iRhs->aux])) {
        kill:
            removeInstruction(gPi);
            return logOptimise(O_RED_LD); /* 6fd5 opt_msg[12] = "Redundant loads/stores" */
        }
        sub_44b2(gPi->iLhs);
        if (regValues[gPi->iRhs->aux].tType == T_INVALID) {
            sub_4544(gPi->iRhs->aux);
            regValues[gPi->iRhs->aux] = *gPi->iLhs;
        }

    } else if (gPi->iLhs->tType == T_REG && gPi->iRhs->tType == T_REG) { /* 2824 */
        if (gPi->iLhs->aux == gPi->iRhs->aux || operandsSame(&regValues[gPi->iLhs->aux], &regValues[gPi->iRhs->aux])) {
            goto kill;
        }

        if ((seq2 = gPi->pNext)->tType == T_LD && operandsSame(seq2->iLhs, gPi->iRhs) &&
            operandsSame(seq2->iRhs, gPi->iLhs)) {
            removeInstruction(seq2);
            logOptimise(O_RED_LD); /* 6fd5 opt_msg[12] = "Redundant loads/stores" */
        }
        if (!sub_47e0(gPi->iLhs->aux, gPi->pNext, gPi))
            goto kill;
        if (gPi->iLhs->aux == REG_E && gPi->iRhs->aux == REG_L && (seq2 = gPi->pNext)->tType == T_LD &&
            seq2->iLhs->tType == T_REG && seq2->iLhs->aux == REG_D && seq2->iRhs->tType == T_REG &&
            seq2->iRhs->aux == REG_H && !sub_47e0(REG_HL, seq2->pNext, gPi)) {
            removeInstruction(gPi->pNext);
            gPi->tType      = T_EX;
            gPi->opCode    = NULL;
            gPi->iLhs->aux = REG_DE;
            gPi->iRhs->aux = REG_HL;
            swapHLDE();
            return logOptimise(O_SIMPLE_ADDR); /* 6fd7 opt_msg[13] = "Simplified addresses" */
        }
        sub_4544(gPi->iLhs->aux);
        regValues[gPi->iLhs->aux] = regValues[gPi->iRhs->aux];
    } else if (gPi->iLhs->tType == T_REG)
        sub_4544(gPi->iLhs->aux);

    if (gPi->iLhs->tType == T_REG && gPi->iRhs->tType == T_REG) {
        if ((l1 = gPi->pAlt)->tType == T_LD && (l2 = l1->iLhs)->tType == T_REG && l2->aux == gPi->iRhs->aux &&
            !sub_47e0(l2->aux, gPi->pNext, l1) && sub_1369(l1->iRhs)) {
            sub_4544(l2->aux);
            regValues[l2->aux = gPi->iLhs->aux] = *l1->iRhs;
            goto kill;
        }
        if ((l1 = gPi->pNext)->tType == T_LD && (l2 = l1->iRhs)->tType == T_REG && l2->aux == gPi->iLhs->aux &&
            !sub_47e0(l2->aux, l1->pNext, gPi) && sub_1369(l1->iLhs)) {
            sub_4544(l2->aux);
            l2->aux = gPi->iRhs->aux;
            goto kill;
        }
    }
    return false; /* m26: */
}

/**************************************************************************
 39	sub_29c3	ok++ (PMO)
 Code is identical, except that the optimiser moves the code in the first if block
 to a different location. The jp conditions are changed to reflect this)
 **************************************************************************/
bool sub_29c3() {
    int l1;
    inst_t *pi1;

    if (operandsSame(gPi->iRhs, &regValues[gPi->iLhs->aux]) || !sub_47e0(gPi->iLhs->aux, gPi->pNext, gPi)) {
        /* OPTIMISER: this block is located differently */
        removeInstruction(gPi);
        return logOptimise(O_RED_LD); /* 6fd5 opt_msg[12] = "Redundant loads/stores" */
    }
    sub_4544(gPi->iLhs->aux);
    if (gPi->iLhs->aux <= REG_HL) {
        if ((l1 = sub_46b1(gPi->iRhs, gPi->iLhs->aux)) != -1) {
            /* code hikes gPi->iLhs->aux before test !!! */
            regValues[gPi->iLhs->aux] = *gPi->iRhs;
            gPi                       = gPi;
            gPi->iRhs->tType           = T_REG;
            gPi->iRhs->aux            = l1;
            if (gPi->iLhs->aux >= REG_BC) { /* goto m5; */
                pi1             = allocInst(gPi);
                pi1->iLhs       = allocOperand();
                pi1->iRhs       = allocOperand();
                pi1->iLhs->tType = T_REG;
                pi1->iRhs->tType = T_REG;
                pi1->iLhs->aux  = regHiLoMap[gPi->iLhs->aux].hiReg;
                pi1->iRhs->aux  = regHiLoMap[gPi->iRhs->aux].hiReg;
                gPi->iLhs->aux  = regHiLoMap[gPi->iLhs->aux].loReg;
                gPi->iRhs->aux  = regHiLoMap[gPi->iRhs->aux].loReg;
                pi1->tType       = T_LD; /* OP_LD */
                gPi             = pi1;
            }
            return logOptimise(O_SIMPLE_ADDR); /* 6fd7 opt_msg[13] = "Simplified addresses" */
        }
    }
    regValues[gPi->iLhs->aux] = *gPi->iRhs;
    return false;
}

/**************************************************************************
 40	sub_2bdb	ok++ (PMO)
 **************************************************************************/

bool sub_2bdb() {
    register inst_t *ilist;
    if (gPi->tType == T_STK && gPi->iLhs->aux == REG_BC && gPi->aux == I_POP && !sub_47e0(REG_BC, gPi->pNext, gPi)) {
        for (ilist = gPi->pNext; ilist; ilist = ilist->pNext) {
            if (ilist->tType == T_JP || ilist->tType == T_CALL || ilist->tType == T_SYMBOL)
                break;
            if (ilist->tType == T_STK)
                if (ilist->aux != I_PUSH || ilist->pNext->tType != T_STK || ilist->pNext->aux != I_POP)
                    break;
                else
                    ilist = ilist->pNext;
            if (ilist->tType == T_EX || (ilist->tType == T_LD && ilist->iLhs->tType == T_REG))
                if (ilist->iLhs->aux == REG_SP)
                    break;
        }
        if (ilist->tType == T_STK && ilist->aux == I_PUSH && ilist->iLhs->aux == REG_HL && !sub_47e0(REG_HL, ilist->pNext, ilist)) {
            removeInstruction(gPi);
            ilist->opCode     = NULL;
            ilist->tType       = T_EX;
            ilist->aux        = 0;
            ilist->iRhs       = ilist->iLhs;
            ilist->iLhs       = allocOperand();
            ilist->iLhs->tType = T_REGREF;
            ilist->iLhs->aux  = REG_SP;
            return logOptimise(O_EX_SPHL); /* 6fd1 opt_msg[10] = "Ex (sp),hl'pi used" */
        }
    }
    return false;
}

/**************************************************************************
 41	sub_2d3b	ok++ (PMO)
 six of the basic blocks are in different positions but the code in the blocks
 is the same and the logic flow is maintained.
 **************************************************************************/

bool sub_2d3b() {
    register operand_t *po;

    switch (gPi->aux) {
    default:
        if (sub_4682(gPi->iLhs))
            gPi->aux = I_OR;
        else if ((po = gPi->iLhs)->tType != T_REG || po->aux != REG_A) {
            sub_4544(REG_AF);
            return false;
        } else
            break;
        /* fall through */
    case I_OR:
        po = gPi->iLhs; /* case 0xB0 */
        if (sub_4682(po)) {
            po->tType = T_REG;
            po->aux  = REG_A;
        }
        /* fall through */
    case I_SUB:
    caseP_SUB: /* common */
        if (gPi->iLhs->tType == T_REG && gPi->iLhs->aux == REG_A) {
            if (gPi->aux == I_SUB)
                break;
            else if (!sub_47e0(REG_F, gPi->pNext, gPi)) {
                removeInstruction(gPi);
                return logOptimise(O_RED_OPS); /* 6fd3 opt_msg[11] = "Redundant operations" */
            }
            return false;
        }
        sub_4544(REG_AF);
        return false;
    case I_AND:
        po = gPi->iLhs;
        if (sub_4682(po))
            break;
        if (po->tType == T_CONST && !po->oPOperand && (po->oVal & 0xff) == 255 /* -1 */) {
            gPi->aux = I_OR;
            po->tType = T_REG;
            po->aux  = REG_A;
        }
        goto caseP_SUB;

    case I_CP:
        return false; /* case 0xB8 */
    }

    if (!sub_47e0(REG_F, gPi->pNext, gPi) && sub_4682(&regValues[REG_A])) {
        removeInstruction(gPi);
        return logOptimise(O_RED_OPS); /* 6fd3 opt_msg[11] = "Redundant operations" */
    }
    gPi->aux                   = I_XOR;
    gPi->opCode                = NULL;
    gPi->iLhs->tType            = T_REG;
    gPi->iLhs->aux             = REG_A;
    regValues[REG_A].tType      = T_CONST;
    regValues[REG_A].oPOperand = NULL;
    regValues[REG_A].oVal      = 0;
    return false;
}

/**************************************************************************
 42	sub_2ef8	ok++ (PMO)
 **************************************************************************/
bool sub_2ef8() {
    operand_t tmp;
    int l2;

    if (gPi->pNext->tType == T_INCDEC && operandsSame(gPi->iLhs, gPi->pNext->iLhs) && gPi->pNext->aux != gPi->aux)
        removeInstruction(gPi->pNext);

    else if (sub_47e0(REG_HL, gPi->pNext, gPi)) {
        tmp = regValues[REG_TRACKER];
        l2  = hlDelta;
        if (regValues[REG_HL].tType != T_INVALID ||
            (regValues[REG_L].tType == T_INDEXED && regValues[REG_H].tType == T_INDEXED &&
             regValues[REG_L].aux == regValues[REG_H].aux && regValues[REG_L].oVal + 1 == regValues[REG_H].oVal)) {
            if (regValues[REG_HL].tType != T_INVALID)
                tmp = regValues[REG_HL];
            else
                tmp = regValues[REG_L];
            l2 = 0;
        }
        sub_4544(REG_HL);
        regValues[REG_TRACKER] = tmp;
        hlDelta                = l2;
        if (gPi->aux == SI_INC)
            ++hlDelta;
        else
            --hlDelta;
        return false;
    }
    removeInstruction(gPi);
    return logOptimise(O_RED_OPS); /* 6fd3 opt_msg[11] = "Redundant operations" */
}

/**************************************************************************
 42a	sub_3053	ok++ (PMO)
 **************************************************************************/
bool sub_3053() {
    int l1;
    int l2;

    if (gPi->iLhs->tType != T_REG || ((l1 = gPi->iLhs->aux) != REG_HL && l1 != REG_L))
        return false;

    if (!operandsSame(gPi->iRhs, &regValues[REG_TRACKER]))
        return false;

    l2 = hlDelta;
    if (gPi->iLhs->aux == REG_L) {
        if ((seq2 = gPi->pNext)->tType != T_LD || seq2->iLhs->tType != T_REG || seq2->iLhs->aux != REG_H ||
            seq2->iRhs->oVal != gPi->iRhs->oVal + 1)
            return false;

        sub_4544(REG_HL);
        regValues[REG_L] = *gPi->iLhs;
        regValues[REG_H] = *seq2->iLhs;
        removeInstruction(seq2);
    } else {
        sub_4544(REG_HL);
        regValues[REG_HL] = *gPi->iLhs;
    }
    gPi = gPi->pAlt; /* m4: */
    removeInstruction(gPi->pNext);
    while (l2 != 0) {
        gPi             = allocInst(gPi);
        gPi->tType       = T_INCDEC;
        gPi->iLhs       = allocOperand();
        gPi->iLhs->tType = T_REG;
        gPi->iLhs->aux  = REG_HL;
        if (l2 < 0) {
            ++l2;
            gPi->aux = SI_INC;
        } else {
            --l2;
            gPi->aux = SI_DEC;
        }
    }
    return logOptimise(O_RED_LD); /* 6fd5 opt_msg[12] = "Redundant loads/stores" */
}

/**************************************************************************
 43	sub_31ee	ok++ (PMO)
 **************************************************************************/
void swapHLDE() {
    operand_t ilist;

    ilist                          = regValues[REG_HL];
    regValues[REG_HL]           = regValues[REG_DE];
    regValues[REG_DE]           = ilist;

    ilist                          = regValues[REG_H];
    regValues[REG_H]            = regValues[REG_D];
    regValues[REG_D]            = ilist;

    ilist                          = regValues[REG_L];
    regValues[REG_L]            = regValues[REG_E];
    regValues[REG_E]            = ilist;
    regValues[REG_TRACKER].tType = T_INVALID;
    ;
};

/**************************************************************************
 44	pr_psect	sub_328a	ok++ (PMO)
 **************************************************************************/
void pr_psect(int psect) {

    if (psect == cur_psect)
        return;
    printf("psect\t%s\n", psectNames[psect]);
    cur_psect = psect;
}

/**************************************************************************
 45	num_psect	sub_32bf	ok++ (PMO)
 **************************************************************************/
int num_psect(char const *fmt) {
    int l1;

    for (l1 = 0; l1 < 4; ++l1) {
        if (strcmp(fmt, psectNames[l1]) == 0)
            return l1;
    }
    pr_error("Unknown psect");
}

/**************************************************************************
 46	sub_3313	ok++ (PMO) apart from optimiser changes
 1) one code optimisation to remove jp, putting condition code on call
 2) code block moved with optimisation on shared code
 3) two further code block group moves
 Note the other code is identical and overall the logic is the same
 **************************************************************************/
typedef struct {
    int tType; /* 01	s->str */
    int prec; /* 23	s->i_2 */
} op_t;

term_t *evalExpr() {
    char expectOp;
    op_t *pOp;
    term_t *pTerm;
    term_t termStack[30];
    op_t opStack[30];
    static term_t exprResult; /* so pointer to term can be returned */

    pTerm     = &termStack[30];
    pOp       = &opStack[29];
    pOp->tType = T_MARKER;
    pOp->prec = 0;
    expectOp  = false;
    do {                                 /* REDUCE loop */
        for (;; tokType = get_token()) { /* SHIFT loop */
            if (tokType == T_STRING) {   /* in expressions "A" is treated as ascii value of A */
                if (strlen(yylval.pChar) != 1)
                    pr_warning("Bad character const");
                yylval.i = *yylval.pChar;
                tokType  = T_CONST;
            }
            if (T_FWD <= tokType && tokType <= T_CONST) { /* get the term, note two terms together is an error */
                if (expectOp)
                    exp_err();
                expectOp = true; /* flag as expect operator next */
                --pTerm;         /* where to push the term */
                switch (tokType) {
                case T_SYMBOL: /* its reocatable */
                    pTerm->tPSym = yylval.pSym;
                    pTerm->val   = 0;
                    break;
                case T_CONST: /* its a constant */
                    pTerm->val   = yylval.i;
                    pTerm->tPSym = NULL;
                    break;
                }
                continue;
            } else if (T_UPLUS <= tokType && tokType <= T_LASTOP) {           /* get the operator */
                if (!expectOp && (tokType == T_PLUS || tokType == T_MINUS)) { /* map unary +/- */
                    tokType  = tokType - 7;
                    yylval.i = 8; /* set its precedence */
                }
                if (tokType <= T_MARKER) {
                    if (expectOp)
                        exp_err();
                } else {
                    if (!expectOp && tokType != T_OPAR) /* binary op only when expecting op*/
                        exp_err();
                    if (pOp->prec >= yylval.i) /* pick up precedence */
                        break;
                }
                if (tokType != T_CPAR) { /* not a closing ) */
                    --pOp;
                    pOp->tType = tokType;   /* push its type */
                    if (tokType == T_OPAR) /* if it was a ( then set prec to 1 */
                        yylval.i = 1;
                    else                                     /* OPTIMISER[2]: code moved to here */
                        expectOp = false;                    /* now expecting a term */
                    pOp->prec = yylval.i; /* set the prec */ /* OPTIMISER[3] code block moved / shared */
                } else if (pOp->tType ==
                           T_MARKER) /* ) with nothing on stack */ /* OPTIMISER[4]: test code block moved */
                    break;
            } else
                break;
        }

        /* REDUCE phase */
        if (pOp->tType == T_OPAR) { /* check for matching () */
            if (tokType != T_CPAR)
                exp_err();          /* ")" */
            tokType  = get_token(); /* prime for next part */
            expectOp = 1;           /* assuming its a term */
        } else if (T_MARKER >= pOp->tType)
            uconv(pOp->tType, pTerm); /* calculate unary operator */
        else {
            bconv(pOp->tType, pTerm + 1, pTerm); /* calculate binary operator*/
            ++pTerm;
        }
    } while (++pOp != &opStack[30]); /* loop till end of operator stack */

    exprResult = *pTerm;
    if (&termStack[29] != pTerm) /* oops outstanding term */
        exp_err();
    return &exprResult;
} /**************************************************************************
 47	exp_err		sub_32bf	ok++ (PMO)
 **************************************************************************/
void exp_err() {

    pr_error("Expression error");
}

/**************************************************************************
 48	sub_359e	ok++ (PMO)
 *	Unary Operators
 **************************************************************************/
void uconv(int op, term_t *lhs) {

    switch (op) {
    case T_UMINUS:
        lhs->val = -lhs->val;
        break;
    case T_NOT:
        lhs->val = ~lhs->val;
        break;
    case T_HI:
        lhs->val = (lhs->val >> 8) & 0xff;
        break;
    case T_LOW:
        lhs->val &= 0xff;
        break;
    case T_UPLUS:
        return;
    case T_MARKER:
        return;
    default:
        pr_error("uconv - bad op");
    }
    if (lhs->tPSym)
        rel_err();
    return;
}

/**************************************************************************
 49	sub_3630	ok++ (PMO)
 *	Binary Operators
 **************************************************************************/
void bconv(int op, term_t *lhs, term_t const *rhs) {

    switch (op) {
    case T_PLUS:
        if (lhs->tPSym && rhs->tPSym)
            rel_err();
        lhs->val += rhs->val;
        if (!lhs->tPSym)
            lhs->tPSym = rhs->tPSym;
        return;
    case T_MINUS:
        if (rhs->tPSym)
            rel_err();
        lhs->val -= rhs->val;
        return;
    case T_MUL:
        lhs->val *= rhs->val;
        break;
    case T_DIV:
        lhs->val /= rhs->val;
        break;
    case T_MOD:
        lhs->val %= rhs->val;
        break;
    case T_SHR:
        lhs->val >>= rhs->val;
        break;
    case T_SHL:
        lhs->val <<= rhs->val;
        break;
    case T_AND:
        lhs->val &= rhs->val;
        break;
    case T_OR:
        lhs->val |= rhs->val;
        break;
    case T_XOR:
        lhs->val ^= rhs->val;
        break;
    case T_EQ:
        lhs->val = lhs->val == rhs->val;
        break;
    case T_LE:
        lhs->val = rhs->val < lhs->val;
        break;
    case T_GE:
        lhs->val = lhs->val < rhs->val;
        break;
    case T_ULE:
        lhs->val = (uint16_t)rhs->val < (uint16_t)lhs->val;
        break;
    case T_UGE:
        lhs->val = (uint16_t)lhs->val < (uint16_t)rhs->val;
        break;
    default:
        pr_error("Bconv - bad op");
        break;
    }

    if (lhs->tPSym || rhs->tPSym)
        rel_err();
}

/**************************************************************************
 50	rel_err		ok++ (PMO)
 **************************************************************************/
void rel_err() {

    pr_warning("Relocation error");
}

/**************************************************************************
 51	sub_3856	ok++ (PMO) except for one jp z, that is optimised to jr z,
 **************************************************************************/
operand_t *evalOperand() {
    register operand_t *oper;

    oper = allocOperand();

    switch (tokType) {
    case T_EOL:
        oper->tType = T_INVALID;
        break;

    case T_REG:
        if (expectCond && yylval.i == REG_C) { /* check if C is used in jp c context */
            tokType  = T_COND;                 /* convert to condition */
            yylval.i = COND_LLT;
        }
        /* fall through */
    case T_COND:
        oper->tType = tokType; /* save type, value and prep for next token */
        oper->aux  = (uint8_t)yylval.i;
        tokType    = get_token();
        break;

    case T_OPAR:
        if ((tokType = get_token()) == T_REG) {
            if (yylval.i != REG_C && yylval.i < REG_BC) /* only (C) and (BC) onwards are valid */
                oper_err();
            oper->tType = T_REGREF;
            oper->aux  = (uint8_t)yylval.i;          /* save reg id */
            if ((tokType = get_token()) == T_CPAR) { /* simple (reg) so prep for next token */
                tokType = get_token();
                break;
            }
            if (oper->aux < REG_IX) /* only IX & IY are allowed expressions */
                oper_err();
            oper->tType = T_INDEXED; /* is IX/IY +/- d operand */
        } else
            oper->tType = T_ADDRREF; /* is a (addr) operand */

        oper->term = *evalExpr(); /* get the expression */

        if (tokType != T_CPAR) /* should now be the closing ) */
            oper_err();
        tokType = get_token(); /* prep for next token */
        /* IX & IY are only allowed displacements -128 to 127 */
        if (oper->tType == T_INDEXED && (oper->oVal < -128 || oper->oVal >= 128))
            pr_warning("Index offset too large");
        break;
    /*
        the operands below are only valid as the last operand on a line
        so there is no preping for the next token
    */
    case T_FWD:
    case T_BWD:
        oper->tType = tokType;
        oper->oVal = yylval.i; /* save the label reference number */
        break;
    default:
        oper->tType = T_CONST;
        oper->term = *evalExpr(); /* allow for more complex expression */
    }
    return oper;
}
/**************************************************************************
 52	oper_err	ok++ (PMO) but note jmpbuf not set
 **************************************************************************/
void oper_err() {

    pr_warning("Operand error");
    longjmp(jmpbuf, 1); /* BUG: jmpbuf not set */
}

/**************************************************************************
 53	sub_39a3	ok++ (PMO)
 **************************************************************************/
void getOperands(register inst_t *ilist) {

    tokType    = get_token();
    cntOperand = 0;
    ilist->iLhs   = NULL;
    ilist->iRhs   = NULL;
    if (tokType == T_COMM)
        oper_err();         /* cannot start with a comma */
    if (tokType != T_EOL) { /* no operands */
        ilist->iLhs = evalOperand();
        if (tokType == T_COMM) { /* if comma then 2nd operand */
            tokType  = get_token();
            ilist->iRhs = evalOperand();
            ++cntOperand;
        }
        ++cntOperand;
    }
    clr_len_inbuf();
}

/**************************************************************************
 54	sub_3a15	ok++ (PMO)
 **************************************************************************/

void loadFunction() {
    sym_t *ps;           /* ??? */
    inst_t *l2;          /* ok */
    inst_t *l3;          /* ok */
    int fpBase;          /* ??? */
    register inst_t *ilist; /* ok */

    ilist = root = (inst_t *)alloc_mem(sizeof(inst_t));
    ilist->pNext = (inst_t *)alloc_mem(sizeof(inst_t));
    HEAP(ilist->pNext);
    ilist->pNext->pAlt = ilist;
    ilist              = ilist->pNext;
    l2 = switchVectors = word_6fee = (inst_t *)alloc_mem(sizeof(inst_t));

    for (;; HEAP(ilist->iRhs)) {
        tokType = get_token();
        HEAP(ilist->iRhs);
        for (;; HEAP(ilist->pNext)) {
            if (tokType == T_EOL) {
                clr_len_inbuf();
                break;
            }

            if (ilist->tType != T_INVALID)
                ilist = allocInst(ilist); /* m3: */
            if (tokType == -1) {    /* m4: */
                word_6ffc = ilist;
                word_6fee = l2;
                return;
            }
            ilist->tType = tokType; /* m5: */
            if (psect == SWDATA) {
                if (tokType == T_DEFW) { /* collect the switch table */
                    ilist->opCode = yytext;
                    getOperands(ilist);
                    l2->pNext       = ilist;
                    ilist              = ilist->pAlt;
                    ilist->pNext       = NULL;
                    l2->pNext->pAlt = l2;
                    l2              = l2->pNext;
                    break;
                }
                psect = DATA; /* revert to normal data handling */
            }
            switch (ilist->tType = tokType) { /* m7: */
            case T_CONST:
                if ((psect == DATA) || (psect == BSS)) {
                    ilist->tType = T_INVALID;
                    pr_psect(psect);
                    printf("%d:\n", yylval.i);
                } else {
                    ilist->aux = yylval.i; /* m10: */
                    l3      = ilist->pAlt;
                    if (ilist->pAlt->tType == T_JP && l3->iLhs->tType == T_FWD && l3->iLhs->oVal == ilist->aux)
                        removeInstruction(l3);
                }
                tokType = get_token(); /* m11: */
                if (tokType != T_COLN) /* ":" */
                    pr_error("Bad temp label");
                break;

            case T_SYMBOL:
                ps      = yylval.pSym;
                ilist->aux = 0;
                tokType = get_token();
                if (tokType == T_EQU) {
                    if (ps->label[0] != 'f') /* compiler generated equ names begin with f */
                        pr_error("Unknown EQU");

                    ilist->tType = T_INVALID;
                    tokType  = get_token();
                    ilist->iLhs = evalOperand();
                    /* check is constant with no unresolved symbol ref */
                    if (ilist->iLhs->tType != T_CONST || ilist->iLhs->oPSym)
                        pr_error("Bad arg to EQU");

                    fpBase    = ilist->iLhs->oVal; /* the frame pointer offset to lowest local (will be 0 or -ve) */

                    word_6ffc = ilist;
                    word_6fee = l2;
                    ilist        = root;

                    do { /* update any references to the frame size */
                        if (ilist->iRhs && ilist->iRhs->tType == T_CONST && ilist->iRhs->oPSym == ps) {
                            ilist->iRhs->oVal += fpBase;
                            ilist->iRhs->oPSym = NULL;
                        }
                        if (ilist->iLhs && ilist->iLhs->tType == T_CONST && ilist->iLhs->oPSym == ps) {
                            ilist->iLhs->oVal += fpBase;
                            ilist->iLhs->oPSym = NULL;
                        }
                    } while (ilist = ilist->pNext);
                    return;
                }
                ilist->iPSym   = ps;

                ps->p.pInst = ilist;

                ilist->aux     = INT_MAX;
                if (psect == DATA && ps->label[0] == 'S') { /* compiler generated switch tables start with S */
                    psect           = SWDATA;
                    l2->pNext       = ilist;
                    ilist              = ilist->pAlt;
                    ilist->pNext       = NULL;
                    l2->pNext->pAlt = l2;
                    l2              = l2->pNext;
                }
                if (psect == DATA || psect == BSS) {
                    ilist->tType    = T_INVALID;
                    ilist->iLhs    = NULL;
                    ps->p.pInst = NULL;
                    pr_psect(psect);
                    printf("%s:\n", ps->label);
                } else if (ps->label[0] == '_') /* external name */
                    name_fun = ps->label;

                if (tokType == T_COLN)
                    break;
                continue; /* inner loop */

            case 255: /* -1 */
                ilist->tType  = T_INVALID;
                word_6ffc = ilist;
                return;

            case T_DEFW:
            case T_DEFB:
                if (psect == TEXT)
                    goto case_default;
            case T_DEFM:
            case T_DEFS:
            case T_DEFF:
                if (psect == TEXT)
                    pr_error("DEF[BMSF] in text psect");
                pr_psect(psect);
                /* fall through */
            case T_GLB:
                printf("%s\t%s\n", yytext, ptr_token());
                ilist->tType = T_INVALID;
                break;

            case T_PSCT:
                psect    = num_psect(ptr_token()); /* m30: */
                ilist->tType = T_INVALID;
                break;

            case T_JR:
                ilist->tType = T_JP; /* convert to jp so it is safe to move code */
                yytext   = "jp";
                /* fall through */
            default:
            case_default:
                ilist->opCode = yytext;
                ilist->aux    = yylval.i;
                if (tokType == T_JP || tokType == T_CALL) /* set if can have conditional */
                    expectCond = true;
                else
                    expectCond = false;
                getOperands(ilist);
                if ((ilist->tType == T_JP) || (ilist->tType == T_CALL)) {
                    if (ilist->iLhs->tType == T_COND) { /* if cond then hoist condition and remove lhs */
                        ilist->aux  = ilist->iLhs->aux;
                        ilist->iLhs = ilist->iRhs;
                        ilist->iRhs = NULL;
                    }
                }
                if (ilist->tType == T_JP && ilist->aux == 0 && ilist->iLhs->tType != T_REGREF &&
                    (l3 = ilist->pAlt)->tType == T_CONST && l3->pAlt->tType == T_JP && l3->pAlt->aux == 0) {

                    while (l3 = l3->pAlt) {
                        if (l3->tType == T_JP && l3->iLhs->tType == T_FWD && l3->iLhs->oVal == ilist->pAlt->aux)
                            *l3->iLhs = *ilist->iLhs;
                        else if (l3->tType == T_CONST && l3->aux == ilist->pAlt->aux)
                            break;
                    }
                    removeInstruction(ilist->pAlt);
                    freeOperand(ilist->iLhs);
                    ilist->tType   = T_INVALID;
                    ilist->iLhs   = NULL;
                    ilist->opCode = NULL;
                }
                break;
            }

            break; /* to outer loop */
        }
    }
}

/**************************************************************************
 55	sub_4000	ok++ (PMO)
 **************************************************************************/
bool sub_4000(register inst_t const *ilist) {

    return ilist->tType == T_JP && ilist->iLhs->oPSym && strcmp(ilist->iLhs->oPSym->label, "cret") == 0;
}

/**************************************************************************
 56	sub_404d	ok++ (PMO)	Used in: optimise
 **************************************************************************/
void sub_404d() {

    register inst_t *ilist;

    if (root->pNext) {
        pr_psect(TEXT);
        for (ilist = root->pNext; ilist; ilist = ilist->pNext) {
            /* 22-08-22 fix guard against calls to absolute locations */
            if (ilist->tType == T_CALL && ilist->iLhs->oPSym && strcmp(ilist->iLhs->oPSym->label, "ncsv") == 0) {
                ilist = ilist->pNext;
                if (ilist->tType != T_DEFW) /* "defw" */
                    pr_error("Expecting defw after call ncsv");
                if (ilist->iLhs->oVal == 0) {
                    if (usesIXorIY)
                        printf("global csv\ncall csv\n");
                } else {
                    usesIXorIY = true;
                    if (ilist->iLhs->oVal >= -4) {
                        printf("global csv\ncall csv\npush hl\n");
                        if (ilist->iLhs->oVal < -2)
                            printf("push hl\n");
                    } else
                        printf("call ncsv\ndefw %d\n", ilist->iLhs->oVal);
                }
            } else if (!usesIXorIY && sub_4000(ilist)) {
                ilist->tType   = T_RET;
                ilist->opCode = NULL;
                pr_instruction(ilist);
            } else if (!usesIXorIY && ilist->tType == T_CALL && ilist->aux == 0 && ilist->pNext->aux == 0 &&
                       sub_4000(ilist->pNext) && ilist->iLhs->oPSym->label[0] == '_') {
                ilist->tType   = T_JP; /* "jp" */
                ilist->opCode = NULL;
                pr_instruction(ilist);
                ilist = ilist->pNext;
            } else
                pr_instruction(ilist);
        }
    }
    if (switchVectors->pNext) {
        pr_psect(DATA);
        for (ilist = switchVectors->pNext; ilist; ilist = ilist->pNext)
            pr_instruction(ilist);
    }
}

/**************************************************************************
 57	sub_420a	ok++ (PMO)
 benign differences
 1) printf call/return code is shared
 2) fputc('\n', stdout) is located differently
 **************************************************************************/

void pr_instruction(register inst_t *ilist) {

    if (ilist->tType == T_INVALID)
        ;
    else if (ilist->tType == T_SYMBOL) {
        if ((ilist->iPSym->label[0]))
            printf("%s:\n", ilist->iPSym->label);
        else
            printf("L%d:\n", ilist->iSymId);
    } else if (ilist->tType == T_CONST) { /* m4: */
        printf("%d:\n", ilist->aux);     /* OPTIMISER[1]: shares printf call with above*/
    } else if (key_f && ilist->tType == T_CALL && strcmp(ilist->iLhs->oPSym->label, "csv") == 0) {
        printf("push\tiy\npush\tix\nld\tix,0\nadd\tix,sp\n");
    } else {
        if (key_n)
            fputc('\t', stdout); /* m7: */

        pr_token(ilist);

        if (ilist->tType == T_JP || ilist->tType == T_CALL || ilist->tType == T_RET) {
            fputc('\t', stdout);
            if (ilist->aux != 0)
                printf("%s", conditions[ilist->aux]);
            if (ilist->tType != T_RET) {
                if (ilist->aux != 0)
                    fputc(',', stdout);
                sub_436e(ilist->iLhs); /* m11: */
            }
        } else if (ilist->iLhs) { /* m14: */
            fputc('\t', stdout);
            sub_436e(ilist->iLhs);
            if (ilist->iRhs) {
                fputc(',', stdout);
                sub_436e(ilist->iRhs);
            }
        }
        fputc('\n', stdout); /* OPTIMISER: minor movement in where this is located also optimises return */
    }
}

/**************************************************************************
 58	sub_436e	ok++ (PMO)
 Same except optimiser misses the optimisation of fputc(')', stdout)
 **************************************************************************/
void sub_436e(register operand_t const *ilist) {
    HEAP(ilist);
    switch (ilist->tType) {
    case T_INDEXED:
    case T_ADDRREF:
        fputc('(', stdout);
        if (ilist->tType == T_INDEXED) {
            if (ilist->aux == REG_IX)
                printf("ix");
            else
                printf("iy");
            fputc('+', stdout);
        }
    case T_CONST:
        if (ilist->oPSym) {
            if (ilist->oPSym->label[0])
                printf("%s", ilist->oPSym->label);
            else
                printf("L%d", ilist->oPSym->p.pInst->iSymId);
            if (0 < ilist->oVal)
                fputc('+', stdout);
        }
        if (ilist->oVal != 0 || !ilist->oPSym)
            printf("%d", ilist->oVal);
        if (ilist->tType != T_CONST)
            fputc(')', stdout);
        break;
    case T_REGREF:
        fputc('(', stdout);
    case T_REG:
        printf("%s", regs[ilist->aux]);
        if (ilist->tType == T_REGREF)
            fputc(')', stdout); /* OPTIMISER[1]: misses optimising htis with same fputc above */
        break;
    case T_FWD:
        printf("%df", ilist->oVal);
        break;
    default:
        pr_error("Bad operand");
        break;
    }
    HEAP(ilist);
}

/**************************************************************************
 59	sub_44b2	ok++ (PMO)
 Optimiser saves some code with minor changes to code for regValues[REG_TRACKER]

 original                   replacement
 ld a,(regValues + 6ch)     ld  hl,regvalues + 6ch  ; test regValues[REG_TRACKER].type
 or a                       ld  a,(hl)
 jp z,cret                  or  a
 push   iy                  jp  z,cret
 ld hl,regVales + 6ch       push    iy
 push   hl                  push    hl              ; &regValues[REG_TRACKER]

 The replacement code is slightly short
 **************************************************************************/
void sub_44b2(register operand_t const *po) {
    int n;

    while ((n = sub_46b1(po, REG_B)) != -1)
        sub_4544(n);
    while ((n = sub_46b1(po, REG_BC)) != -1)
        sub_4544(n);

    if (regValues[17].tType && operandsSame(po, &regValues[17]))
        sub_4544(17);

    if (regValues[REG_TRACKER].tType && operandsSame(&regValues[REG_TRACKER], po))
        regValues[REG_TRACKER].tType = T_INVALID;
}

/**************************************************************************
 60	sub_4544	ok++ (PMO)
 *	Optimiser generates marginally less efficient code for regValues[REG_TRACKER] access
 * it chooses to use hl and (hl) rather than the load directly to/from a
 * code functionally is the same
 **************************************************************************/
void sub_4544(int reg) {
    register operand_t *ilist;

    regValues[reg].tType = T_INVALID;
    if (regTestMasks[REG_HL] & regTestMasks[reg]) {
        hlDelta                     = 0;
        regValues[REG_TRACKER].tType = T_INVALID;
    }
    if (ilist = regHiLoValMap[reg].pHiRegVal) {
        ilist->tType = T_INVALID;
        if (ilist = regHiLoValMap[reg].pLoRegVal)
            ilist->tType = T_INVALID;
    }
    if (reg != 17)
        return;
    for (ilist = regValues; ilist < &regValues[REG_TRACKER]; ++ilist)
        if (ilist->tType == T_INDEXED && ilist->aux == REG_IY)
            ilist->tType = T_INVALID;

    if (regValues[REG_TRACKER].tType == T_INDEXED && regValues[REG_TRACKER].aux == REG_IY)
        regValues[REG_TRACKER].tType = T_INVALID;
}

/**************************************************************************
 61	sub_4601	ok+ (PMO)
 **************************************************************************/
void sub_4601() {
    register operand_t *po;

    for (po = regValues; po < &regValues[REG_TRACKER]; ++po)
        po->tType = T_INVALID;
    regValues[REG_TRACKER].tType = T_INVALID;
}

/**************************************************************************
 62	sub_4625	ok++ (PMO)
 **************************************************************************/
bool sub_4625(register inst_t const *ilist) {

    switch (ilist->tType) {
    case T_LD:
    case T_STK:
        return true;
    case T_INCDEC:
        return ilist->iLhs->tType == T_REG && ilist->iLhs->aux >= REG_BC;
    case T_EX:
        return ilist->iLhs->aux != REG_AF;
    }
    return false;
}

/**************************************************************************
 63	sub_4682	ok++ (PMO)
 **************************************************************************/
bool sub_4682(register operand_t const *ilist) {

    return ilist->tType == T_CONST && !ilist->oPSym && ilist->oVal == 0;
}

/**************************************************************************
 64	sub_46b1	ok++ (PMO)
 **************************************************************************/
int sub_46b1(register operand_t const *opr, int reg) {
    operand_t *po;
    int i;

    po = reg < REG_BC ? &regValues[REG_B] : &regValues[REG_BC];
    do {
        if (po->tType) {
            if (operandsSame(po, opr)) {
                i = (int)(po - regValues);
                if (i >= REG_BC && reg < REG_BC)
                    return regHiLoMap[i].loReg;
                return i;
            }
        }
    } while (++po < &regValues[REG_SP]);
    return -1;
}

/**************************************************************************
 65	sub_475c	ok++ (PMO)
 **************************************************************************/
int sub_475c(register operand_t const *po, int p2) {
    if (!po)
        return false;
    if (po->tType != T_REG && po->tType != T_INDEXED && po->tType != T_REGREF)
        return false;
    if (p2 & regTestMasks[po->aux])
        return true;
    return false;
}

/**************************************************************************
 66	sub_47a2	ok++
 **************************************************************************/
int sub_47a2(register operand_t const *po, int p2) {
    if (!po)
        return false;
    if (po->tType != T_REG)
        return false;
    if (p2 & regTestMasks[po->aux])
        return true;
    return false;
}

/**************************************************************************
 67	sub_47e0	ok++ (PMO)
 *	Optimiser differences
 * 1) the code for case 0 is detected as the same as the code in case T_JP
 *    and reused
 * 2) there are two instances where the test cases are reversed along with
 *    the associated jumps. Leaving the logic the same
 **************************************************************************/
bool sub_47e0(int reg, register inst_t const *pi1, inst_t const *pi2) {
    uint16_t msk;
    sym_t *ps;
    inst_t *l3;
    int n; /* number of iterations */

    if (reg == REG_SP || reg == REG_IX)
        return true;

    if (REG_BC <= reg && reg <= REG_HL) {
        if (sub_47e0(regHiLoMap[reg].hiReg, pi1, pi2))
            return true;
        reg = regHiLoMap[reg].loReg;
    }
    if (reg >= sizeof(regTestMasks) / sizeof(regTestMasks[0]))
        fprintf(stderr, "%d\n", reg);
    /* #pragma warning(suppress : 6385) /* reg has limited values */
    msk = regTestMasks[reg]; /* m3: */
    n   = 40;

    do {
        switch (pi1->tType) {

        case T_CALL: /* "call" */
            if (pi1->aux != 0 && (msk & 0x40))
                return true;

            if (reg == REG_IY)
                break;
            if (!pi1->iLhs->oPSym || pi1->iLhs->oPSym->label[0] == '_')
                return false;

            if (!(msk & 0xBF))
                return false;
            else
                return true;

        case T_JP: /* "jp" */
            if (pi1->aux != 0 || pi1->iLhs->tType == T_REGREF || !(ps = pi1->iLhs->oPSym))
                return true;
            if (!(l3 = ps->p.pInst))
                if (strcmp(ps->label, "cret") != 0)
                    return true;
                else if (msk & 0x3C) /* code reused */
                    return true;
                else
                    return false;
            pi1 = l3;
            break;

        case T_SIMPLE:          /* 0x1 */
            switch (pi1->aux) { /* m14: */
            case 0:             /* "nop" */
            case I_CPL:         /* "cpl" */
            case I_SCF:         /* "scf" */
            case I_CCF:         /* "ccf" */
            case I_NEG:         /* "neg" */
            case I_HALT:
            case I_DI: /* "di" */
            case I_EI:
                break;
            case I_EXX:
                while ((pi1 = pi1->pNext) && (pi1->tType != T_SIMPLE || pi1->aux != I_EXX))
                    if (pi1->tType != T_LD && pi1->tType != T_STK && pi1->tType != T_CADD)
                        return false;
                if (!pi1)
                    return false;
                break;
            default:
                if (msk & 0x80)
                    return true;
                break;
            }
            break;
        case T_TWOBYTE: /* 0x2 */
            if (reg != 17)
                return true;
            break;

        case T_3:
            if (pi1->aux == I_XOR) /* "xor"				//m22: */
                if (pi1->iLhs->tType == T_REG && pi1->iLhs->aux == REG_A && reg == REG_A)
                    return false;
            if (msk & 0x80)
                return true; /* m23: */
            if (msk & 0x40)
                return false;
            if (sub_475c(pi1->iLhs, msk)) /* OPTIMISER[1]: see below */
                return true;
            break;

        case T_SHIFT:
            if ((pi1->aux & 0xFFE7) != 0x20 && (msk & 0x40))
                return true;

        case T_INCDEC: /* 0x4 "dec", "inc" */
            if ((msk & 0x40) && (pi1->iLhs->tType != T_REG || pi1->iLhs->aux < REG_BC))
                return false;
            if (pi1->iLhs->tType == T_REG || sub_475c(pi1->iLhs, msk))
                return true;
            break;

        case T_BIT: /* "set", "res", "bit" */
            if (pi1->aux == 0x40 && (msk & 0x40))
                return false;

        case T_0xF: /* 0xF */
            if (sub_475c(pi1->iRhs, msk) || sub_475c(pi1->iLhs, msk))
                return true;
            break;

        case T_5:
            break;

        case T_DJNZ: /* "djnz" */
            if (msk & 1)
                return true;
            break;

        case T_0xE: /* 0xE */
            if (sub_475c(pi1->iRhs, msk))
                return true;
            if (sub_47a2(pi1->iLhs, msk))
                return false;
            break;

        case T_STK:
            if (pi1->aux == I_PUSH && (msk & regTestMasks[pi1->iLhs->aux]))
                return true;
            if (msk & regTestMasks[pi1->iLhs->aux])
                return false;
            break;

        case T_EX: /* "ex" */
            if (pi1->iLhs->tType == T_REGREF && (msk & regTestMasks[pi1->iRhs->aux]))
                return true;
            if (msk & 0x3C)
                return true;
            break;

        case T_CADD: /* CADD */
            if (msk & 0x40)
                return false;

        case T_CARR: /* Add, sub with Carry */
            if ((regTestMasks[pi1->iLhs->aux] | 0x40) & msk)
                return true;
            if (sub_475c(pi1->iRhs, msk))
                return true;
            break;
        case T_LD: /* load */
            if (!operandsSame(pi1->iLhs, pi1->iRhs))
                if (sub_475c(pi1->iRhs, msk))
                    return true;
                else if (sub_47a2(pi1->iLhs, msk))
                    return false;
                else if (sub_475c(pi1->iLhs, msk))
                    return true;
            break;

        case 0: /* OPTIMISER[2]: optimised to reuse code in T_JP */
            if (msk & 0x3C)
                return true;
            else
                return false;
        case T_JR: /* 0x9 "jr" */
        case T_RET:
        case T_RST: /* 0xD "rst" */
            break;
        }
        pi1 = pi1->pNext;
        if (pi2 == pi1)
            return false;
    } while (n-- != 0);
    return true;
}

#define HASHSIZE 311
sym_t *hashtab[HASHSIZE]; /* 7086	[622] */

/**************************************************************************
 68	sub_4c33	ok++ (PMO)	Used in: sub_4cf0, sub_4da7
 **************************************************************************/
sym_t *allocItem() {
    register sym_t *ps;

    ps = (sym_t *)freeItemList; /* check the free list*/
    if (ps) {                   /* if there is an entry release it*/
        freeItemList = ((list_t *)ps)->pNext;
        ps->label    = NULL;
        ps->p.pInst  = NULL;
        return ps;
    }
    return alloc_mem(sizeof(sym_t)); /* else allocate a new one */
}

/**************************************************************************
 69	sub_4c6b	ok++ (PMO)
 **************************************************************************/

void freeSymbol(register sym_t *ps) {

    if (strlen(ps->label) >= sizeof(sym_t)) { /* if string could be reused as a symbol at it to the free list*/
        ((list_t *)(ps->label))->pNext = freeItemList;
        freeItemList                   = (list_t *)(ps->label);
    }
    ((list_t *)ps)->pNext = freeItemList; /* add the symbol to the free list */
    freeItemList          = (list_t *)ps;
}

/**************************************************************************
 70	hash_index	sub_4cab	ok++ (PMO)	Used in: sub_4cf0
 **************************************************************************/
int hash_index(register char const *s, int size) {
    uint16_t hash;

    for (hash = 0; *s != 0; ++s)
        hash += *(uint8_t *)s + hash;
    return hash % size;
}

/**************************************************************************
 71	sub_4cf0	ok++ (PMO)			Used in: get_token
 **************************************************************************/

sym_t *lookupSym(register char const *s) {
    sym_t **pps;
    sym_t *ps;

    pps = &hashtab[hash_index(s, HASHSIZE)];
    while (*pps && strcmp((*pps)->label, s))
        if (++pps == &hashtab[HASHSIZE])
            pps = hashtab;
    if (ps = *pps)
        return ps;
    *pps = ps = allocItem();
    ps->label = alloc_mem((int)strlen(s) + 1);
    strcpy(ps->label, s);
    return ps;
}

/**************************************************************************
 72	sub_4da7	ok++ (PMO)
 **************************************************************************/
sym_t *allocBlankSym() {
    register sym_t *ps;

    ps        = allocItem();
    ps->label = "";
    return ps;
}

/**************************************************************************
 73	sub_4dbf	ok++ (PMO)			Used in: optimise
 **************************************************************************/
void resetHeap() {
#ifdef CPM
    int *p;
#endif
    if (programBreak == 0)
        programBreak = sbrk(0); /* current base of heap */
    else
        brk(programBreak); /* reset the heap */
    alloct = allocs = programBreak;
#ifdef CPM
    for (p = (int *)hashtab; p < (int *)&hashtab[HASHSIZE];)
        *p++ = 0;
#else
    memset(hashtab, 0, sizeof(hashtab));
#endif
    freeItemList = NULL;
}

/**************************************************************************
 74	sub_4e20	ok++	 (PMO)		Used in: optimise
 **************************************************************************/
void freeHashtab() { /* not realy needed for systems with large memory */
#ifdef CPM
    allocs = (char *)&hashtab;           /* */
    alloct = (char *)&hashtab[HASHSIZE]; /* */
#endif
}

/**************************************************************************
 75	alloc_mem	sub_4e2d	ok++ (PMO)
 Optimiser differences.
 1) res instruction is used to clear lsb rather than and 0xfe
 2) de is loaded before the size calculation. As it is not used before
    the code does the same thing
 **************************************************************************/
#ifdef CPM
#define CHUNK 512
#else
#define CHUNK 8192 /* larger chunk to avoid waste as allocaiton sizes are larger */
#endif
void *alloc_mem(int size) {
#ifdef CPM
    char *p;
#endif
    register char *ilist;
    if ((size = (size + 1) & ~1) + allocs > alloct) {
        if ((allocs = sbrk(CHUNK)) == (char *)-1)
            pr_error("Out of memory in %s", name_fun);
        alloct = sbrk(0);
    }

    ilist = allocs;
    allocs += size;
#ifdef CPM
    for (p = pi; size-- != 0;)
        *p++ = 0; /* Clearing allocated memory area */
#else
    memset(ilist, 0, size);
#endif
    return ilist;
}

/* simple sbrk & brk implementations */
#ifndef CPM

#define MAXHEAP (0x8000 * sizeof(char *))
static char *heapBase;
static char *heapTop;

void *sbrk(int size) {
    if (!heapBase && !(heapTop = heapBase = malloc(MAXHEAP)))
        fprintf(stderr, "can't allocate heap!!\n");
    if (heapTop + size >= heapBase + MAXHEAP)
        return (void *)-1;
    heapTop += size;
    return heapTop - size;
}

int brk(void *p) {
    if (heapBase <= (char *)p && (char *)p < heapBase + MAXHEAP) {
        heapTop = p;
        return 0;
    }
    return -1;
};

void heapchk(void const *p) {
    if (p && (p < (void *)heapBase || p >= (void *)heapTop) &&
        (p < (void *)hashtab || p >= (void *)&hashtab[HASHSIZE])) {
        fprintf(stderr, "out of range heap item\n");
    }
}
#endif

/**************************************************************************

#######				####### ######  #######   ###   #     #
#        #    #  #####		#     # #     #    #       #    ##   ##
#        ##   #  #    #		#     # #     #    #       #    # # # #
#####    # #  #  #    #		#     # ######     #       #    #  #  #
#        #  # #  #    #		#     # #          #       #    #     #
#        #   ##  #    #		#     # #          #       #    #     #
#######  #    #  #####		####### #          #      ###   #     #

*/

