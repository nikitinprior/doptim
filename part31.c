/*
 * File part31.c created 14.08.2021, last modified 11.11.2021.
 *
 * The part31.c file is part of the restored optimization program
 * from the Hi-Tech C compiler v3.09 package.
 *
 *	Andrey Nikitin & Mark Ogden 11.11.2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "optim1.h"

int regTestMasks[] = {
    0x01,                              		/* 6cb2 REG_B   */
    0x02,                              		/* 6cb4 REG_C   */
    0x04,                              		/* 6cb6 REG_D   */
    0x08,                              		/* 6cb8 REG_E   */
    0x10,                              		/* 6cba REG_H   */
    0x20,                              		/* 6cbc REG_L   */
    0x40,                              		/* 6cbe REG_F   */
    0x80,                              		/* 6cc0 REG_A   */
    0x00,                              		/* 6cc2 REG_I   */
    0x00,                              		/* 6cc4 REG_R   */
    0x03,                              		/* 6cc6 REG_BC  */
    0x0C,                              		/* 6cc8 REG_DE  */
    0x30,                              		/* 6cca REG_HL  */
    0x00,                              		/* 6ccc REG_SP  */
    0xC0,                              		/* 6cce REG_AF  */
    0x00,                              		/* 6cd0 REG_AF1 */
    0x100,                             		/* 6cd2 REG_IX  */
    0x200,                             		/* 6cd4 REG_IY  */
};

/**************************************************************************
 50 rel_err     ok++
 **************************************************************************/
void rel_err() {

    pr_warning("Relocation error");
}

/**************************************************************************
 51 sub_3856    ok++ (PMO) except for one jp z, that is optimised to jr z,
 **************************************************************************/
operand_t *evalOperand() {
    register operand_t *oper;

    oper = allocOperand();

    switch (tokType) {
        case T_EOL:
            oper->type = T_INVALID;
            break;

        case T_REG:
            if (expectCond && yylval.i == REG_C) { /* check if C is used in jp c context */
                tokType  = T_COND;                 /* convert to condition */
                yylval.i = COND_LLT;
            }
        /* fall through */
        case T_COND:
            oper->type = tokType; /* save type, value and prep for next token */
            oper->aux  = (uint8_t)yylval.i;
            tokType    = get_token();
            break;

        case T_OPAR:
            if ((tokType = get_token()) == T_REG) {
                if (yylval.i != REG_C && yylval.i < REG_BC) { /* only (C) and (BC) onwards are valid */
                    oper_err();
                }
                oper->type = T_REGREF;
                oper->aux  = (uint8_t)yylval.i;          /* save reg id */
                if ((tokType = get_token()) == T_CPAR) { /* simple (reg) so prep for next token */
                    tokType = get_token();
                    break;
                }
                if (oper->aux < REG_IX) { /* only IX & IY are allowed expressions */
                    oper_err();
                }
                oper->type = T_INDEXED; /* is IX/IY +/- d operand */
            } else {
                oper->type = T_ADDRREF;    /* is a (addr) operand */
            }

            oper->term = *evalExpr(); /* get the expression */

            if (tokType != T_CPAR) { /* should now be the closing ) */
                oper_err();
            }
            tokType = get_token(); /* prep for next token */
            /* IX & IY are only allowed displacements -128 to 127 */
            if (oper->type == T_INDEXED && (oper->oVal < -128 || oper->oVal >= 128)) {
                pr_warning("Index offset too large");
            }
            break;
        /*
            the operands below are only valid as the last operand on a line
            so there is no preping for the next token
        */
        case T_FWD:
        case T_BWD:
            oper->type = tokType;
            oper->oVal = yylval.i; /* save the label reference number */
            break;
        default:
            oper->type = T_CONST;
            oper->term = *evalExpr(); /* allow for more complex expression */
    }
    return oper;
}

/**************************************************************************
 52 oper_err    ok++ (PMO) but note jmpbuf not set
 **************************************************************************/
