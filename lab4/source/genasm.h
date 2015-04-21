#ifndef __GENASM_H__
#define __GENASM_H__
void printCode(char* filename);

typedef struct _SPADD*  SPADD;
struct _SPADD
{
	Operand op;
	int addr;
	SPADD next;
};
#endif

