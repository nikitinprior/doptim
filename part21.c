/*
 * File part21.c created 14.08.2021, last modified 11.11.2021.
 *
 * The part21.c file is part of the restored optimization program
 * from the Hi-Tech C compiler v3.09 package.
 *
 *	Andrey Nikitin & Mark Ogden 11.11.2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "optim1.h"

/**************************************************************************
 31 sub_1795    ok++
 **************************************************************************/
bool sub_1795() {
    register inst_t *pi;
    static int stackAdjust;

    if (gPi->type != T_LD) {
        return false;
    }
    if (gPi->iLhs->type != T_REG || gPi->iLhs->aux != REG_SP) {
        return false;
    }
    if ((pi = gPi->pAlt)->type != T_CADD || (pi = pi->pAlt)->type != T_LD) {
        return false;
    }
    if (pi->iLhs->type != T_REG || pi->iLhs->aux != REG_HL || pi->iRhs->type != T_CONST) {
        return false;
    }
    if (pi->iRhs->oPOperand) {
        pr_error("Funny stack adjustment");
    }
    stackAdjust = pi->iRhs->oVal;
    pi          = gPi->pNext;
    if (pi->type == T_SIMPLE && pi->aux == I_EXX) {
        pi = pi->pNext;
    }
    for (; pi->type != T_CALL && pi->type != T_JP && pi->type != T_STK && (pi->type != T_EX || pi->iLhs->aux != REG_SP);
            pi = pi->pNext)
        ;

    if (stackAdjust > 0 && usesIXorIY && pi->aux == 0 && sub_4000(pi)) {
        removeInstruction(gPi->pAlt->pAlt);
        removeInstruction(gPi->pAlt);
        gPi = gPi->pAlt;
        removeInstruction(gPi->pNext);
        if (gPi->pNext->type == T_SIMPLE && gPi->pNext->aux == I_EXX && gPi->type == T_SIMPLE &&
                gPi->aux == I_EXX) { /* exx */
            removeInstruction(gPi->pNext);
            removeInstruction(gPi);
        }
        return logOptimise(O_STK_ADJUST); /* 6fc1 opt_msg[2] = "Stack adjustments" m7: */
    }
    pi = gPi->pAlt->pAlt;
    if (stackAdjust < 0) {
        stackAdjust = -stackAdjust;
    }
    if (pi->pAlt->type == T_SIMPLE && pi->pAlt->aux == I_EXX) {
        stackAdjust -= 2;
    }

    if (stackAdjust > 8 || pi->iRhs->oVal < 0) {
        return false;
    }

    logOptimise(O_STK_ADJUST); /* 6fc1 opt_msg[2] = "Stack adjustments" */

    stackAdjust = pi->iRhs->oVal;
    pi          = pi->pAlt;
    removeInstruction(pi->pNext->pNext);
    removeInstruction(pi->pNext);
    removeInstruction(gPi);
    gPi = pi;

    while (stackAdjust != 0) {
        gPi             = allocInst(gPi);
        gPi->iLhs       = allocOperand();
        gPi->iLhs->type = T_REG;
        if (1 < stackAdjust) {
            gPi->iLhs->aux = REG_BC;
            gPi->type      = T_STK;
            stackAdjust -= 2;
            gPi->aux = I_POP;
        } else {
            gPi->iLhs->aux = REG_SP;
            gPi->type      = T_INCDEC; /* Decrement, Increment */
            --stackAdjust;
            gPi->aux = SI_INC;
        }
    }
    if (gPi->pNext->type == T_SIMPLE && gPi->pNext->aux == I_EXX && pi->type == T_SIMPLE && pi->aux == I_EXX) {
        removeInstruction(gPi->pNext);
        removeInstruction(pi);
    }
    return true;
}

/**************************************************************************
 32 sub_1aec    ok++
 **************************************************************************/
bool sub_1aec() {
    register inst_t *pi;

    if (gPi->iLhs->type != T_REGREF) {
        if ((gPs = gPi->iLhs->oPSym)->p.pInst) {
            pi = getNextRealInst(gPs->p.pInst);
            pi = pi->pAlt;
            if (gPs->p.pInst != pi) {
                gPi->iLhs->oPOperand = pi->iLhs;
                removeLabelRef(gPs);
                ++pi->aux;                     /* safe const change */
                return logOptimise(O_RED_LAB); /* 6fbd opt_msg[0] = "Redundant labels" */
            }
        }
    }
    return false;
}

/**************************************************************************
 33 sub_1b86    ok++
 **************************************************************************/
bool sub_1b86() {
    register inst_t *pi;

    if (gPi->type == T_JP || gPi->type == T_CALL) {
        if ((gPs = gPi->iLhs->oPSym) && (pi = gPs->p.pInst)) {
            pi = getNextRealInst(pi);
            if (pi->type == T_JP && (pi->aux == 0 || pi->aux == gPi->aux) && pi->iLhs->oPSym != gPs) {
                removeLabelRef(gPs);
                gPs              = pi->iLhs->oPSym;
                gPi->iLhs->oPSym = gPs;
                if (gPs->p.pInst) {
                    ++gPs->p.pInst->aux;
                }
                return logOptimise(O_JMP_TO_JMP); /* 6fbf opt_msg[1] = "Jumps to jumps" */
            }
        }
    }

    return false;
}