void oper_err() {

    pr_warning("Operand error");
    longjmp(jmpbuf, 1); /* BUG: jmpbuf not set */
}

/**************************************************************************
 53 sub_39a3    ok++ (PMO)
 **************************************************************************/
void getOperands(register inst_t *pi) {

    tokType    = get_token();
    cntOperand = 0;
    pi->iLhs   = NULL;
    pi->iRhs   = NULL;
    if (tokType == T_COMM) {
        oper_err();			/* cannot start with a comma */
    }
    if (tokType != T_EOL) {		/* no operands */
        pi->iLhs = evalOperand();
        if (tokType == T_COMM) {	/* if comma then 2nd operand */
            tokType  = get_token();
            pi->iRhs = evalOperand();
            ++cntOperand;
        }
        ++cntOperand;
    }
    clr_len_inbuf();
}

/**************************************************************************
 54 sub_3a15    ok++ (PMO)
 **************************************************************************/
void loadFunction() {
    sym_t  *ps;
    inst_t *l2;
    inst_t *l3;
    int     fpBase;
    register inst_t *pi;

    pi = root = (inst_t *)alloc_mem(sizeof(inst_t));
    pi->pNext = (inst_t *)alloc_mem(sizeof(inst_t));
    HEAP(pi->pNext);
    pi->pNext->pAlt = pi;
    pi              = pi->pNext;
    l2 = switchVectors = word_6fee = (inst_t *)alloc_mem(sizeof(inst_t));

    for (;; HEAP(pi->iRhs)) {
        tokType = get_token();
        HEAP(pi->iRhs);
        for (;; HEAP(pi->pNext)) {
            if (tokType == T_EOL) {
                clr_len_inbuf();
                break;
            }

            if (pi->type != T_INVALID) {
                pi = allocInst(pi);    /* m3: */
            }
            if (tokType == -1) {    /* m4: */
                word_6ffc = pi;
                word_6fee = l2;
                return;
            }
            pi->type = tokType; /* m5: */
            if (psect == SWDATA) {
                if (tokType == T_DEFW) { /* collect the switch table */
                    pi->opCode = yytext;
                    getOperands(pi);
                    l2->pNext       = pi;
                    pi              = pi->pAlt;
                    pi->pNext       = NULL;
                    l2->pNext->pAlt = l2;
                    l2              = l2->pNext;
                    break;
                }
                psect = DATA; /* revert to normal data handling */
            }
            switch (pi->type = tokType) { /* m7: */
                case T_CONST:
                    if ((psect == DATA) || (psect == BSS)) {
                        pi->type = T_INVALID;
                        pr_psect(psect);
                        printf("%d:\n", yylval.i);
                    } else {
                        pi->aux = yylval.i; /* m10: */
                        l3      = pi->pAlt;
                        if (pi->pAlt->type == T_JP && l3->iLhs->type == T_FWD && l3->iLhs->oVal == pi->aux) {
                            removeInstruction(l3);
                        }
                    }
                    tokType = get_token(); /* m11: */
                    if (tokType != T_COLN) {
                        pr_error("Bad temp label");
                    }
                    break;

                case T_SYMBOL:
                    ps      = yylval.pSym;
                    pi->aux = 0;
                    tokType = get_token();
                    if (tokType == T_EQU) {
                        if (ps->label[0] != 'f') { /* compiler generated equ names begin with f */
                            pr_error("Unknown EQU");
                        }

                        pi->type = T_INVALID;
                        tokType  = get_token();
                        pi->iLhs = evalOperand();
                        /* check is constant with no unresolved symbol ref */
                        if (pi->iLhs->type != T_CONST || pi->iLhs->oPSym) {
                            pr_error("Bad arg to EQU");
                        }

                        fpBase    = pi->iLhs->oVal; /* the frame pointer offset to lowest local (will be 0 or -ve) */

                        word_6ffc = pi;
                        word_6fee = l2;
                        pi        = root;

                        do { /* update any references to the frame size */
                            if (pi->iRhs && pi->iRhs->type == T_CONST && pi->iRhs->oPSym == ps) {
                                pi->iRhs->oVal += fpBase;
                                pi->iRhs->oPSym = NULL;
                            }
                            if (pi->iLhs && pi->iLhs->type == T_CONST && pi->iLhs->oPSym == ps) {
                                pi->iLhs->oVal += fpBase;
                                pi->iLhs->oPSym = NULL;
                            }
                        } while (pi = pi->pNext);
                        return;
                    }
                    pi->iPSym   = ps;

                    ps->p.pInst = pi;

                    pi->aux     = INT_MAX;
                    if (psect == DATA && ps->label[0] == 'S') { /* compiler generated switch tables start with S */
                        psect           = SWDATA;
                        l2->pNext       = pi;
                        pi              = pi->pAlt;
                        pi->pNext       = NULL;
                        l2->pNext->pAlt = l2;
                        l2              = l2->pNext;
                    }
                    if (psect == DATA || psect == BSS) {
                        pi->type    = T_INVALID;
                        pi->iLhs    = NULL;
                        ps->p.pInst = NULL;
                        pr_psect(psect);
                        printf("%s:\n", ps->label);
                    } else if (ps->label[0] == '_') { /* external name */
                        name_fun = ps->label;
                    }

                    if (tokType == T_COLN) {
                        break;
                    }
                    continue; /* inner loop */

                case 255: /* -1 */
                    pi->type  = T_INVALID;
                    word_6ffc = pi;
                    return;

                case T_DEFW:
                case T_DEFB:
                    if (psect == TEXT) {
                        goto case_default;
                    }
                case T_DEFM:
                case T_DEFS:
                case T_DEFF:
                    if (psect == TEXT) {
                        pr_error("DEF[BMSF] in text psect");
                    }
                    pr_psect(psect);
                /* fall through */
                case T_GLB:
                    printf("%s\t%s\n", yytext, ptr_token());
                    pi->type = T_INVALID;
                    break;

                case T_PSCT:
                    psect    = num_psect(ptr_token()); /* m30: */
                    pi->type = T_INVALID;
                    break;

                case T_JR:
                    pi->type = T_JP; /* convert to jp so it is safe to move code */
                    yytext   = "jp";
                /* fall through */
                default:
case_default:
                    pi->opCode = yytext;
                    pi->aux    = yylval.i;
                    if (tokType == T_JP || tokType == T_CALL) { /* set if can have conditional */
                        expectCond = true;
                    } else {
                        expectCond = false;
                    }
                    getOperands(pi);
                    if ((pi->type == T_JP) || (pi->type == T_CALL)) {
                        if (pi->iLhs->type == T_COND) { /* if cond then hoist condition and remove lhs */
                            pi->aux  = pi->iLhs->aux;
                            pi->iLhs = pi->iRhs;
                            pi->iRhs = NULL;
                        }
                    }
                    if (pi->type == T_JP && pi->aux == 0 && pi->iLhs->type != T_REGREF &&
                            (l3 = pi->pAlt)->type == T_CONST && l3->pAlt->type == T_JP && l3->pAlt->aux == 0) {

                        while (l3 = l3->pAlt) {
                            if (l3->type == T_JP && l3->iLhs->type == T_FWD && l3->iLhs->oVal == pi->pAlt->aux) {
                                *l3->iLhs = *pi->iLhs;
                            } else if (l3->type == T_CONST && l3->aux == pi->pAlt->aux) {
                                break;
                            }
                        }
                        removeInstruction(pi->pAlt);
                        freeOperand(pi->iLhs);
                        pi->type   = T_INVALID;
                        pi->iLhs   = NULL;
                        pi->opCode = NULL;
                    }
                    break;
            }
            break; /* to outer loop */
        }
    }
}

