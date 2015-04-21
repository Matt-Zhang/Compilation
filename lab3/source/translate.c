#include "translate.h"
#include "ASTNode.h"
extern struct Symbol *funcList ;
extern struct Symbol *varList;
extern struct StructList *structList; /* The declared struct type */

InterCode codeListHead = NULL;
InterCode codeListTail = NULL;
int tempVarNo = 0;
int varNo = 0;
int labelNo = 0;
struct Stack {
	int pointer;
	InterCode code[MAX_ARG];
};
typedef struct Stack Stack;

void push(Stack *s, InterCode c) {
    s->code[++s->pointer] = c;
}

InterCode pop(Stack *s) {
	if(s->pointer == -1)
		return NULL;
	return s->code[s->pointer--];
}

void translateStmt(ASTNode* stmt);
void translateComp(ASTNode* comp);
void translateExp(ASTNode* exp, Operand place);
int sizeofArray(Type* type) {
    int pro = 1;
    while (type->kind != basic) {
        pro *= type->u.array.size;
        type = type->u.array.elem;
    }
    //Size of int or float
    pro *= 4;
    return pro;
}

Operand newOperand(OperandKind type, ... ) {
	Operand op = (Operand)malloc(sizeof(struct Operand_));
	memset(op, 0, sizeof(struct Operand_));
	op->kind = type;
	va_list list;
	va_start(list, type);
	switch(type) {
		case VARIABLE: {
			Symbol* s = va_arg(list, Symbol*);
			op->isPointer = va_arg(list, bool);
            if (s->varno == 0) {
                s->varno = ++varNo;
            }
            op->u.no = s->varno;
            break;
        }
		case TEMPVAR: {
            op->u.no = ++tempVarNo;
            break;
        }
		case ADDRESS: {
            op->u.no = va_arg(list, int);
			op->isPointer = va_arg(list, bool);
            break;
        }
		case CONSTANT: {op->u.value = va_arg(list,int); break;}
		case LABEL: {op->u.value = ++labelNo; break;}
        default:{break;}
	}
	va_end(list);
	return op;
}

InterCode newInterCode(InterCodeKind type, ... ) {
	InterCode code = (InterCode)malloc(sizeof(struct InterCode_));
	memset(code, 0, sizeof(struct InterCode_));
	code->kind = type;
	va_list list;
	va_start(list, type);
	switch(type) {
		case Assign: {
            code->u.assign.left = va_arg(list,Operand);
            code->u.assign.right = va_arg(list,Operand);
        } break;
		case BINOP: {
            code->u.binop.kind = va_arg(list, char *);
            code->u.binop.op1 = va_arg(list, Operand);
            code->u.binop.op2 = va_arg(list, Operand);
            code->u.binop.result = va_arg(list, Operand);
        } break;
		case UNARYOP: {
            code->u.unaryop.kind = va_arg(list, char *);
            code->u.unaryop.op = va_arg(list, Operand);
        } break;
		case If: {
            code->u.ifop.op1 = va_arg(list, Operand);
            code->u.ifop.relop = va_arg(list, char *);
            code->u.ifop.op2 = va_arg(list, Operand);
            code->u.ifop.op3 = va_arg(list, Operand);
        } break;
		case IRDEC: {
            code->u.dec.op = va_arg(list, Operand);
            code->u.dec.size = va_arg(list, int);
        } break;
		case CALL: {
            code->u.call.result = va_arg(list, Operand);
            code->u.call.fun_name = va_arg(list, char *);
        } break;
		case FUNC: {
            code->u.function = va_arg(list, char *);
        } break;
		case RET: {
            code->u.returnc.op = va_arg(list, Operand);
        } break;
		case PARAM: {
            code->u.param.op = va_arg(list, Operand);
        } break;
	}
	va_end(list);
	return code;
}

