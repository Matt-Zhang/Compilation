#include "translate.h"
#include "genasm.h"
#include "common.h"

extern InterCode codeListHead;
extern InterCode codeListTail;
SPADD spaddHead;
int sp;//The relative place to right now $fp
FILE* stream;

struct Stack_Func {
	int pointer;
	int sp[128];
}stack_func;
typedef struct Stack_Func Stack_Func;

struct Stack_Arg {
	int pointer;
	Operand op[32];
}stack_arg;
typedef struct Stack_Arg Stack_Arg;

void push(Stack_Func *s, int c) {
    s->sp[++s->pointer] = c;
}

int pop(Stack_Func *s) {
	if(s->pointer == -1) {
		return -1;
    }
	return s->sp[s->pointer--];
}
//Because C cannot have same function name
void pusha(Stack_Arg *s, Operand c) {
    s->op[++s->pointer] = c;
}

Operand popa(Stack_Arg *s) {
	if(s->pointer == -1) {
		return NULL;
    }
	return s->op[s->pointer--];
}

int searchSPADDiter(Operand op){
	for(SPADD head = spaddHead; head != NULL; head = head->next){
		if((head->op->kind == op->kind && head->op->u.no == op->u.no) ||
           (head->op->kind == ADDRESS && head->op->u.no == op->u.no)  ||
           (op->kind == ADDRESS && head->op->u.no == op->u.no))
            return head->addr;
	}
    return -1;
}

int searchSPADD(Operand op) {
    int r = searchSPADDiter(op);
    if (r == -1) {
        debug("The result of searchSPADD is -1!!!\n");
        //exit(-1);
    }
    return r;
}
//All use the relative location to $fp
void insertSPADD(Operand op){
	if(searchSPADDiter(op) == -1){
		SPADD tspadd = (SPADD)malloc(sizeof(struct _SPADD));
        memset(tspadd, 0, sizeof(struct _SPADD));
		tspadd->op = op;
		sp = sp - 4;
		tspadd->addr = sp;
		tspadd->next = spaddHead;
		spaddHead = tspadd;
		fprintf(stream,"sw $t1 %d($fp)\n", tspadd->addr);
	}
	else{
		fprintf(stream,"sw $t1 %d($fp)\n",searchSPADDiter(op));
	}
}