/**************************************************************************
 55 sub_4000    ok++ (PMO)
 **************************************************************************/
bool sub_4000(register inst_t const *pi) {

    return pi->type == T_JP && pi->iLhs->oPSym && strcmp(pi->iLhs->oPSym->label, "cret") == 0;
}

/**************************************************************************
 56 sub_404d    ok++ (PMO)  Used in: optimise
 **************************************************************************/
void sub_404d() {

    register inst_t *pi;

    if (root->pNext) {
        pr_psect(TEXT);
        for (pi = root->pNext; pi; pi = pi->pNext) {
            if (pi->type == T_CALL && strcmp(pi->iLhs->oPSym->label, "ncsv") == 0) {
                pi = pi->pNext;
                if (pi->type != T_DEFW) { /* "defw" */
                    pr_error("Expecting defw after call ncsv");
                }
                if (pi->iLhs->oVal == 0) {
                    if (usesIXorIY) {
                        printf("global csv\ncall csv\n");
                    }
                } else {
                    usesIXorIY = true;
                    if (pi->iLhs->oVal >= -4) {
                        printf("global csv\ncall csv\npush hl\n");
                        if (pi->iLhs->oVal < -2) {
                            printf("push hl\n");
                        }
                    } else {
                        printf("call ncsv\ndefw %d\n", pi->iLhs->oVal);
                    }
                }
            } else if (!usesIXorIY && sub_4000(pi)) {
                pi->type   = T_RET;
                pi->opCode = NULL;
                pr_instruction(pi);
            } else if (!usesIXorIY && pi->type == T_CALL && pi->aux == 0 && pi->pNext->aux == 0 &&
                       sub_4000(pi->pNext) && pi->iLhs->oPSym->label[0] == '_') {
                pi->type   = T_JP; /* "jp" */
                pi->opCode = NULL;
                pr_instruction(pi);
                pi = pi->pNext;
            } else {
                pr_instruction(pi);
            }
        }
    }
    if (switchVectors->pNext) {
        pr_psect(DATA);
        for (pi = switchVectors->pNext; pi; pi = pi->pNext) {
            pr_instruction(pi);
        }
    }
}

