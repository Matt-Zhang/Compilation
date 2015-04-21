#include "optimize.h"
#include "translate.h"
extern InterCode codeListHead;
extern InterCode codeListTail;
int countUsefulOp = 0;
Operand usefulOps[4096];
void delCode(InterCode code) {
    InterCode prev = code->prev;
    InterCode next = code->next;
    prev->next = next;
    next->prev = prev;
    InterCodeKind type = code->kind;
    /* Because the issue of place, it is impossible to free these memories.
     * The Operand place is used in two InterCodes so cannot free one while remain another one.
     * Because this is only a toy model which will be covered by dust of data after the class Principal Of Compilation, I will not fix this bug.
     * Actually, the solution is easy. Just make a copy of `place` wherever need to insert place in the children function.
     */
    /*
     switch(type) {
     case Assign: {
     free(code->u.assign.left);
     free(code->u.assign.right);
     } break;
     case BINOP: {
     free(code->u.binop.op1);
     free(code->u.binop.op2);
     free(code->u.binop.result);
     } break;
     case UNARYOP: {
     free(code->u.unaryop.op);
     } break;
     case If: {
     free(code->u.ifop.op1);
     free(code->u.ifop.op2);
     free(code->u.ifop.op3);
     } break;
     case IRDEC: {
     free(code->u.dec.op);
     } break;
     case CALL: {
     free(code->u.call.result);
     } break;
     case FUNC: {
     } break;
     case RET: {
     free(code->u.returnc.op);
     } break;
     case PARAM: {
     free(code->u.param.op);
     } break;
     }
     free(code);
     */
}
bool isUseful(Operand op) {
    if (op->isPointer) {
        return true;
    }
    for (int i = 0; i < countUsefulOp; i++) {
        if (op->u.no == usefulOps[i]->u.no ) {
             if (!((op->kind == TEMPVAR && usefulOps[i]->kind == VARIABLE) ||
                (op->kind == VARIABLE && usefulOps[i]->kind == TEMPVAR))) {
            return true;
             }
        }
    }
    return false;
}

void insertUsefulOp(Operand op) {
    if (op->isPointer) {
        usefulOps[countUsefulOp++] = op;
        return;
    }
    if (op->kind == CONSTANT) {
        return;
    }
    if (isUseful(op)) {
        return;
    }
    usefulOps[countUsefulOp++] = op;
}
void optUseless() {
    InterCode backiter= codeListTail->prev;
    while (backiter != codeListHead) {
        if (backiter->kind == RET) {
            insertUsefulOp(backiter->u.returnc.op);
        }
        else if(backiter->kind == PARAM) {
            insertUsefulOp(backiter->u.param.op);
        }
        else if(backiter->kind == UNARYOP) {
            if (strcmp(backiter->u.unaryop.kind, "WRITE") == 0) {
                insertUsefulOp(backiter->u.unaryop.op);
            }
            if(strcmp(backiter->u.unaryop.kind, "ARG") == 0) {
                insertUsefulOp(backiter->u.unaryop.op);
            }
        }
        else if(backiter->kind == If) {
            insertUsefulOp(backiter->u.ifop.op1);
            insertUsefulOp(backiter->u.ifop.op2);
        }
        backiter = backiter->prev;
    }
    
    bool changing = false;
    while (changing) {
        changing = false;
        for(backiter = codeListTail->prev; backiter != codeListHead;backiter = backiter->prev) {
            if(backiter->kind == Assign) {
                if (backiter->u.assign.left->isPointer) {
                    insertUsefulOp(backiter->u.assign.left);
                    changing = true;
                }
                if (isUseful(backiter->u.assign.left)) {
                    insertUsefulOp(backiter->u.assign.right);
                    changing = true;
                }
                
            }
            else if(backiter->kind == BINOP) {
                if (backiter->u.binop.result->isPointer) {
                    insertUsefulOp(backiter->u.binop.result);
                    changing = true;
                }
                if (isUseful(backiter->u.binop.result)) {
                    insertUsefulOp(backiter->u.binop.op1);
                    insertUsefulOp(backiter->u.binop.op2);
                }
            }
        }
    }
    
    for(backiter = codeListTail->prev; backiter != codeListHead;backiter = backiter->prev) {
        if(backiter->kind == Assign) {
            if (!isUseful(backiter->u.assign.left)) {
                InterCode t = backiter;
                backiter = backiter->prev;
                delCode(t);
            }
        }
        else if(backiter->kind == BINOP) {
            if (!isUseful(backiter->u.binop.result)) {
                InterCode t = backiter;
                backiter = backiter->prev;
                delCode(t);
            }
        }
    }
}