/**************************************************************************
 34 sub_1c67    ok++
 **************************************************************************/
bool sub_1c67() {
    register inst_t *pi;
    if (gPi->aux == 0) {
        for (pi = gPi->pNext; pi; pi = pi->pNext) {
            if (instructionsSame(pi, gPi)) {
                seq1 = gPi;
                seq2 = pi;
                /* match chains of instructions */
                while (instructionsSame(seq2->pAlt, seq1->pAlt)) {
                    seq1 = seq1->pAlt;
                    seq2 = seq2->pAlt;
                }
                if (seq1 != gPi) { /* we matched some common code */
                    seq1              = syntheticLabel(seq1->pAlt);
                    seq2              = allocInst(seq2->pAlt);
                    seq2->iLhs        = allocOperand();
                    seq2->type        = T_JP;
                    seq2->iLhs->type  = T_CONST;
                    seq2->iLhs->oPSym = seq1->iPSym;
                    ++seq1->aux;
                    removeInstruction(pi);
                    return logOptimise(O_CMN_CODE_SEQ); /* 6fcd opt_msg[8] = "Common code seq's" */
                }
            }
        }
    }
    return false;
}

/**************************************************************************
 35 sub_1d94    ok++
 **************************************************************************/
bool sub_1d94() {
    register inst_t *pi;

    if (gPi->aux != 0 && (pi = gPi->iLhs->oPSym->p.pInst)) {
        for (seq1 = gPi; sub_4625(pi->pNext) && instructionsSame(seq1->pNext, pi->pNext); pi = pi->pNext) {
            HEAP(pi->pNext);
            HEAP(seq1->pNext);
            seq1 = seq1->pNext;
        }
        if (seq1 != gPi) {
            if (pi->pNext->type == T_SYMBOL) {
                pi = pi->pNext;
            } else {
                pi = syntheticLabel(pi);
            }
            seq1                  = allocInst(seq1);
            seq1->iLhs            = allocOperand();
            seq1->iLhs->type      = T_CONST;
            seq1->iLhs->oPOperand = pi->iLhs;
            seq1->type            = gPi->type;
            seq1->aux             = gPi->aux;
            ++pi->aux;
            removeInstruction(gPi);
            return logOptimise(O_CMN_CODE_SEQ); /* 6fcd opt_msg[8] = "Common code seq's" */
        }
    }
    return false;
}

/**************************************************************************
 36 sub_1ec1    ok++    Used in: optimize
 **************************************************************************/
#if 1