/**************************************************************************
 57 pr_instruction	sub_420a    ok++ (PMO)
 *
 * benign differences
 *   1) printf call/return code is shared
 *   2) fputc('\n', stdout) is located differently
 **************************************************************************/
void pr_instruction(register inst_t *pi) {

    if (pi->type == T_INVALID)
        ;
    else if (pi->type == T_SYMBOL) {
        if ((pi->iPSym->label[0])) {
            printf("%s:\n", pi->iPSym->label);
        } else {
            printf("L%d:\n", pi->iSymId);
        }
    } else if (pi->type == T_CONST) {		/* m4: */
        printf("%d:\n", pi->aux);     /* OPTIMISER[1]: shares printf call with above*/
    } else if (key_f && pi->type == T_CALL && strcmp(pi->iLhs->oPSym->label, "csv") == 0) {
        printf("push\tiy\npush\tix\nld\tix,0\nadd\tix,sp\n");
    } else {
        if (key_n) {
            fputc('\t', stdout);		/* m7: */
        }

        pr_token(pi);

        if (pi->type == T_JP || pi->type == T_CALL || pi->type == T_RET) {
            fputc('\t', stdout);
            if (pi->aux != 0) {
                printf("%s", conditions[pi->aux]);
            }
            if (pi->type != T_RET) {
                if (pi->aux != 0) {
                    fputc(',', stdout);
                }
                sub_436e(pi->iLhs);		/* m11: */
            }
        } else if (pi->iLhs) {			/* m14: */
            fputc('\t', stdout);
            sub_436e(pi->iLhs);
            if (pi->iRhs) {
                fputc(',', stdout);
                sub_436e(pi->iRhs);
            }
        }
        fputc('\n', stdout); /* OPTIMISER: minor movement in where this is located also optimises return */
    }
}

/**************************************************************************
 58 sub_436e    ok++ (PMO)
 Same except optimiser misses the optimisation of fputc(')', stdout)
 **************************************************************************/
