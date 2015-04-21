#ifndef __TRANSLATE_H__
#define __TRANSLATE_H__
#include "common.h"
#include "ASTNode.h"
typedef struct InterCode_* InterCode;
typedef struct Operand_* Operand; 

typedef enum {LABEL, VARIABLE, TEMPVAR, CONSTANT, ADDRESS, FALL} OperandKind;
typedef enum {Assign, If, BINOP, UNARYOP, IRDEC, FUNC, RET, PARAM, CALL} InterCodeKind;

struct Operand_ {
	OperandKind kind;
	union {
		int no;
		int value;
	}u;
	bool isPointer;
};
union U{ 
		struct { Operand right, left; } assign; 
		struct { char *kind; Operand result, op1, op2; } binop; 
		struct { char *kind; Operand op;} unaryop;
		struct { Operand op1; char *relop; Operand op2, op3;} ifop;
		struct { Operand op; int size;} dec;	
		struct { Operand result; char *fun_name;} call;
		struct { Operand op; } returnc;
		struct { Operand op; } param;
		char *function;
	}; 
struct InterCode_ {
	InterCodeKind kind;
	union U u;
	InterCode prev,next;
};



void getOp(FILE* stream, Operand op);
void translate(ASTNode* root);
Operand newOperand(OperandKind type, ...);
void getInterCode(FILE* stream, InterCode temp);
void printInterCode(char *filename);
#endif