void sub_1ec1() {
    int l1;

    sub_4601();
    for (gPi = root; gPi; gPi = gPi->pNext) {
        switch (gPi->type) {
            case T_LD:
                if (!key_r) {
                    if (sub_24c0()) {
                        continue;
                    }
                    if (gPi->iLhs->type == T_REG && gPi->iLhs->aux == REG_A && sub_4682(gPi->iRhs) &&
                            !sub_47e0(REG_F, gPi->pNext, gPi)) {
                        gPi->iRhs   = NULL;
                        gPi->type   = T_3;
                        gPi->aux    = I_XOR; /* xor */
                        gPi->opCode = NULL;
                        logOptimise(O_XOR_A); /* 6fbf opt_msg[14] = "xor a's used" */
                    }
                }
                break;

            case T_SYMBOL:
                for (; gPi->type == T_SYMBOL && gPi->aux == 0; gPi = gPi->pNext) {
                    removeInstruction(gPi);
                    logOptimise(O_UNREF_LAB); /* 6fc5 opt_msg[4] = "Unref'ed labels" */
                }
            /* fall through */

            case T_TWOBYTE:
            case T_CALL:
                sub_4601();
                break;

            case T_EX:
                if (gPi->iLhs->type == T_REG) {
                    if (sub_23c1()) {
                        continue;
                    }
                    swapHLDE();
                } else {
                    sub_4544(gPi->iRhs->aux);
                }
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
                    if (gPi->iRhs->type == T_REG) {
                        sub_4544(gPi->iRhs->aux);
                    } else if ((l1 = sub_46b1(gPi->iRhs, 0)) != -1) {
                        sub_4544(l1);
                    }
                break;
caseCommon:
            case T_CARR:
                if (gPi->iLhs->type == T_REG && !sub_47e0(gPi->iLhs->aux, gPi->pNext, gPi)) {
                    if ((gPi->type == T_INCDEC && gPi->iLhs->aux >= REG_BC) || !sub_47e0(REG_F, gPi->pNext, gPi)) {
                        removeInstruction(gPi);
                        logOptimise(O_RED_OPS); /* 6fd3 opt_msg[11] = "Redundant operations" */
                        continue;
                    }
                }
                /* fall through */ /* m25 */

            case T_SHIFT:
            case T_0xE:
                if (gPi->iLhs->type == T_REG) {
                    sub_4544(gPi->iLhs->aux);
                } else {
                    sub_44b2(gPi->iLhs);
                }
                break;

            case T_CADD:
                if (!sub_47e0(gPi->iLhs->aux, gPi->pNext, gPi)) {
                    removeInstruction(gPi);
                    logOptimise(O_RED_OPS); /* 6fd3 opt_msg[11] = "Redundant operations" */
                    continue;
                } else if (gPi->iRhs->type == T_CONST && abs(gPi->iRhs->oVal) == 1 && !gPi->iRhs->oPOperand) {
                    gPi->type   = T_INCDEC;
                    gPi->aux    = gPi->iRhs->oVal != 1 ? SI_DEC : SI_INC;
                    gPi->opCode = NULL;
                    freeOperand(gPi->iRhs);
                    gPi->iRhs = NULL;
                    logOptimise(O_SIMPLE_ADDR); /* 6fd7 opt_msg[13] = "Simplified addresses" */
                } else {
                    goto caseCommon;
                }
            /* fall through */

            case T_INCDEC:
                if (gPi->iLhs->type != T_REG) {
                    goto caseCommon;
                }
                if (gPi->iLhs->aux != REG_HL) {
                    goto caseCommon;
                }

                if (sub_2ef8()) {
                    continue;
                }
                break;

            case T_DJNZ:
                sub_4544(0);
                break;

            case T_STK:
                if (gPi->aux == I_PUSH) {
                    if (gPi->iLhs->aux == REG_IY && (l1 = sub_46b1(&regValues[17], REG_IY)) != -1 && l1 != REG_IY) {
                        gPi->iLhs->aux = l1;
                        logOptimise(O_SIMPLE_ADDR); /* 6fd7 opt_msg[13] = "Simplified addresses" */
                    }
                    /* check for pop */
                    /* OTIMISER: fails to optimise the gPi->pNext-aux below */
                    if (gPi->pNext->type == T_STK && gPi->pNext->aux == I_POP && gPi->iLhs->aux == gPi->pNext->iLhs->aux) {
                        removeInstruction(gPi->pNext);
                        removeInstruction(gPi);
                        logOptimise(O_RED_OPS); /* 6fd3 opt_msg[11] = "Redundant operations" */
                    } else {
                        break;
                    }
                } else {
                    sub_4544(gPi->iLhs->aux); /* m39: */
                    if (gPi->pAlt->type == T_STK && gPi->pAlt->aux == I_PUSH) {
                        regValues[gPi->iLhs->aux] = regValues[gPi->pAlt->iLhs->aux];
                        if (gPi->iLhs->aux == REG_IY && gPi->pNext->type == T_STK && gPi->pNext->iLhs->aux == REG_IY) {
                            gPi->pNext->iLhs->aux = gPi->pAlt->iLhs->aux;
                            logOptimise(O_SIMPLE_ADDR); /* 6fd7 opt_msg[13] = "Simplified addresses" */
                        }
                    }
                    break;
                }
                continue;

            case T_3:
                if (sub_2d3b()) {
                    continue;
                }
                break;
        }
        sub_2bdb();
    }
}

#else

/*  This function generates correct code */