void sub_436e(register operand_t const *pi) {

    HEAP(pi);
    switch (pi->type) {
        case T_INDEXED:
        case T_ADDRREF:
            fputc('(', stdout);
            if (pi->type == T_INDEXED) {
                if (pi->aux == REG_IX) {
                    printf("ix");
                } else {
                    printf("iy");
                }
                fputc('+', stdout);
            }
        case T_CONST:
            if (pi->oPSym) {
                if (pi->oPSym->label[0]) {
                    printf("%s", pi->oPSym->label);
                } else {
                    printf("L%d", pi->oPSym->p.pInst->iSymId);
                }
                if (0 < pi->oVal) {
                    fputc('+', stdout);
                }
            }
            if (pi->oVal != 0 || !pi->oPSym) {
                printf("%d", pi->oVal);
            }
            if (pi->type != T_CONST) {
                fputc(')', stdout);
            }
            break;
        case T_REGREF:
            fputc('(', stdout);
        case T_REG:
            printf("%s", regs[pi->aux]);
            if (pi->type == T_REGREF) {
                fputc(')', stdout);    /* OPTIMISER[1]: misses optimising htis with same fputc above */
            }
            break;
        case T_FWD:
            printf("%df", pi->oVal);
            break;
        default:
            pr_error("Bad operand");
            break;
    }
    HEAP(pi);
}

/**************************************************************************
 59 sub_44b2    ok++ (PMO)
 Optimiser saves some code with minor changes to code for regValues[18]

 original                   replacement
 ld a,(regValues + 6ch)     ld  hl,regvalues + 6ch  ; test regValues[18].type
 or a                       ld  a,(hl)
 jp z,cret                  or  a
 push   iy                  jp  z,cret
 ld hl,regVales + 6ch       push    iy
 push   hl                  push    hl              ; &regValues[18]

 The replacement code is slightly short
 **************************************************************************/
void sub_44b2(register operand_t const *po) {
    int n;

    while ((n = sub_46b1(po, REG_B)) != -1) {
        sub_4544(n);
    }
    while ((n = sub_46b1(po, REG_BC)) != -1) {
        sub_4544(n);
    }

    if (regValues[17].type && operandsSame(po, &regValues[17])) {
        sub_4544(17);
    }

    if (regValues[REG_TRACKER].type && operandsSame(&regValues[REG_TRACKER], po)) {
        regValues[REG_TRACKER].type = T_INVALID;
    }
}

/**************************************************************************
 60 sub_4544    ok++ (PMO)
 *
 * Optimiser generates marginally less efficient code for regValues[18]
 * access it chooses to use hl and (hl) rather than the load directly
 * to/from a code functionally is the same
 **************************************************************************/
void sub_4544(int reg) {
    register operand_t *pi;

    regValues[reg].type = T_INVALID;
    if (regTestMasks[REG_HL] & regTestMasks[reg]) {
        hlDelta                     = 0;
        regValues[REG_TRACKER].type = T_INVALID;
    }
    if (pi = regHiLoValMap[reg].pHiRegVal) {
        pi->type = T_INVALID;
        if (pi = regHiLoValMap[reg].pLoRegVal) {
            pi->type = T_INVALID;
        }
    }
    if (reg != 17) {
        return;
    }
    for (pi = regValues; pi < &regValues[REG_TRACKER]; ++pi)
        if (pi->type == T_INDEXED && pi->aux == REG_IY) {
            pi->type = T_INVALID;
        }

    if (regValues[REG_TRACKER].type == T_INDEXED && regValues[REG_TRACKER].aux == REG_IY) {
        regValues[REG_TRACKER].type = T_INVALID;
    }
}

/**************************************************************************
 61 sub_4601    ok+
 *
 *  Generates correct code, but different from original
 **************************************************************************/
void sub_4601() {
    register operand_t *po;

    for (po = regValues; po < &regValues[REG_TRACKER]; ++po) {
        po->type = T_INVALID;
    }
    regValues[REG_TRACKER].type = T_INVALID;
}

/**************************************************************************
 62 sub_4625    ok+
 *
 *  Generates correct code, but in a sequence different from original
 **************************************************************************/