void insertCode(InterCode code){
	if(codeListHead == NULL){
        codeListHead = (InterCode)malloc(sizeof(struct InterCode_));
        memset(codeListHead, 0, sizeof(struct InterCode_));
        codeListTail = (InterCode)malloc(sizeof(struct InterCode_));
        memset(codeListTail, 0, sizeof(struct InterCode_));
        code->prev = codeListHead;
        code->next = codeListTail;
        codeListHead->next = code;
        codeListTail->prev = code;
	}
	else{
        InterCode prev = codeListTail->prev;
        prev->next = code;
        code->next = codeListTail;
        codeListTail->prev = code;
        code->prev = prev;
	}
}
//Use for If isFalse
char* getOppositeRelop(char* s) {
    if (strlen(s) == 1) {
        if (s[0] == '>')
            s[0] = '<';
        else
            s[0] = '>';
        s[1] = '=';
    }
    else {
        if (s[0] == '=') {
            s[0] = '!';
        }
        else if(s[0] == '!')
            s[0] = '=';
        else {
            s[1] = '\0';
            if (s[0] == '>')
                s[0] = '<';
            else
                s[0] = '>';
        }
        
    }
    return s;
}

void translateFundec(ASTNode* fundec) {
    Symbol* func = getSymbol(funcList, fundec->children->text);
    insertCode(newInterCode(FUNC, func->name));
    Stack stack;
    stack.pointer = -1;
    for (Symbol* para = func->para; para != NULL; para = para->next) {
        Symbol* var = getSymbol(varList, para->name);
        if (var->type.kind == basic) {
            Operand o = newOperand(VARIABLE, var, false);
            push(&stack, newInterCode(PARAM, o));
        }
        else {
            var->isdec = true;
            var->ispara = true;
            Operand o = newOperand(VARIABLE, var, false);
            push(&stack, newInterCode(PARAM, o));
        }
    }
    while (stack.pointer != -1) {
        insertCode(pop(&stack));
    }
    //To malloc the space for all arrays no matter where it is used or defined
    /*if (strcmp(func->name, "main") == 0) {
        for (Symbol* s = varList; s != NULL; s = s->next) {
            if (s->type.kind == array && s->isdec == 0) {
         //   if (s->type.kind == array) {
                Operand v1 = newOperand(VARIABLE, s, 0);
                insertCode(newInterCode(IRDEC, v1, sizeofArray(&s->type)));
                s->isdec = 1;
            }
        }
    }
     */
}
//Return the pointer
void translateArrayiter(ASTNode* exp, Operand place) {
    Operand t1 = newOperand(TEMPVAR);
    if (strcmp(exp->children->type, "ID") == 0) {
        translateExp(exp, t1);
        insertCode(newInterCode(Assign, place, t1));
    }
    else {
        translateArrayiter(exp->children, t1);
        Operand t2 = newOperand(TEMPVAR);
        translateExp(exp->children->sibling->sibling, t2);
        int size = sizeofArray(&exp->ntype);
        Operand c = newOperand(CONSTANT, size);
        Operand t4 = newOperand(TEMPVAR);
        insertCode(newInterCode(BINOP, "*", t2, c, t4));
        insertCode(newInterCode(BINOP, "+", t4, t1, place));
    }
}
//If need Addr, return addr; otherwise return the value
void translateArray(ASTNode* exp, Operand place, bool needAddr) {
    Operand t1 = newOperand(TEMPVAR);//18
    //t1 is the returned pointer
    translateArrayiter(exp, t1);
    if (needAddr) {
        insertCode(newInterCode(Assign, place, t1));
    }
    else {
        Operand t3 = newOperand(ADDRESS, t1->u.no, true);
        insertCode(newInterCode(Assign, place, t3));
    }
}
//Use FALL to go through control flow like the way on textbook
void translateCond(ASTNode* exp, Operand labelt, Operand labelf) {
    if (exp->children->sibling == NULL ||
        strcmp(exp->children->type, "MINUS") == 0) {
        Operand t1 = newOperand(TEMPVAR);
        translateExp(exp, t1);
        Operand c = newOperand(CONSTANT, 0);
        if (labelt->kind != FALL && labelf->kind != FALL) {
            insertCode(newInterCode(If, t1, "!=" , c, labelt));
            insertCode(newInterCode(UNARYOP, "GOTO", labelf));
        }
        else if(labelf->kind != FALL) {
            insertCode(newInterCode(If, t1, "==" ,c, labelf));
        }
        else if(labelt->kind != FALL) {
            insertCode(newInterCode(If, t1, "!=", c, labelt));
        }
    }/*
    else if(strcmp(exp->children->type, "MINUS") == 0) {
    
    }
      */
    else {
        char* kind = exp->children->sibling->type;
        if (strcmp(kind, "RELOP") == 0) {
            Operand t1 = newOperand(TEMPVAR);
            Operand t2 = newOperand(TEMPVAR);
            translateExp(exp->children, t1);
            translateExp(exp->children->sibling->sibling, t2);
            if (labelt->kind != FALL && labelf->kind != FALL) {
                insertCode(newInterCode(If, t1, exp->children->sibling->text, t2, labelt));
                insertCode(newInterCode(UNARYOP, "GOTO", labelf));
            }
            else if(labelf->kind != FALL) {
                insertCode(newInterCode(If, t1, getOppositeRelop(exp->children->sibling->text), t2, labelf));
            }
            else if(labelt->kind != FALL) {
                insertCode(newInterCode(If, t1, exp->children->sibling->text, t2, labelt));
            }
        }
        else if(strcmp(exp->children->type, "NOT") == 0) {
            Operand t1 = newOperand(TEMPVAR);
            translateExp(exp->children->sibling, t1);
            Operand c = newOperand(CONSTANT, 0);
            if (labelt->kind != FALL && labelf->kind != FALL) {
                insertCode(newInterCode(If, t1, "==" , c, labelt));
                insertCode(newInterCode(UNARYOP, "GOTO", labelf));
            }
            else if(labelf->kind != FALL) {
                insertCode(newInterCode(If, t1, "!=" ,c, labelf));
            }
            else if(labelt->kind != FALL) {
                insertCode(newInterCode(If, t1, "==", c, labelt));
            }
        }
        
        else if(strcmp(kind, "AND") == 0) {
            Operand lt1 = newOperand(FALL);
            Operand lf1;
            if (labelf->kind == FALL) {
                lf1 = newOperand(LABEL);
            }
            else {
                lf1 = labelf;
            }
            translateCond(exp->children, lt1, lf1);
            translateCond(exp->children->sibling->sibling, labelt, labelf);
            if (labelf->kind == FALL) {
                insertCode(newInterCode(UNARYOP, "LABEL", lf1));
            }
        }
        else if(strcmp(kind, "OR") == 0) {
            Operand lt1;
            if (labelt->kind == FALL) {
                lt1 = newOperand(LABEL);
            }
            else {
                lt1 = labelt;
            }
            Operand lf1 = newOperand(FALL);
            translateCond(exp->children, lt1, lf1);
            translateCond(exp->children->sibling->sibling, labelt, labelf);
            if (labelt->kind == FALL) {
                insertCode(newInterCode(UNARYOP, "LABEL", lt1));
            }
        }
        else if(strcmp(exp->children->type, "LP") == 0) {
            translateCond(exp->children->sibling, labelt, labelf);
        }
        else {
            debug("IN translateCond have unpredicted op kind!\n");
            fprintf(stderr, "%s%d", exp->children->type, exp->lineno);
            //exit(-1);
        }
    }
}