void sub_1ec1() {
    int l1;
    register inst_t *pi;

    sub_4601();

    gPi = root;
    while (gPi != 0) {                                                        /* m5: */
        pi = gPi;                                                             /* m1: */
        switch (pi->type) {
            case T_LD:
                if (key_r == 0) {                                             /* m6: */
                    if (sub_24c0() != 0) {
                        goto m4;
                    }

                    if (((pi = gPi)->iLhs->type == T_REG)
                            && (pi->iLhs->aux == REG_A)
                            && (sub_4682(pi->iRhs) != 0)
                            && (sub_47e0(REG_F, (pi = gPi)->pNext, gPi) == 0)) {
                        pi       = gPi;
                        pi->iRhs = 0;
                        pi->type = T_3;
                        pi->aux  = P_XOR;
                        gPi->opCode = 0;
                        ++optimiseCounters[14]; /* 6fd9 opt_msg[14] = "Xor a's used" m7: */
                        hasChanged = true;
                    }
                }
                goto m2;

            case T_SYMBOL:
                while (((pi = gPi)->type == T_SYMBOL) && (pi->aux == 0)) {      /* m9: */
                    removeInstruction(gPi);                     		/* m8: */
                    optimiseCounters[4]++; /* 6fc5 opt_msg[4] = "Unref'ed labels" */
                    hasChanged = true;
                    gPi = (pi = gPi)->pNext;
                }
                /* fall through */

            case T_TWOBYTE:
            case T_CALL:
m10:
                sub_4601();                                                     /* m10: */
                goto m2;

            case T_EX:
                if ((pi = gPi)->iLhs->type == T_REG) {                          /* m11: */
                    if (sub_23c1() != 0) {
                        goto m3;
                    }
                    swapHLDE();
                    goto m2;
                }
                sub_4544((pi = gPi)->iRhs->aux);                                /* m13: */
                goto m2;

            case T_ONEBYTE:
                switch ((pi = gPi)->aux) {                                      /* m15: */
                    case P_NOP:
                    case P_SCF:
                    case P_CCF:
                    case P_HALT:
                    case P_DI:
                    case P_EI:
                        goto m2;
                    case P_EXX:
                        goto m10;
                    default:
                        sub_4544(REG_AF);
                        break;                                                  /* m16: */
                }
                goto m2;

            case T_BIT:
                if ((pi = gPi)->aux == P_BIT) {
                    goto m2;    						/* m17: */
                }
                if (pi->iRhs->type == T_REG) {
                    sub_4544(pi->iRhs->aux);
                    goto m2;
                }
                if ((l1 = sub_46b1((pi = gPi)->iRhs, 0)) == -1) {
                    goto m2;    						/* m18: */
                }
                sub_4544(l1);
                goto m2;

            case T_CARR:
m19:
                pi = gPi;							/* m19: */
m20:
m21:
                if ((pi = gPi)->iLhs->type == T_REG) {
                    if (sub_47e0(pi->iLhs->aux, (pi = gPi)->pNext, gPi) != 0) {
                        goto m26;
                    }

                    pi = gPi;
                    if ((pi->type != T_INCDEC) || (pi->iLhs->aux < REG_BC)) {
                        if (sub_47e0(REG_F, (pi = gPi)->pNext, gPi) == 0) {
                            goto m23;    					/* m22: */
                        }
                        goto m25;
                    }
m23:
                    removeInstruction(gPi);
m24:
                    optimiseCounters[11]++; /* 6fd3 opt_msg[11] = "Redundant operations" */
                    hasChanged = true;
                    goto m3;
                }
                /* fall through */
            case T_6:
            case T_0xE:
m25:
                pi = gPi;
m26:
                if ((pi = gPi)->iLhs->type == T_REG) {
                    sub_4544((pi = gPi)->iLhs->aux);                            /* m27: */
                } else {
                    sub_44b2((pi = gPi)->iLhs);                                 /* m28: */
                }
                goto m2;

            case T_CADD:

                if (sub_47e0(pi->iLhs->aux, (pi = gPi)->pNext, gPi) == 0) {     /* m29: */
m30:
                    removeInstruction(gPi);
                    goto m24;
                }
                if ((pi = gPi)->iRhs->type != T_CONST) {
                    goto m20;                                                   /* m31: */
                }
                if (abs(pi->iRhs->oVal) != 1) {
                    goto m20;
                }
                if ((pi = gPi)->iRhs->oPOperand != 0) {
                    goto m20;
                }
                pi->type = T_INCDEC;
                (pi = gPi)->aux = (pi->iRhs->oVal  == 1) ? REG_H : REG_L;       /* m33: */
                gPi->opCode = 0;
                freeOperand(pi->iRhs);
                (pi = gPi)->iRhs = 0;
                optimiseCounters[13]++; /* 6fd7 opt_msg[13] = "Simplified addresses" */
                hasChanged = true;
                /* fall through */

            case T_INCDEC:
                if ((pi = gPi)->iLhs->type != T_REG) {
                    goto m21;    //m34:
                }
                if (pi->iLhs->aux          != REG_HL) {
                    goto m19;
                }
                if (sub_2ef8() != 0) {
                    goto m3;                                                    /* m35: */
                }
                goto m2;

            case T_DJNZ:
                sub_4544(0); //REG_B                                            /* m36: */
                goto m2;

            case T_STK:
                if ((pi = gPi)->aux == P_PUSH) {                                /* m37: */
                    if ((pi = gPi)->iLhs->aux == REG_IY) {
                        l1 = sub_46b1(&regValues[17], REG_IY);
                        if (l1 != -1) {
                            if (l1 !=  REG_IY) {
                                (pi = gPi)->iLhs->aux = pi->type;
                                optimiseCounters[13]++; /* 6fd7 opt_msg[13] = "Simplified addresses" */
                                hasChanged = true;
                            }
                        }
                    }
                    /* Check for pop */
                    if ((pi = gPi)->pNext->type != T_STK) {
                        goto m2;    						/* m38: */
                    }
                    if (pi->pNext->aux != P_POP) {
                        goto m2;
                    }
                    if (pi->iLhs->aux != pi->pNext->iLhs->aux) {
                        goto m2;
                    }
                    removeInstruction(pi->pNext);
                    goto m30;
                }
                sub_4544(pi->iLhs->aux);                                        /* m39: */
                if ((pi = gPi)->pAlt->type != T_STK) {
                    goto m2;
                }
                if (pi->pAlt->aux          != P_PUSH) {
                    goto m2;
                }

                /* Ambiguious code - pi could be modified on rhs before using on lhs
                 * on hitech generated code gets lhs before rhs
                 */
                regValues[(pi = gPi)->iLhs->aux] = regValues[pi->pAlt->iLhs->aux];
                if ((pi = gPi)->iLhs->aux != REG_IY) {
                    goto m2;
                }
                if (pi->pNext->type       != T_STK) {
                    goto m2;
                }
                if (pi->pNext->iLhs->aux  != REG_IY) {
                    goto m2;
                }
                pi->pNext->iLhs->aux = pi->pAlt->iLhs->aux;
                ++optimiseCounters[13]; /* 6fd7 opt_msg[13] = "Simplified addresses" */
                hasChanged = true;
                goto m2;

            case T_3:   //   -   -   -   -   -   -   -   -   -   -   -   -   -
                if (sub_2d3b() != 0) {
                    goto m3;                                                    //m40: */
                }
        }
m2:
        sub_2bdb();
m3:
        pi = gPi;
m4:
        gPi = (pi = gPi)->pNext;
    }
}