bool sub_4625(register inst_t const *pi) {

    switch (pi->type) {
        case T_LD:
        case T_STK:
            return true;
        case T_INCDEC:
            return pi->iLhs->type == T_REG && pi->iLhs->aux >= REG_BC;
        case T_EX:
            return pi->iLhs->aux != REG_AF;
    }
    return false;
}

/**************************************************************************
 63 sub_4682    ok++
 **************************************************************************/
bool sub_4682(register operand_t const *pi) {

    return pi->type == T_CONST && !pi->oPSym && pi->oVal == 0;
}

/**************************************************************************
 64 sub_46b1    ok++
 **************************************************************************/
int sub_46b1(register operand_t const *opr, int reg) {
    operand_t *po;
    int        i;

    po = reg < REG_BC ? &regValues[REG_B] : &regValues[REG_BC];
    do {
        if (po->type) {
            if (operandsSame(po, opr)) {
                i = (int)(po - regValues);
                if (i >= REG_BC && reg < REG_BC) {
                    return regHiLoMap[i].loReg;
                }
                return i;
            }
        }
    } while (++po < &regValues[REG_SP]);
    return -1;
}

/**************************************************************************
 65 sub_475c    ok++
 **************************************************************************/
int sub_475c(register operand_t const *po, int p2) {

    if (!po) {
        return false;
    }
    if (po->type != T_REG && po->type != T_INDEXED && po->type != T_REGREF) {
        return false;
    }
    if (p2 & regTestMasks[po->aux]) {
        return true;
    }
    return false;
}

/**************************************************************************
 66 sub_47a2    ok++
 **************************************************************************/
int sub_47a2(register operand_t const *po, int p2) {

    if (!po) {
        return false;
    }
    if (po->type != T_REG) {
        return false;
    }
    if (p2 & regTestMasks[po->aux]) {
        return true;
    }
    return false;
}

/**************************************************************************
 67 sub_47e0    ok++ (PMO)
 *
 *  Optimiser differences
 * 1) The code for case 0 is detected as the same as the code in case T_JP
 *    and reused
 * 2) There are two instances where the test cases are reversed along with
 *    the associated jumps. Leaving the logic the same
 **************************************************************************/