//The structure problem is handled in .y
void translateExp(ASTNode* exp, Operand place) {
    if(strcmp(exp->children->type, "INT") == 0) {
        if (place)
            insertCode(newInterCode(Assign, place,
                                    newOperand(CONSTANT, *(int*)exp->children->text)));
    }
    else if(strcmp(exp->children->type, "FLOAT") == 0) {
        if (place)
            insertCode(newInterCode(Assign, place,
                                    newOperand(CONSTANT, *(float*)exp->children->text)));
    }
    //Varibles
    else if(strcmp(exp->children->type, "ID") == 0 && exp->children->sibling == NULL) {
        if (place) {
            Symbol* s = getSymbol(varList, exp->children->text);
            if(s->type.kind == basic) {
                insertCode(newInterCode(Assign, place,
                                        newOperand(VARIABLE, s, false)));
            }
            //ARRAY
            else {
                if (s->isdec == false) {
                    Operand v1 = newOperand(VARIABLE, s, 0);
                    insertCode(newInterCode(IRDEC, v1, sizeofArray(&s->type)));
                    s->isdec = true;
                }
                if (s->ispara) {
                    Operand v1 = newOperand(VARIABLE, s, 0);
                    insertCode(newInterCode(Assign, place, v1));
                }
                else {
                    Operand v1 = newOperand(VARIABLE, s, 1);
                    insertCode(newInterCode(Assign, place, v1));
                }
            }
        }
    }
    //Function
    else if(strcmp(exp->children->type, "ID") == 0) {
        if (strcmp(exp->children->text, "read") == 0) {
            insertCode(newInterCode(UNARYOP, "READ", place));
        }
        else if (strcmp(exp->children->text, "write") == 0) {
            Operand t1 = newOperand(TEMPVAR);
            translateExp(exp->children->sibling->sibling->children, t1);
            insertCode(newInterCode(UNARYOP, "WRITE", t1));
        }
        else {
            ASTNode* args = exp->children->sibling->sibling;
            Symbol* func = getSymbol(funcList, exp->children->text);
            //No args
            if (strcmp(args->type, "RP") == 0){
                if (place)
                    insertCode(newInterCode(CALL, place, func->name));
                else {
                    Operand t1 = newOperand(TEMPVAR);
                    insertCode(newInterCode(CALL, t1, func->name));
                }
            }
            //Have args
            else {
                    Stack stack;
                    stack.pointer = -1;
                    for (ASTNode* exp = args->children; ; exp = exp->sibling->sibling->children) {
                        Operand t1 = newOperand(TEMPVAR);
                        if (exp->ntype.kind == basic) {
                            translateExp(exp, t1);
                            push(&stack, newInterCode(UNARYOP, "ARG", t1));
                        }
                        //Array
                        else {
                            translateArray(exp, t1, true);
                            push(&stack, newInterCode(UNARYOP, "ARG", t1));
                        }
                        if (exp->sibling == NULL) {
                            break;
                        }
                    }
                    while (stack.pointer != -1) {
                        insertCode(pop(&stack));
                    }
                if (place) {
                    insertCode(newInterCode(CALL, place, func->name));
                }
                else {
                    Operand t1 = newOperand(TEMPVAR);
                    insertCode(newInterCode(CALL, t1, func->name));
                }
            }
        }
    }
    else if(strcmp(exp->children->sibling->type, "ASSIGNOP") == 0) {
        //Normal
        if (strcmp(exp->children->children->type, "ID") == 0) {
            Operand v1 = newOperand(VARIABLE, getSymbol(varList, exp->children->children->text), false);
            Operand t2 = newOperand(TEMPVAR);
            translateExp(exp->children->sibling->sibling, t2);
            insertCode(newInterCode(Assign, v1, t2));
            if (place) {
                Operand v2 = newOperand(VARIABLE, getSymbol(varList, exp->children->children->text), false);
                insertCode(newInterCode(Assign, place, v2));
            }
        }
        //Array
        else {
            Operand t1 = newOperand(TEMPVAR);
            translateArray(exp->children, t1, true);
            Operand t3 = newOperand(ADDRESS, t1->u.no, true);
            Operand t2 = newOperand(TEMPVAR);
            translateExp(exp->children->sibling->sibling, t2);
            insertCode(newInterCode(Assign, t3, t2));
        }
    }
    //To handle the array, use another function.
    //Default to return value instead of addr
    else if(strcmp(exp->children->sibling->type,"LB") == 0) {
        Operand t1 = newOperand(TEMPVAR);
        translateArray(exp, t1, false);
        insertCode(newInterCode(Assign, place, t1));
        
    }
    
    else if(strcmp(exp->children->type, "MINUS") == 0) {
        if (place) {
            Operand t1 = newOperand(TEMPVAR);
            translateExp(exp->children->sibling, t1);
            insertCode(newInterCode(BINOP, "-", newOperand(CONSTANT, 0), t1, place));
        }
    }
    else if (!strcmp(exp->children->sibling->type, "AND")   ||
             !strcmp(exp->children->sibling->type, "OR")    ||
             !strcmp(exp->children->sibling->type, "RELOP") ||
             !strcmp(exp->children->type, "NOT")){
        Operand lt = newOperand(FALL);
        Operand lf = newOperand(LABEL);
        Operand c0 = newOperand(CONSTANT, 0);
        Operand c1 = newOperand(CONSTANT, 1);
        if (place) {
            insertCode(newInterCode(Assign, place, c0));
        }
        translateCond(exp, lt, lf);
        if (place) {
            insertCode(newInterCode(Assign, place, c1));
        }
        insertCode(newInterCode(UNARYOP, "LABEL", lf));
    }
    else if(strcmp(exp->children->type, "LP") == 0)
        translateExp(exp->children->sibling, place);
    else if(strcmp(exp->children->sibling->type, "DOT") == 0) {
        printf("￼Can not translate the code: Contain structure and function parameters of structure type!");
        exit(-1);
    }
    else {//+,-,*,/
        if (place) {
            Operand t1 = newOperand(TEMPVAR);
            Operand t2 = newOperand(TEMPVAR);
            translateExp(exp->children, t1);
            translateExp(exp->children->sibling->sibling, t2);
            char* kind = exp->children->sibling->type;
            InterCode code;
            if(strcmp(kind, "PLUS") == 0)
                code = newInterCode(BINOP, "+", t1, t2, place);
            else if(strcmp(kind, "MINUS") == 0)
                code = newInterCode(BINOP, "-", t1, t2, place);
            else if(strcmp(kind, "STAR") == 0)
                code = newInterCode(BINOP, "*", t1, t2, place);
            else
                code = newInterCode(BINOP, "/", t1, t2, place);
            insertCode(code);
        }
    }
}