void getCode(InterCode node) {
	if(node == NULL)
		return;
	switch(node->kind) {
		case Assign: {
            Operand r = node->u.assign.right;
            Operand l = node->u.assign.left;
            if (r->kind == CONSTANT) {
                if (l->isPointer == false) {
                    fprintf(stream, "li $t1, %d\n", r->u.value);
                    insertSPADD(l);
                }
                else {
                    fprintf(stream, "li $t2 %d\n", r->u.value);
                    fprintf(stream, "lw $t3 %d($fp)\n", searchSPADD(l));
                    fprintf(stream, "add $t3, $t3, $fp\n");
                    fprintf(stream, "sw $t2 0($t3)\n");
                }
            }
            else {
                if (l->isPointer == false) {
                    //t=a
                    if (r->isPointer == false) {
                        fprintf(stream, "lw $t1 %d($fp)\n", searchSPADD(r));
                        insertSPADD(l);
                    }
                    //t=&a
                    else {
                        fprintf(stream, "li $t1, %d\n", searchSPADD(r));
                        insertSPADD(l);
                    }
                }
                //*t=a
                else {
                    fprintf(stream, "lw $t2 %d($fp)\n", searchSPADD(r));
                    fprintf(stream, "lw $t3 %d($fp)\n", searchSPADD(l));
                    fprintf(stream, "add $t3, $t3, $fp\n");
                    fprintf(stream, "sw $t2 0($t3)\n");
                }
            }
            break;
        }
        case BINOP: {
            Operand op1 = node->u.binop.op1;
            Operand op2 = node->u.binop.op2;
            char kind = node->u.binop.kind[0];
            //Load correct data to $t2 and $t3
            if (op1->kind == CONSTANT) {
                fprintf(stream, "li $t2 %d\n", op1->u.value);
            }
            else if(op1->isPointer == false){
                fprintf(stream, "lw $t2 %d($fp)\n", searchSPADD(op1));
            }
            else {
               // t = a + *t
                if (op1->kind == ADDRESS) {
                    fprintf(stream, "lw $t2 %d($fp)\n", searchSPADD(op1));
                    fprintf(stream, "add $t2, $t2, $fp\n");
                    fprintf(stream, "lw $t2 0($t2)\n");
                }
                else {
                    // t = a + &b
                    fprintf(stream, "li $t2, %d\n", searchSPADD(op1));
                }
            }
           
            if (op2->kind == CONSTANT) {
                fprintf(stream, "li $t3 %d\n", op2->u.value);
            }
            else if(op2->isPointer == false){
                fprintf(stream, "lw $t3 %d($fp)\n", searchSPADD(op2));
            }
            else {
               // t = a + *t
                if (op2->kind == ADDRESS) {
                    fprintf(stream, "lw $t3 %d($fp)\n", searchSPADD(op2));
                    fprintf(stream, "add $t3, $t3, $fp\n");
                    fprintf(stream, "lw $t3 0($t3)\n");
                }
                else {
                    // t = a + &b
                    fprintf(stream, "li $t3, %d\n", searchSPADD(op2));
                }
            }
            
            
            if (kind == '+') {
                fprintf(stream, "add $t1, $t2, $t3\n");
            }
            else if (kind == '-') {
                fprintf(stream, "sub $t1, $t2, $t3\n");
            }
            else if (kind == '*') {
                fprintf(stream, "mul $t1, $t2, $t3\n");
            }
            else if(kind == '/') {
                fprintf(stream, "div $t2, $t3\n");
                fprintf(stream, "mflo $t1\n");
            }
            insertSPADD(node->u.binop.result);
            break;
        }
        
        case UNARYOP: {
            Operand op = node->u.unaryop.op;
            char* kind = node->u.unaryop.kind;
            if (strcmp(kind, "LABEL") == 0) {
                fprintf(stream, "Label%d:\n", op->u.no);
            }
            else if(strcmp(kind, "GOTO") == 0) {
                fprintf(stream, "j Label%d\n", op->u.no);
            }
            else if(strcmp(kind, "READ") == 0) {
                fprintf(stream, "li $v0, 4\n");
                fprintf(stream, "la $a0, _prompt\n");
                fprintf(stream, "syscall\n");
                fprintf(stream, "li $v0, 5\n");
                fprintf(stream, "syscall\n");
                fprintf(stream, "move $t1, $v0\n");
                insertSPADD(node->u.unaryop.op);
            }
            else if(strcmp(kind, "WRITE") == 0) {
                if (node->u.unaryop.op->kind == CONSTANT) {
                    fprintf(stream, "li $a0 %d\n", node->u.unaryop.op->u.value);
                }
                else if(node->u.unaryop.op->isPointer == false) {
                    fprintf(stream,"lw $a0 %d($fp)\n",searchSPADD(node->u.unaryop.op));
                }
                else{
                    fprintf(stream,"lw $t1 %d($fp)\n",searchSPADD(node->u.unaryop.op));
                    fprintf(stream,"add $t3, $t1, $fp\n");
                    fprintf(stream,"lw $a0, 0($t3)\n");
                }
                fprintf(stream, "li $v0, 1\n");
                fprintf(stream, "syscall\n");
                fprintf(stream, "li $v0, 4\n");
                fprintf(stream, "la $a0, _ret\n");
                fprintf(stream, "syscall\n");
            }
            else if(strcmp(kind, "ARG") == 0) {
                pusha(&stack_arg, op);
            }
            break;
        }
            
        case If: {
            Operand op1 = node->u.ifop.op1;
            Operand op2 = node->u.ifop.op2;
            char* relop = node->u.ifop.relop;
            //Load correct data
            if (op1->kind == CONSTANT) {
                fprintf(stream, "li $t2 %d\n", op1->u.value);
            }
            else if(op1->isPointer == false){
                fprintf(stream, "lw $t2 %d($fp)\n", searchSPADD(op1));
            }
            else {
                // t = a + *t
                if (op1->kind == ADDRESS) {
                    fprintf(stream, "lw $t2 %d($fp)\n", searchSPADD(op1));
                    fprintf(stream, "add $t2, $t2, $fp\n");
                    fprintf(stream, "lw $t2 0($t2)\n");
                }
                else {
                    // t = a + &b
                    fprintf(stream, "li $t2, %d\n", searchSPADD(op1));
                }
            }
            
            if (op2->kind == CONSTANT) {
                fprintf(stream, "li $t3 %d\n", op2->u.value);
            }
            else if(op2->isPointer == false){
                fprintf(stream, "lw $t3 %d($fp)\n", searchSPADD(op2));
            }
            else {
                // t = a + *t
                if (op2->kind == ADDRESS) {
                    fprintf(stream, "lw $t3 %d($fp)\n", searchSPADD(op2));
                    fprintf(stream, "add $t3, $t3, $fp\n");
                    fprintf(stream, "lw $t3 0($t3)\n");
                }
                else {
                    // t = a + &b
                    fprintf(stream, "li $t3, %d\n", searchSPADD(op2));
                }
            }
            
            if (strcmp(relop, "==") == 0) {
                fprintf(stream, "beq $t2, $t3, ");
            }
            else if (strcmp(relop, "!=") == 0) {
                fprintf(stream, "bne $t2, $t3, ");
            }
            else if (strcmp(relop, ">") == 0) {
                fprintf(stream, "bgt $t2, $t3, ");
            }
            else if (strcmp(relop, "<") == 0) {
                fprintf(stream, "blt $t2, $t3, ");
            }
            else if (strcmp(relop, ">=") == 0) {
                fprintf(stream, "bge $t2, $t3, ");
            }
            else if (strcmp(relop, "<=") == 0) {
                fprintf(stream, "ble $t2, $t3, ");
            }
            fprintf(stream, "Label%d\n", node->u.ifop.op3->u.no);
            break;
        }
        
        case IRDEC: {
            SPADD add = (SPADD)malloc(sizeof(struct _SPADD));
            memset(add, 0, sizeof(struct _SPADD));
            add->op = node->u.dec.op;
            sp = sp -node->u.dec.size;
            add->addr = sp;
            add->next = spaddHead;
            spaddHead = add;
            break;
        }
            
        case CALL: {
            //Save the current sp
            push(&stack_func, sp);
            
            //Set arguments on the stack
            Operand arg = popa(&stack_arg);
            while (arg != NULL) {
                if (arg->kind != CONSTANT) {
                    //ARG *t i.e. func(a[5])
                    if (arg->isPointer == true) {
                        fprintf(stream, "lw $t1 %d($fp)\n", searchSPADD(arg));
                        fprintf(stream, "add $t1, $t1, $fp\n");
                        fprintf(stream, "lw $t1 0($t1)\n");
                    }
                    else {
                        fprintf(stream, "lw $t1 %d($fp)\n", searchSPADD(arg));
                    }
                }
                else {
                    fprintf(stream, "li $t1, %d\n", arg->u.value);
                }
                sp = sp - 4;
                fprintf(stream, "sw $t1 %d($fp)\n", sp);
                arg = popa(&stack_arg);
            }
            //Save $fp and $ra
            sp -= 4;
            fprintf(stream, "sw $fp %d($fp)\n", sp);
            sp -= 4;
            fprintf(stream, "sw $ra %d($fp)\n", sp);
            
            fprintf(stream, "addi $fp, $fp, %d\n", sp);
            fprintf(stream, "jal _%s\n", node->u.call.fun_name);
            //Restore
            sp = pop(&stack_func);
            fprintf(stream, "lw $ra 0($fp)\n");
            fprintf(stream, "lw $fp 4($fp)\n");
            //Deal with result
            fprintf(stream, "move $t1, $v0\n");
            insertSPADD(node->u.call.result);
            break;
        }
        case FUNC: {
            if (strcmp(node->u.function, "main") == 0) {
                fprintf(stream, "%s:\n", node->u.function);
                fprintf(stream, "move $fp $sp\n");
            }
            //Afraid of the function named "sub", which is the same with instruction sub
            else {
                fprintf(stream, "_%s:\n", node->u.function);
            }
            fprintf(stream, "addi $sp, $sp, -2048\n");
            sp = 0;
            break;
        }
        case RET: {
            //$sp is not used at all
            fprintf(stream, "addi $sp, $sp, 2048\n");
            if (node->u.returnc.op->kind == CONSTANT) {
                fprintf(stream, "li $v0 %d\n", node->u.returnc.op->u.value);
            }
            else {
                fprintf(stream,"lw $v0 %d($fp)\n",searchSPADD(node->u.returnc.op));
                if(node->u.returnc.op->isPointer == 1){
                    fprintf(stream,"add $t3, $tv0, $fp\n");
                    fprintf(stream,"lw $v0, 0($t3)\n");
                }
            }
			fprintf(stream,"jr $ra");
            break;
        }
        case PARAM: {
            int num_a = 0;
            for (InterCode t = node->next; t->kind == PARAM; t = t->next) {
                num_a++;
            }
            num_a += 2;//$ra And $fp
            fprintf(stream, "lw $t1 %d($fp)\n", num_a * 4);
            insertSPADD(node->u.call.result);
            break;
        }

    }
}

void printCode(char *filename) {
    stack_func.pointer = -1;
    stack_arg.pointer = -1;
    spaddHead = NULL;
    sp = 0;
	if(codeListHead == NULL) {
		printf("No valuable code\n");
		return;
	}
	
	InterCode temp = codeListHead->next;
	if ((stream= fopen(filename, "w")) == NULL) {
		fprintf(stderr, "Cannot open file.\n");
		return;
	}
	fprintf(stream, ".data\n");
	fprintf(stream, "_prompt: .asciiz \"Enter an integer:\"\n");
	fprintf(stream, "_ret: .asciiz \"\\n\"\n");
	fprintf(stream, ".globl main\n");
	fprintf(stream, ".text\n");
    for (InterCode temp = codeListHead->next; temp != codeListTail; temp = temp ->next) {
        fprintf(stream, "#");
        getInterCode(stream, temp);
		getCode(temp);
        fprintf(stream, "\n");
	}
	printf("OK.\n");
}