#endif

/**************************************************************************
 37 sub_23c1    ok++
 **************************************************************************/
bool sub_23c1() {
    register inst_t *pi;

    if ((pi = gPi->pNext)->type == T_STK && pi->aux == I_PUSH && pi->iLhs->aux == gPi->iLhs->aux &&
            !sub_47e0(gPi->iLhs->aux, pi->pNext, gPi) && !sub_47e0(gPi->iRhs->aux, pi, gPi)) {
        pi->iLhs->aux = gPi->iRhs->aux;
        removeInstruction(gPi);
        gPi = pi;
    } else if (gPi->pNext->type == T_EX && gPi->pNext->iLhs->type == T_REG) {
        removeInstruction(gPi->pNext);
        removeInstruction(gPi);
    } else {
        return false;
    }
    return logOptimise(O_RED_EX_DEHL); /* 6fdb opt_msg[15] = "Redundant ex de,hl's" */
}

/**************************************************************************
 38 sub_24c0    ok++
 **************************************************************************/
bool sub_24c0() {
    inst_t const *l1;
    operand_t    *l2;
    int           reg;

    if (sub_3053()) {
        return true;
    }
    if (gPi->iLhs->type == T_REG &&
            (gPi->iRhs->type == T_INDEXED || gPi->iRhs->type == T_ADDRREF || gPi->iRhs->type == T_CONST)) {
        if (sub_29c3()) {
            return true;
        }
    } else if (gPi->iRhs->type == T_CONST && gPi->iLhs->type == T_INDEXED) {
        if ((reg = sub_46b1(gPi->iRhs, REG_A)) != -1) {
            gPi->iRhs->type = T_REG; /* m6: */
            gPi->iRhs->aux  = reg;
            return logOptimise(O_SIMPLE_ADDR); /* 6fd7 opt_msg[13] = "Simplified addresses" */
        }
        sub_44b2(gPi->iLhs);
    } else if (gPi->iRhs->type == T_REG && (gPi->iLhs->type == T_INDEXED || gPi->iLhs->type == T_ADDRREF)) {
        if (operandsSame(gPi->iLhs, &regValues[gPi->iRhs->aux])) {
kill:
            removeInstruction(gPi);
            return logOptimise(O_RED_LD); /* 6fd5 opt_msg[12] = "Redundant loads/stores" */
        }
        sub_44b2(gPi->iLhs);
        if (regValues[gPi->iRhs->aux].type == T_INVALID) {
            sub_4544(gPi->iRhs->aux);
            regValues[gPi->iRhs->aux] = *gPi->iLhs;
        }

    } else if (gPi->iLhs->type == T_REG && gPi->iRhs->type == T_REG) { /* 2824 */
        if (gPi->iLhs->aux == gPi->iRhs->aux || operandsSame(&regValues[gPi->iLhs->aux], &regValues[gPi->iRhs->aux])) {
            goto kill;
        }

        if ((seq2 = gPi->pNext)->type == T_LD && operandsSame(seq2->iLhs, gPi->iRhs) &&
                operandsSame(seq2->iRhs, gPi->iLhs)) {
            removeInstruction(seq2);
            logOptimise(O_RED_LD); /* 6fd5 opt_msg[12] = "Redundant loads/stores" */
        }
        if (!sub_47e0(gPi->iLhs->aux, gPi->pNext, gPi)) {
            goto kill;
        }
        if (gPi->iLhs->aux == REG_E && gPi->iRhs->aux == REG_L && (seq2 = gPi->pNext)->type == T_LD &&
                seq2->iLhs->type == T_REG && seq2->iLhs->aux == REG_D && seq2->iRhs->type == T_REG &&
                seq2->iRhs->aux == REG_H && !sub_47e0(REG_HL, seq2->pNext, gPi)) {
            removeInstruction(gPi->pNext);
            gPi->type      = T_EX;
            gPi->opCode    = NULL;
            gPi->iLhs->aux = REG_DE;
            gPi->iRhs->aux = REG_HL;
            swapHLDE();
            return logOptimise(O_SIMPLE_ADDR); /* 6fd7 opt_msg[13] = "Simplified addresses" */
        }
        sub_4544(gPi->iLhs->aux);
        regValues[gPi->iLhs->aux] = regValues[gPi->iRhs->aux];
    } else if (gPi->iLhs->type == T_REG) {
        sub_4544(gPi->iLhs->aux);
    }

    if (gPi->iLhs->type == T_REG && gPi->iRhs->type == T_REG) {
        if ((l1 = gPi->pAlt)->type == T_LD && (l2 = l1->iLhs)->type == T_REG && l2->aux == gPi->iRhs->aux &&
                !sub_47e0(l2->aux, gPi->pNext, l1) && sub_1369(l1->iRhs)) {
            sub_4544(l2->aux);
            regValues[l2->aux = gPi->iLhs->aux] = *l1->iRhs;
            goto kill;
        }
        if ((l1 = gPi->pNext)->type == T_LD && (l2 = l1->iRhs)->type == T_REG && l2->aux == gPi->iLhs->aux &&
                !sub_47e0(l2->aux, l1->pNext, gPi) && sub_1369(l1->iLhs)) {
            sub_4544(l2->aux);
            l2->aux = gPi->iRhs->aux;
            goto kill;
        }
    }
    return false; /* m26: */
}