void translateIf(ASTNode *iif) {
    //No else
    ASTNode* eelse = iif->sibling->sibling->sibling->sibling->sibling;
    if(eelse == NULL) {
        Operand lt = newOperand(FALL);
        Operand lf = newOperand(LABEL);
        translateCond(iif->sibling->sibling, lt, lf);
        translateStmt(iif->sibling->sibling->sibling->sibling);
        insertCode(newInterCode(UNARYOP, "LABEL", lf));
    }
    //Have else
    else {
        Operand lt = newOperand(FALL);
        Operand lf = newOperand(LABEL);
        Operand l2 = newOperand(LABEL);
        translateCond(iif->sibling->sibling, lt, lf);
        translateStmt(iif->sibling->sibling->sibling->sibling);
        insertCode(newInterCode(UNARYOP, "GOTO", l2));
        insertCode(newInterCode(UNARYOP, "LABEL", lf));
        translateStmt(eelse->sibling);
        insertCode(newInterCode(UNARYOP, "LABEL", l2));
    }
}

void translateWhile(ASTNode* whil) {
    Operand lt = newOperand(FALL);
    Operand lf = newOperand(LABEL);
    Operand l1 = newOperand(LABEL);
    insertCode(newInterCode(UNARYOP, "LABEL", l1));
    translateCond(whil->sibling->sibling, lt, lf);
    translateStmt(whil->sibling->sibling->sibling->sibling);
    insertCode(newInterCode(UNARYOP, "GOTO", l1));
    insertCode(newInterCode(UNARYOP, "LABEL", lf));
}