bool isLabel(InterCode c) {
    if (c->kind == UNARYOP) {
        if(strcmp(c->u.unaryop.kind, "LABEL") == 0) {
            return true;
        }
    }
    return false;
    
}
// To opt the situation where more labels are near
void optLabel() {
	InterCode back = codeListTail->prev;
	while(back != codeListHead->next) {
        if (isLabel(back)) {
            while(isLabel(back->prev)) {
                InterCode toDel = back->prev;
                for(InterCode c = back->prev->prev; c != codeListHead; c = c->prev) {
                    if(c->kind == UNARYOP) {
                        if (strcmp(c->u.unaryop.kind, "GOTO") == 0) {
                            if (toDel->u.unaryop.op->u.value == c->u.unaryop.op->u.value) {
                                c->u.unaryop.op->u.value = back->u.unaryop.op->u.value;
                            }
                        }
                    }
                    if (c->kind == If) {
                        if (toDel->u.unaryop.op->u.value == c->u.ifop.op3->u.value) {
                            c->u.ifop.op3->u.value= back->u.unaryop.op->u.value;
                        }
                    }
                }
                delCode(toDel);
            }
        }
        back = back->prev;
	}
}


/* The InterCode to consider is `code`
 * If the right value is op, then replace it by c
 */
bool replaceRightValue(InterCode code, Operand op, Operand c) {
    int size = sizeof(struct Operand_);
    if (code->kind == Assign) {
        if (memcmp(code->u.assign.right, op, size) == 0) {
            memcpy(code->u.assign.right, c, size);
            return true;
        }
    }
    else if(code->kind == BINOP) {
        if (memcmp(code->u.binop.op1, op, size) == 0) {
            memcpy(code->u.binop.op1, c, size);
            return true;
        }
        if (memcmp(code->u.binop.op2, op, size) == 0 ) {
            memcpy(code->u.binop.op2, c, size);
            return true;
        }
    }
    else if(code->kind == RET) {
        if (memcmp(code->u.returnc.op, op, size) == 0) {
            memcpy(code->u.returnc.op, c, size);
            return true;
        }
    }
    else if(code->kind == UNARYOP) {
        if(strcmp(code->u.unaryop.kind, "ARG") == 0 ||
           strcmp(code->u.unaryop.kind, "WRITE") == 0) {
            if (memcmp(code->u.unaryop.op, op, size) == 0) {
                memcpy(code->u.unaryop.op, c, size);
                return true;
            }
        }
    }
    else if(code->kind == If) {
        if (memcmp(code->u.ifop.op1, op, size) == 0) {
            memcpy(code->u.ifop.op1, c, size);
            return true;
        }
        if (memcmp(code->u.ifop.op2, op, size) == 0 ) {
            memcpy(code->u.ifop.op2, c, size);
            return true;
        }
    }
    return false;
}
//Only opt the next-line temp
void optVerboseTemp() {
    while (1) {
        InterCode code = codeListHead->next;
        bool changing = false;
        while(code->next != codeListTail) {
            if (code->kind == Assign) {
                if(code->u.assign.left->kind == TEMPVAR){
                    if(replaceRightValue(code->next, code->u.assign.left, code->u.assign.right)) {
                        InterCode t = code;
                        code = code->next;
                        delCode(t);
                        changing = true;
                        continue;
                    }
                    else if(code->u.assign.right->kind == VARIABLE && code->u.assign.right->isPointer == true) {
                        InterCode addr = code->next->next;
                        //This opt is only for t = &v in array.If optAdvance is enabled, this part is meaningless
                        if (addr->kind == BINOP) {
                            if (addr->u.binop.op2 == code->u.assign.left && !strcmp(addr->u.binop.kind, "+") ) {
                                addr->u.binop.op2 = code->u.assign.right;
                                InterCode t = code;
                                code = code->next;
                                delCode(t);
                                changing = true;
                                continue;  //Otherwise will be an other code = code->next;
                            }
                        }
                    }
                }
            }
            code = code->next;
        }
        if (!changing)
            break;
    }
    while (1) {
        InterCode code = codeListHead->next;
        bool changing = false;
        while(code != codeListTail) {
            if (code->kind == BINOP) {
                if (code->u.binop.op1->kind == CONSTANT && code->u.binop.op2->kind == CONSTANT) {
                    Operand result = code->u.binop.result;
                    code->kind = Assign;
                    Operand newC;
                    if (code->u.binop.kind[0] == '+')
                        newC = newOperand(CONSTANT, code->u.binop.op1->u.value + code->u.binop.op2->u.value);
                    else if (code->u.binop.kind[0] == '-')
                        newC = newOperand(CONSTANT, code->u.binop.op1->u.value - code->u.binop.op2->u.value);
                    else if (code->u.binop.kind[0] == '*')
                        newC = newOperand(CONSTANT, code->u.binop.op1->u.value * code->u.binop.op2->u.value);
                    else if (code->u.binop.kind[0] == '/')
                        newC = newOperand(CONSTANT, code->u.binop.op1->u.value / code->u.binop.op2->u.value);
                    code->u.assign.left = result;
                    code->u.assign.right = newC;
                }
                if(code->u.binop.result->kind == TEMPVAR) {
                    if(code->next->kind == Assign) {
                        //For the part after &&, the reason is *t = 3+4 is not valid
                        if (code->next->u.assign.right == code->u.binop.result && code->next->u.assign.left->kind != ADDRESS) {
                            Operand left = code->next->u.assign.left;
                            code->next->kind = BINOP;
                            code->next->u.binop.op1 = code->u.binop.op1;
                            code->next->u.binop.op2 = code->u.binop.op2;
                            code->next->u.binop.result = left;
                            code->next->u.binop.kind = code->u.binop.kind;
                            InterCode t = code;
                            code = code->next;
                            delCode(t);
                            changing = true;
                            continue;
                        }
                    }
                }
            }
            code = code->next;
        }
        if (!changing)
            break;
    }
}
//opt as more lines as possible
void optAdvance() {
    while (1) {
        InterCode code = codeListHead->next;
        bool changing = false;
        while(code->next != codeListTail) {
            if (code->kind == Assign) {
                if(code->u.assign.left->kind == TEMPVAR){
                    bool isReplaced = false;
                    for(InterCode goThrough = code->next; ; goThrough = goThrough->next) {
                        if (goThrough->kind == Assign) {
                            if (memcmp(goThrough->u.assign.left, code->u.assign.left, sizeof(struct Operand_)) == 0) {
                                break;
                            }
                            if(replaceRightValue(goThrough, code->u.assign.left, code->u.assign.right))
                                isReplaced = true;
                        }
                        else if (goThrough->kind == BINOP) {
                            if (memcmp(goThrough->u.binop.result, code->u.assign.left, sizeof(struct Operand_)) == 0)
                                break;
                            if(replaceRightValue(goThrough, code->u.assign.left, code->u.assign.right))
                                isReplaced = true;
                        }
                        else if(goThrough->kind == UNARYOP) {
                            if (strcmp(goThrough->u.unaryop.kind, "ARG") == 0) {
                                if(replaceRightValue(goThrough, code->u.assign.left, code->u.assign.right)) {
                                    isReplaced = true;
                                }
                            }
                            else {
                                break;
                            }
                        }
                        else if(goThrough->kind == If) {
                            if(replaceRightValue(goThrough, code->u.assign.left, code->u.assign.right)) {
                                isReplaced = true;
                            }
                            break;
                        }
                        else if(goThrough->kind == RET) {
                            if(replaceRightValue(goThrough, code->u.assign.left, code->u.assign.right)) {
                                isReplaced = true;
                            }
                            break;
                        }
                        else
                            break;
                    }
                    if (isReplaced) {
                        InterCode t = code;
                        code = code->next;
                        delCode(t);
                        changing = true;
                        continue;//Otherwise will be an other code = code->next;
                    }
                }
            }
            code = code->next;
        }
        if (!changing)
            break;
    }
}

void optimize() {
    optLabel();
    optVerboseTemp();
    optAdvance();
  //  optUseless();
}