/**************************************************************************
 39 sub_29c3    ok+ (PMO)
 *
 * Code is identical, except that the optimiser moves the code in the
 * first if block to a different location. The jp conditions are changed to
 * reflect this)
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
            gPi->iRhs->type           = T_REG;
            gPi->iRhs->aux            = l1;
            if (gPi->iLhs->aux >= REG_BC) {
                pi1             = allocInst(gPi);
                pi1->iLhs       = allocOperand();
                pi1->iRhs       = allocOperand();
                pi1->iLhs->type = T_REG;
                pi1->iRhs->type = T_REG;
                pi1->iLhs->aux  = regHiLoMap[gPi->iLhs->aux].hiReg;
                pi1->iRhs->aux  = regHiLoMap[gPi->iRhs->aux].hiReg;
                gPi->iLhs->aux  = regHiLoMap[gPi->iLhs->aux].loReg;
                gPi->iRhs->aux  = regHiLoMap[gPi->iRhs->aux].loReg;
                pi1->type       = T_LD;
                gPi             = pi1;
            }
            return logOptimise(O_SIMPLE_ADDR); /* 6fd7 opt_msg[13] = "Simplified addresses" */
        }
    }
    regValues[gPi->iLhs->aux] = *gPi->iRhs;
    return false;
}

/**************************************************************************
 40 sub_2bdb    ok++ (PMO)
 **************************************************************************/
bool sub_2bdb() {
    register inst_t *pi;

    if (gPi->type == T_STK && gPi->iLhs->aux == REG_BC && gPi->aux == I_POP && !sub_47e0(REG_BC, gPi->pNext, gPi)) {
        for (pi = gPi->pNext; pi && (pi->type != T_JP && pi->type != T_CALL && pi->type != T_SYMBOL); pi = pi->pNext) {
            if (pi->type == T_STK)
                if (pi->aux != I_PUSH || pi->pNext->type != T_STK || pi->pNext->aux != I_POP) {
                    break;
                } else {
                    pi = pi->pNext;
                }
            if (pi->type == T_EX || (pi->type == T_LD && pi->iLhs->type == T_REG))
                if (pi->iLhs->aux == REG_SP) {
                    break;
                }
        }
        if (pi->type == T_STK && pi->aux == I_PUSH && pi->iLhs->aux == REG_HL && !sub_47e0(REG_HL, pi->pNext, pi)) {
            removeInstruction(gPi);
            pi->opCode     = NULL;
            pi->type       = T_EX;
            pi->aux        = 0;
            pi->iRhs       = pi->iLhs;
            pi->iLhs       = allocOperand();
            pi->iLhs->type = T_REGREF;
            pi->iLhs->aux  = REG_SP;
            return logOptimise(O_EX_SPHL); /* 6fd1 opt_msg[10] = "Ex (sp),hl'pi used" */
        }
    }
    return false;
}

/**************************************************************************
 41 sub_2d3b    ok++ (PMO)
 *
 * six of the basic blocks are in different positions but the code in the
 * block is the same and the logic flow is maintained.
 **************************************************************************/
#if 1

bool sub_2d3b() {
    register operand_t *po;

    switch (gPi->aux) {
        default:
            if (sub_4682(gPi->iLhs)) {
                gPi->aux = I_OR;
            } else if ((po = gPi->iLhs)->type != T_REG || po->aux != REG_A) {
                sub_4544(REG_AF);
                return false;
            } else {
                break;
            }
        /* fall through */
        case I_OR:
            po = gPi->iLhs; /* case 0xB0 */
            if (sub_4682(po)) {
                po->type = T_REG;
                po->aux  = REG_A;
            }
        /* fall through */
        case I_SUB:
caseP_SUB: /* common */
            if (gPi->iLhs->type == T_REG && gPi->iLhs->aux == REG_A) {
                if (gPi->aux == I_SUB) {
                    break;
                } else if (!sub_47e0(REG_F, gPi->pNext, gPi)) {
                    removeInstruction(gPi);
                    return logOptimise(O_RED_OPS); /* 6fd3 opt_msg[11] = "Redundant operations" */
                }
                return false;
            }
            sub_4544(REG_AF);
            return false;
        case I_AND:
            po = gPi->iLhs;
            if (sub_4682(po)) {
                break;
            }
            if (po->type == T_CONST && !po->oPOperand && (po->oVal & 0xff) == 255 /* -1 */) {
                gPi->aux = I_OR;
                po->type = T_REG;
                po->aux  = REG_A;
            }
            goto caseP_SUB;

        case I_CP:
            return false;
    }

    if (!sub_47e0(REG_F, gPi->pNext, gPi) && sub_4682(&regValues[REG_A])) {
        removeInstruction(gPi);
        return logOptimise(O_RED_OPS); /* 6fd3 opt_msg[11] = "Redundant operations" */
    }
    gPi->aux                   = I_XOR;
    gPi->opCode                = NULL;
    gPi->iLhs->type            = T_REG;
    gPi->iLhs->aux             = REG_A;
    regValues[REG_A].type      = T_CONST;
    regValues[REG_A].oPOperand = NULL;
    regValues[REG_A].oVal      = 0;
    return false;
}