void translateStmt(ASTNode* stmt) {
    if (strcmp(stmt->children->type, "Exp") == 0) {
        translateExp(stmt->children, NULL);
    }
    else if (strcmp(stmt->children->type, "CompSt") == 0) {
        translateComp(stmt->children);
    }
    else if (strcmp(stmt->children->type, "RETURN") == 0) {
        Operand t1 = newOperand(TEMPVAR);
        translateExp(stmt->children->sibling, t1);
        insertCode(newInterCode(RET, t1));
    }
    
    else if (strcmp(stmt->children->type, "IF") == 0) {
        translateIf(stmt->children);
    }
    else if (strcmp(stmt->children->type, "WHILE") == 0) {
        translateWhile(stmt->children);
    }
    else {
        printf("Error type in translateComp!This is %s\n", stmt->children->type);
    }
}
void translateComp(ASTNode* comp) {
    // DefList
    for (ASTNode* def = comp->children->sibling->children; def != NULL; def = def->sibling->children) {
        for (ASTNode* dec = def->children->sibling->children; dec != NULL; dec = dec->sibling->sibling->children) {
            //Have assign
            if (dec->children->sibling != NULL) {
                Operand t1 = newOperand(TEMPVAR);
                translateExp(dec->children->sibling->sibling, t1);
                Symbol *s = getSymbol(varList, dec->children->children->text);
                assert(s->type.kind != array);
                if (s->varno == 0) {
                    s->varno = ++varNo;
                }
                Operand v1 = newOperand(VARIABLE, s, false);
                insertCode(newInterCode(Assign, v1, t1));
            }
            if (dec->sibling == NULL) {
                break;
            }
        }
        if (def->sibling->lineno == 0) {
            break;
        }
    }
    //Stmt
    for (ASTNode* stmt = comp->children->sibling->sibling->children; stmt != NULL; stmt = stmt->sibling->children) {
        translateStmt(stmt);
        if (stmt->sibling->lineno == 0) {
            break;
        }
    }
}