bool sub_47e0(int reg, register inst_t const *pi1, inst_t const *pi2) {
    uint16_t msk;
    sym_t   *ps;
    inst_t  *l3;
    int      n; /* number of iterations */

    if (reg == REG_SP || reg == REG_IX) {
        return true;
    }

    if (REG_BC <= reg && reg <= REG_HL) {
        if (sub_47e0(regHiLoMap[reg].hiReg, pi1, pi2)) {
            return true;
        }
        reg = regHiLoMap[reg].loReg;
    }
    if (reg >= sizeof(regTestMasks) / sizeof(regTestMasks[0])) {
        fprintf(stderr, "%d\n", reg);
    }
    /* #pragma warning(suppress : 6385) /* reg has limited values */
    msk = regTestMasks[reg]; /* m3: */
    n   = 40;

    do {
        switch (pi1->type) {

            case T_CALL:
                if (pi1->aux != 0 && (msk & 0x40)) {
                    return true;
                }

                if (reg == REG_IY) {
                    break;
                }
                if (!pi1->iLhs->oPSym || pi1->iLhs->oPSym->label[0] == '_') {
                    return false;
                }

                if (!(msk & 0xBF)) {
                    return false;
                } else {
                    return true;
                }

            case T_JP:
                if (pi1->aux != 0 || pi1->iLhs->type == T_REGREF || !(ps = pi1->iLhs->oPSym)) {
                    return true;
                }
                if (!(l3 = ps->p.pInst))
                    if (strcmp(ps->label, "cret") != 0) {
                        return true;
                    } else if (msk & 0x3C) { /* code reused */
                        return true;
                    } else {
                        return false;
                    }
                pi1 = l3;
                break;

            case T_SIMPLE:
                switch (pi1->aux) { /* m14: */
                    case 0:             /* "nop" */
                    case I_CPL:
                    case I_SCF:
                    case I_CCF:
                    case I_NEG:
                    case I_HALT:
                    case I_DI:
                    case I_EI:
                        break;
                    case I_EXX:
                        while ((pi1 = pi1->pNext) && (pi1->type != T_SIMPLE || pi1->aux != I_EXX))
                            if (pi1->type != T_LD && pi1->type != T_STK && pi1->type != T_CADD) {
                                return false;
                            }
                        if (!pi1) {
                            return false;
                        }
                        break;
                    default:
                        if (msk & 0x80) {
                            return true;
                        }
                        break;
                }
                break;
            case T_TWOBYTE:
                if (reg != 17) {
                    return true;
                }
                break;

            case T_3:
                if (pi1->aux == I_XOR) /* m22: */
                    if (pi1->iLhs->type == T_REG && pi1->iLhs->aux == REG_A && reg == REG_A) {
                        return false;
                    }
                if (msk & 0x80) {
                    return true;    /* m23: */
                }
                if (msk & 0x40) {
                    return false;
                }
                if (sub_475c(pi1->iLhs, msk)) { /* OPTIMISER[1]: see below */
                    return true;
                }
                break;

            case T_SHIFT:
                if ((pi1->aux & 0xFFE7) != 0x20 && (msk & 0x40)) {
                    return true;
                }

            case T_INCDEC:
                if ((msk & 0x40) && (pi1->iLhs->type != T_REG || pi1->iLhs->aux < REG_BC)) {
                    return false;
                }
                if (pi1->iLhs->type == T_REG || sub_475c(pi1->iLhs, msk)) {
                    return true;
                }
                break;

            case T_BIT: /* "set", "res", "bit" */
                if (pi1->aux == 0x40 && (msk & 0x40)) {
                    return false;
                }

            case T_0xF: /* 0xF */
                if (sub_475c(pi1->iRhs, msk) || sub_475c(pi1->iLhs, msk)) {
                    return true;
                }
                break;

            case T_5:
                break;

            case T_DJNZ:
                if (msk & 1) {
                    return true;
                }
                break;

            case T_0xE:
                if (sub_475c(pi1->iRhs, msk)) {
                    return true;
                }
                if (sub_47a2(pi1->iLhs, msk)) {
                    return false;
                }
                break;

            case T_STK:
                if (pi1->aux == I_PUSH && (msk & regTestMasks[pi1->iLhs->aux])) {
                    return true;
                }
                if (msk & regTestMasks[pi1->iLhs->aux]) {
                    return false;
                }
                break;

            case T_EX:
                if (pi1->iLhs->type == T_REGREF && (msk & regTestMasks[pi1->iRhs->aux])) {
                    return true;
                }
                if (msk & 0x3C) {
                    return true;
                }
                break;

            case T_CADD:
                if (msk & 0x40) {
                    return false;
                }

            case T_CARR: /* Add, sub with Carry */
                if ((regTestMasks[pi1->iLhs->aux] | 0x40) & msk) {
                    return true;
                }
                if (sub_475c(pi1->iRhs, msk)) {
                    return true;
                }
                break;
            case T_LD:
                if (!operandsSame(pi1->iLhs, pi1->iRhs))
                    if (sub_475c(pi1->iRhs, msk)) {
                        return true;
                    } else if (sub_47a2(pi1->iLhs, msk)) {
                        return false;
                    } else if (sub_475c(pi1->iLhs, msk)) {
                        return true;
                    }
                break;

            case 0: /* OPTIMISER[2]: optimised to reuse code in T_JP */
                if (msk & 0x3C) {
                    return true;
                } else {
                    return false;
                }
            case T_JR:
            case T_RET:
            case T_RST:
                break;
        }
        pi1 = pi1->pNext;
        if (pi2 == pi1) {
            return false;
        }
    } while (n-- != 0);
    return true;
}

/**************************************************************************
 68 sub_4c33    ok++ (PMO)  Used in: sub_4cf0, sub_4da7
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
    return (sym_t *)alloc_mem(sizeof(sym_t)); /* else allocate a new one */
}

