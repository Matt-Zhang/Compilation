#ifndef __ASTNODE_H__
#define __ASTNODE_H__
#include "common.h"

typedef struct FieldList_* FieldList;

struct StructList {
    FieldList fieldlist;
    struct StructList* next;
};
typedef struct StructList StructList;

struct Type
{
	enum {basic, array, structure} kind;
	union{
		int basic;
		struct {struct Type* elem; int size;} array;
		FieldList structure;
	} u;
};
typedef struct Type Type;

/* To register a kind of structure */
struct StructField{
    char name[MAX_NAME];     // The variable's name, i.e.:int name
    Type type;               // The type of this field, i.e.:type name
	struct StructField* next;
};
typedef struct StructField StructField;
struct FieldList_
{
	char name[MAX_NAME];     // The name of structure, i.e. struct name
	struct StructField *tail;
};

struct Symbol
{
	char name[MAX_NAME];
	Type type;
    struct Symbol* para;
	int lineno;
    struct Symbol* next;
	int varno; //the variable number
	bool isdec;//is already alloc memory space
	bool ispara;//is a para pass to a function.
};
typedef struct Symbol Symbol;


struct ASTNode{
	int terminal;            // 1=syntax,0=lexcial
	char type[MAX_NAME];     // Name of text
	char text[MAX_NAME];
	int lineno;
	struct ASTNode* sibling;
	struct ASTNode* children;
    Type ntype;
    bool left;               // If can be only used as left value
    
};
typedef struct ASTNode ASTNode;

char* getArg(Symbol* func);
char* checkArg(Symbol* func, ASTNode* exp);
void insertComp(ASTNode* deflist);
bool checkReturnValid(Type type, ASTNode* comp);
Symbol* getSymbol(Symbol* head, char *name);
bool isEqualType(Type* t1, Type* t2);
Type* checkInStruct(Type* type, char* name);
Type* handleExp(ASTNode* n1, ASTNode *n2);
void defFunc(ASTNode *spec, ASTNode* func);
void defStruct(char* name, ASTNode* deflist);
void printSym(Symbol *root);
void insertSymVar(ASTNode* spec, ASTNode* declist);
void storeSpecType(ASTNode* spec, char* typeName);
struct ASTNode * connTree(char *type, int num,...);
void display(struct ASTNode *t, int level, FILE *f);
#endif