#else

/*  This function generates correct code */

bool sub_2d3b() {
    register operand_t *po;

    switch (gPi->aux) {
        case P_OR:
            po = gPi->iLhs;					/* m2: */
            if (sub_4682(po) != 0) {
                po->type = T_REG;
                po->aux  = REG_A;
            }
        /* fall through */

        case P_SUB:
m3:
            if ((gPi->iLhs->type != T_REG)
                    && (gPi->iLhs->aux  != REG_A)) {
                sub_4544(REG_AF);				/* m8: */
                return false;
            }
            if (gPi->aux == P_SUB)  {
m4:
                if ((sub_47e0(REG_F, gPi->pNext, gPi)  != 0)
                        && (sub_4682(&regValues[7]) == 0)) {
                    gPi->aux                   = P_XOR;		/* m12: */
                    gPi->opCode                = 0;
                    gPi->iLhs->type            = T_REG;
                    gPi->iLhs->aux             = REG_A;
                    regValues[REG_A].type      = T_CONST;
                    regValues[REG_A].oPOperand = 0;
                    regValues[REG_A].oVal      = 0;
                    return false;
                }
                goto m5;
            }
            if (sub_47e0(REG_F, gPi->pNext, gPi) != 0) {	/* m7: */
                return false;
            }

m5:
            removeInstruction(gPi);
            ++optimiseCounters[11]; /* 6fd3 err_msg[11] = "Redundant operations" */
            return hasChanged = true;

        case P_AND:
            po = gPi->iLhs;					/* m6: */
            if (sub_4682(po) != 0) {
                goto m4;
            }
            if ((po->type == T_CONST)
                    && (po->oPOperand == 0)
                    && (((int)(po->oVal & 0xff) == 255))) {
                gPi->aux  = P_OR;
                po->type  = T_REG;
                po->aux   = REG_A;
            }
            goto m3;

        case P_CP:
            return false;					/* m9: */
    }
    if (sub_4682(gPi->iLhs) != 0) {				/* m1: */
        gPi->aux = P_OR;
    }
    if (((po = gPi->iLhs)->type == T_REG)			/* m10: */
            && (po->aux == REG_A)) {
        goto m4;
    }
    sub_4544(REG_AF);						/* m11: */
    return false;
}

#endif

/**************************************************************************
 42 sub_2ef8    ok++
 **************************************************************************/
bool sub_2ef8() {
    operand_t tmp;
    int       l2;

    if (gPi->pNext->type == T_INCDEC && operandsSame(gPi->iLhs, gPi->pNext->iLhs) && gPi->pNext->aux != gPi->aux) {
        removeInstruction(gPi->pNext);
    }

    else if (sub_47e0(REG_HL, gPi->pNext, gPi)) {
        tmp = regValues[REG_TRACKER];
        l2  = hlDelta;
        if (regValues[REG_HL].type != T_INVALID ||
                (regValues[REG_L].type == T_INDEXED && regValues[REG_H].type == T_INDEXED &&
                 regValues[REG_L].aux == regValues[REG_H].aux && regValues[REG_L].oVal + 1 == regValues[REG_H].oVal)) {
            if (regValues[REG_HL].type != T_INVALID) {
                tmp = regValues[REG_HL];
            } else {
                tmp = regValues[REG_L];
            }
            l2 = 0;
        }
        sub_4544(REG_HL);
        regValues[REG_TRACKER] = tmp;
        hlDelta                = l2;
        if (gPi->aux == SI_INC) {
            ++hlDelta;
        } else {
            --hlDelta;
        }
        return false;
    }
    removeInstruction(gPi);
    return logOptimise(O_RED_OPS); /* 6fd3 opt_msg[11] = "Redundant operations" */
}

/**************************************************************************
 42a    sub_3053    ok+-
 **************************************************************************/