/**************************************************************************
 69 sub_4c6b    ok+
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
 70 hash_index  sub_4cab    ok++    Used in: sub_4cf0
 **************************************************************************/
int hash_index(register char const *s, int size) {
    uint16_t hash;

    for (hash = 0; *s != 0; ++s) {
        hash += *(uint8_t *)s + hash;
    }
    return hash % size;
}

/**************************************************************************
 71 sub_4cf0    ok++ (PMO)          Used in: get_token
 **************************************************************************/
sym_t *lookupSym(register char const *s) {
    sym_t **pps;
    sym_t *ps;

    pps = &hashtab[hash_index(s, HASHSIZE)];
    while (*pps && strcmp((*pps)->label, s))
        if (++pps == &hashtab[HASHSIZE]) {
            pps = hashtab;
        }
    if (ps = *pps) {
        return ps;
    }
    *pps = ps = allocItem();
    ps->label = alloc_mem((int)strlen(s) + 1);
    strcpy(ps->label, s);
    return ps;
}

/**************************************************************************
 72 sub_4da7    ok++
 **************************************************************************/
sym_t *allocBlankSym() {
    register sym_t *ps;

    ps        = allocItem();
    ps->label = "";
    return ps;
}

/**************************************************************************
 73 sub_4dbf    ok++            Used in: optimise
 **************************************************************************/
void resetHeap() {
    int *p;

    if (programBreak == 0) {
        programBreak = sbrk(0);    /* current base of heap */
    } else {
        brk(programBreak);         /* reset the heap */
    }
    alloct = allocs = programBreak;

    for (p = (int *)hashtab; p < (int *)&hashtab[HASHSIZE];) {
        *p++ = 0;
    }
    freeItemList = NULL;
}

/**************************************************************************
 74 sub_4e20    ok++            Used in: optimise
 **************************************************************************/
void freeHashtab() {

    allocs = (char *)&hashtab;
    alloct = (char *)&hashtab[HASHSIZE];
}

/**************************************************************************
 75 alloc_mem   sub_4e2d    ok++ (PMO)
 Optimiser differences.
 1) res instruction is used to clear lsb rather than and 0xfe
 2) de is loaded before the size calculation. As it is not used before
    the code does the same thing
 **************************************************************************/
void *alloc_mem(int size) {
    char *p;
    register char *pi;

    if ((size = (size + 1) & ~1) + allocs > alloct) {
        if ((allocs = sbrk(512)) == (char *) -1) {
            pr_error("Out of memory in %s", name_fun);
        }
        alloct = sbrk(0);
    }

    pi = allocs;
    allocs += size;
    for (p = pi; size-- != 0;) {
        *p++ = 0;    /* Clearing allocated memory area */
    }
    return pi;
}

/*
 * Simple sbrk & brk implementations
 */
#ifndef CPM

//#define MAXHEAP 0xff00
//static char *heapBase;
//static char *heapTop;

void *sbrk(int size) {

    if (!heapBase && !(heapTop = heapBase = malloc(MAXHEAP))) {
        fprintf(stderr, "can't allocate heap!!\n");
    }
    if (heapTop + size >= heapBase + MAXHEAP) {
        return (void *) -1;
    }
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

#endif

#ifdef _DEBUG
void heapchk(void const *p) {

    if (p && (p < (void *)heapBase || p >= (void *)heapTop) &&
            (p < (void *)hashtab || p >= (void *)&hashtab[HASHSIZE])) {
        fprintf(stderr, "out of range heap item\n");
    }
}
#endif

/**************************************************************************

#######                     ####### ######  #######   ###   #     #
#        #    #  #####      #     # #     #    #       #    ##   ##
#        ##   #  #    #     #     # #     #    #       #    # # # #
#####    # #  #  #    #     #     # ######     #       #    #  #  #
#        #  # #  #    #     #     # #          #       #    #     #
#        #   ##  #    #     #     # #          #       #    #     #
#######  #    #  #####      ####### #          #      ###   #     #

*/