void translate(ASTNode* root) {
    //display(root, 0, stderr);
    for (ASTNode* extdef = root->children->children; ; extdef = extdef->sibling->children) {
        ASTNode* fundec = extdef->children->sibling;
        if (strcmp(fundec->type, "FunDec") == 0) {
            translateFundec(fundec);
            translateComp(fundec->sibling);
        }
        //Means the extdeflist is reduced to NULL
        if (extdef->sibling->lineno == 0) {
            break;
        }
    }
}

void getOp(FILE *stream, Operand op) {
	if(op == NULL)
		return;
	switch(op->kind) {
		case VARIABLE: {
			if(op->isPointer == true)
				fprintf(stream, "&v%d", op->u.no);
			else
				fprintf(stream, "v%d", op->u.no);
		} break;
		case TEMPVAR: {
			fprintf(stream, "t%d", op->u.no);
		} break;
		case CONSTANT: {
			fprintf(stream, "#%d", op->u.value);
		} break;
		case ADDRESS: {
			if(op->isPointer == true)
				fprintf(stream, "*t%d", op->u.no);
			else
				fprintf(stream, "t%d", op->u.no);
		} break;
		case LABEL: {
			fprintf(stream, "label%d", op->u.no);
		} break;
        default:{}
	}
}

void getInterCode(FILE *stream, InterCode node) {
	if(node == NULL)
		return;
	switch(node->kind) {
		case Assign: {
			getOp(stream, node->u.assign.left);
			fprintf(stream," := ");
			getOp(stream, node->u.assign.right);
		} break;
		case BINOP: {
			getOp(stream, node->u.binop.result);
			fprintf(stream," := ");
			getOp(stream, node->u.binop.op1);
			fprintf(stream," %s ", node->u.binop.kind);
			getOp(stream, node->u.binop.op2);
		} break;
		case UNARYOP: {
			fprintf(stream,"%s ", node->u.unaryop.kind);
			getOp(stream, node->u.unaryop.op);
			if(!strcmp(node->u.unaryop.kind, "LABEL"))
				fprintf(stream," :");
		} break;
		case If: {
			fprintf(stream,"IF ");
			getOp(stream, node->u.ifop.op1);
			fprintf(stream," %s ", node->u.ifop.relop);
			getOp(stream, node->u.ifop.op2);
			fprintf(stream," GOTO ");
			getOp(stream, node->u.ifop.op3);
		} break;
		case IRDEC: {
			fprintf(stream,"DEC ");
			getOp(stream, node->u.dec.op);
			fprintf(stream," %d", node->u.dec.size);
		} break;
		case CALL: {
			getOp(stream, node->u.call.result);
			fprintf(stream," := CALL %s", node->u.call.fun_name);
		} break;
		case FUNC: {
			fprintf(stream,"FUNCTION %s :", node->u.function);
		} break;
		case RET: {
			fprintf(stream,"RETURN ");
			getOp(stream, node->u.returnc.op);
		} break;
		case PARAM: {
			fprintf(stream,"PARAM ");
			getOp(stream, node->u.param.op);
		} break;
	}
	fprintf(stream," \n");
}

void printInterCode(char *filename) {
    
	if(codeListHead == NULL){
		printf("No valuable code\n");
		return;
	}
	FILE *out;
	InterCode temp = codeListHead->next;
	if ((out= fopen(filename, "w")) == NULL)
	{
		fprintf(stderr, "Cannot open file.\n");
		return;
	}
	printf("Output intercodes ...\n");
	while(temp != codeListTail) {
		getInterCode(out, temp);
		temp = temp->next;
	}
	printf("OK.\n");
}