bool sub_3053() {
    int l1;
    int l2;

    if (gPi->iLhs->type != T_REG || ((l1 = gPi->iLhs->aux) != REG_HL && l1 != REG_L)) {
        return false;
    }

    if (!operandsSame(gPi->iRhs, &regValues[REG_TRACKER])) {
        return false;
    }

    l2 = hlDelta;
    if (gPi->iLhs->aux == REG_L) {
        if ((seq2 = gPi->pNext)->type != T_LD || seq2->iLhs->type != T_REG || seq2->iLhs->aux != REG_H ||
                seq2->iRhs->oVal != gPi->iRhs->oVal + 1) {
            return false;
        }

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
        gPi->type       = T_INCDEC;
        gPi->iLhs       = allocOperand();
        gPi->iLhs->type = T_REG;
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
 43 sub_31ee    ok++
 **************************************************************************/
void swapHLDE() {
    operand_t pi;

    pi                          = regValues[REG_HL];
    regValues[REG_HL]           = regValues[REG_DE];
    regValues[REG_DE]           = pi;

    pi                          = regValues[REG_H];
    regValues[REG_H]            = regValues[REG_D];
    regValues[REG_D]            = pi;

    pi                          = regValues[REG_L];
    regValues[REG_L]            = regValues[REG_E];
    regValues[REG_E]            = pi;
    regValues[REG_TRACKER].type = T_INVALID;
    ;
};

/**************************************************************************
 44 pr_psect    sub_328a    ok++
 **************************************************************************/
void pr_psect(int psect) {

    if (psect == cur_psect) {
        return;
    }
    printf("psect\t%s\n", psectNames[psect]);
    cur_psect = psect;
}

/**************************************************************************
 45 num_psect   sub_32bf    ok++
 **************************************************************************/
int num_psect(char const *fmt) {
    int l1;

    for (l1 = 0; l1 < 4; ++l1) {
        if (strcmp(fmt, psectNames[l1]) == 0) {
            return l1;
        }
    }
    pr_error("Unknown psect");
}

/**************************************************************************
 46	evalExpr	sub_3313  ok++   (PMO) apart from optimiser changes
 1) one code optimisation to remove jp, putting condition code on call
 2) code block moved with optimisation on shared code
 3) two further code block group moves
 Note the other code is identical and overall the logic is the same
 **************************************************************************/
typedef struct {
    int type; /* 01 s->str */
    int prec; /* 23 s->i_2 */
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
    pOp->type = T_MARKER;
    pOp->prec = 0;
    expectOp  = false;
    do {                                 /* REDUCE loop */
        for (;; tokType = get_token()) { /* SHIFT loop */
            if (tokType == T_STRING) {   /* in expressions "A" is treated as ascii value of A */
                if (strlen(yylval.pChar) != 1) {
                    pr_warning("Bad character const");
                }
                yylval.i = *yylval.pChar;
                tokType  = T_CONST;
            }
            if (T_FWD <= tokType && tokType <= T_CONST) { /* get the term, note two terms together is an error */
                if (expectOp) {
                    exp_err();
                }
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
                    if (expectOp) {
                        exp_err();
                    }
                } else {
                    if (!expectOp && tokType != T_OPAR) { /* binary op only when expecting op*/
                        exp_err();
                    }
                    if (pOp->prec >= yylval.i) { /* pick up precedence */
                        break;
                    }
                }
                if (tokType != T_CPAR) { /* not a closing ) */
                    --pOp;
                    pOp->type = tokType;   /* push its type */
                    if (tokType == T_OPAR) { /* if it was a ( then set prec to 1 */
                        yylval.i = 1;
                    } else {                                 /* OPTIMISER[2]: code moved to here */
                        expectOp = false;    /* now expecting a term */
                    }
                    pOp->prec = yylval.i; /* set the prec */ /* OPTIMISER[3] code block moved / shared */
                } else if (pOp->type ==
                           T_MARKER) /* ) with nothing on stack */ { /* OPTIMISER[4]: test code block moved */
                    break;
                }
            } else {
                break;
            }
        }

        /* REDUCE phase */
        if (pOp->type == T_OPAR) { 	/* check for matching () */
            if (tokType != T_CPAR) {
                exp_err();    		/* ")" */
            }
            tokType  = get_token(); 	/* prime for next part */
            expectOp = 1;           	/* assuming its a term */
        } else if (T_MARKER >= pOp->type) {
            uconv(pOp->type, pTerm);    /* calculate unary operator */
        } else {
            bconv(pOp->type, pTerm + 1, pTerm); /* calculate binary operator*/
            ++pTerm;
        }
    } while (++pOp != &opStack[30]); 	/* loop till end of operator stack */

    exprResult = *pTerm;
    if (&termStack[29] != pTerm) { 	/* oops outstanding term */
        exp_err();
    }
    return &exprResult;
}

/**************************************************************************
 47 exp_err     sub_32bf    ok++ (PMO)
 **************************************************************************/
void exp_err() {

    pr_error("Expression error");
}

/**************************************************************************
 48 sub_359e    ok++ (PMO)
 *
 *  Unary operators
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
    if (lhs->tPSym) {
        rel_err();
    }
    return;
}

/**************************************************************************
 49 sub_3630    ok++ (PMO)
 *
 *  Binary operators
 **************************************************************************/
void bconv(int op, term_t *lhs, term_t const *rhs) {

    switch (op) {
        case T_PLUS:
            if (lhs->tPSym && rhs->tPSym) {
                rel_err();
            }
            lhs->val += rhs->val;
            if (!lhs->tPSym) {
                lhs->tPSym = rhs->tPSym;
            }
            return;
        case T_MINUS:
            if (rhs->tPSym) {
                rel_err();
            }
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

    if (lhs->tPSym || rhs->tPSym) {
        rel_err();
    }
}
