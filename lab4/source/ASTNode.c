#ifndef __ASTNODE_C__
#define __ASTNODE_C__
#include "ASTNode.h"

struct Symbol *funcList = NULL;        //Function table
struct Symbol *varList = NULL;         // Variable table
struct StructList *structList = NULL;  // Declared struct type table
extern int error;
extern int yylineno;
/* From the func & var List to get the symbol using var's name */
Symbol* getSymbol(Symbol* head, char *name) {
    for (Symbol *p = head; p != NULL; p = p->next) {
        if (strncmp(p->name, name, MAX_NAME) == 0) {
            return p;
        }
    }
    return NULL;
}
/* Redefined symbol in varList */
bool isRedefinedSym(char *name) {
    for (Symbol *p = varList; p != NULL; p = p->next) {
        if (strncmp(p->name, name, MAX_NAME) == 0) {
            return true;
        }
    }
    return false;
}
/* Redefined in var & struct type list */
bool isRedefined(char *name) {
    if (isRedefinedSym(name)) {
        return true;
    }
    for (StructList *s = structList; s != NULL; s = s->next) {
        if (strncmp(name, s->fieldlist->name, MAX_NAME) == 0) {
            return true;
        }
    }
    return false;
}
/* The equality of two type, the struct's equality is space equivalence */
bool isEqualType(Type* t1, Type* t2) {
    if (t1->kind != t2->kind) {
        return false;
    }
    else if (t1->kind == basic) {
        if (t1->u.basic != t2->u.basic) {
            return false;
        }
    }
    else if (t1->kind == array) {
        int count_t1 = 0, count_t2 = 0;
        while(t1->kind == array) {
            t1 = t1->u.array.elem;
            count_t1++;
        }
        while (t2->kind == array) {
            t2 = t2->u.array.elem;
            count_t2++;
        }
        if (count_t2 != count_t1) { //Dimension equal
            return false;
        }
        else {
            return isEqualType(t1,t2);
        }
    }
    else {
        StructField *s1 = t1->u.structure->tail;
        StructField *s2 = t2->u.structure->tail;
        for (;;) { //Dimension Equal
            if (isEqualType(&s1->type, &s2->type)) {
                if (s1->next == NULL) {
                    if (s2->next == NULL) return true;
                    else return false;
                }
                else {
                    if (s2->next == NULL) {
                        return false;
                    }
                    else {
                        s1 = s1->next;
                        s2 = s2->next;
                    }
                }
            }
            else return false;
        }
    }
    return true;
}
/* Insert Symbol into a Symbol list */
bool insertSymbol(Symbol** head, Symbol *s) {
   // debug("In insertSymbol\n");
    if (isRedefined(s->name)) {
        printf("Error type 3 at line %d: Redefined varible \"%s\"\n",s->lineno,s->name);
        free(s);
 //       debug("Out insertSymbol\n");
        return false;
    }
    else {
        if (*head == NULL) {
            *head = s;
        }
        else {
            s->next = *head;
            *head = s;
        }
     //   debug("Out insertSymbol\n");
        return true;
    }
}
/* Return the Struct type with specific name */
FieldList findStruct(char* name) {
//    debug("In findStruct\n");
    for (StructList *p = structList; p != NULL; p = p->next) {
        if (strncmp(name, p->fieldlist->name, MAX_NAME) == 0) {
            return p->fieldlist;
        }
    }
//    debug("Out findStruct\n");
    return NULL;
}
/* use spec->ntype to hold the Type which has name of typeName */
void storeSpecType(ASTNode* spec, char* typeName) {
//    debug("In storeSpecType\n");
    if(strncmp(typeName, "int", 3) == 0) {
        spec->ntype.kind = basic;
        spec->ntype.u.basic = 1;
    }
    else if(strncmp(typeName, "float", 5) == 0) {
        spec->ntype.kind = basic;
        spec->ntype.u.basic = 2;
    }
    else {
        spec->ntype.kind = structure;
        spec->ntype.u.structure = findStruct(typeName);
    }
//    debug("Out storeSpecType\n");
}

/* Using the type of spec and the detail of var makes a symbol */
Symbol* varDectoSym(ASTNode* spec, ASTNode *var) {
 //   debug("In varDectoSym\n");
    Symbol *s = (Symbol*)malloc(sizeof(Symbol));
    memset(s, 0, sizeof(Symbol));
    strncpy(s->name, var->text, MAX_NAME);
    /* Have to reverse the current elem link because of the Exp[Exp] */
    if ((var->ntype).kind == array) {
        /* reverse */
        Type* temp = &var->ntype;
        Type* reverse = temp->u.array.elem;
        if (reverse->u.array.elem != NULL && reverse->kind == array) { // Not only one dimension
            Type* r3 = reverse->u.array.elem;
            temp->u.array.elem = &spec->ntype;
            do {
                reverse->u.array.elem = temp;
                temp = reverse;
                reverse = r3;
                if (r3->u.array.elem)
                    r3 = r3->u.array.elem;
            } while (reverse && reverse->kind == array);
            s->type = *temp;
        }
        else {
            s->type = var->ntype;
            s->type.u.array.elem = &spec->ntype;
        }
    }
    else{
        s->type = spec->ntype;
    }
    s->lineno = var->lineno;
//    debug("Out varDectoSym\n");
    return s;
}
/* Check if the field named *name* is in the struct *type* */
Type* checkInStruct(Type* type, char* name) {
    if (type->kind != structure) {
        printf("Error type 13 at line %d: Illegal use of \"%s\"\n", yylineno, name);
    }
    else {
        for (StructField* field = type->u.structure->tail; field != NULL; field = field->next) {
            if (strncmp(name, field->name, MAX_NAME) == 0) {
                return &(field->type);
            }
        }
        printf("Error type 14 at line %d: Un-existed field \" %s \"\n", yylineno, name);
    }
    return NULL;
}
/* Because of historical problem, this function only be used in Extdeflist. Insert a whole declist into varList */
void insertSymVar(ASTNode* spec, ASTNode* declist) {
   // debug("In insertSymVar\n");
    if (spec->ntype.kind == structure && spec->ntype.u.structure == NULL) {
        printf("Error type 17 at line %d: Undefined struct \"%s\"\n", spec->lineno, spec->text);
    }
    for (ASTNode *dec = declist->children; dec != NULL; dec = dec->sibling->sibling->children) {
        if (isRedefined(dec->text)) {
            printf("Error type 3 at line %d: Redefined varible \"%s\"\n",dec->lineno,dec->text);
            if(dec->sibling == NULL) break;
            else continue;
        }
        Symbol* s = varDectoSym(spec, dec);
        insertSymbol(&varList, s);
        if(dec->sibling == NULL)
            break;
    }
  //  debug("Out insertSymVar\n");
}

/* Insert the deflist into varList */
void insertComp(ASTNode* deflist) {
    if (deflist->lineno == 0) {
        return;
    }
    else {
        for (ASTNode* def = deflist->children; def != NULL; def = def->sibling->children) {
            ASTNode* spec = def->children;
             if (spec->ntype.kind == structure && spec->ntype.u.structure == NULL) {
                printf("Error type 17 at line %d: Undefined struct \"%s\"\n", spec->lineno, spec->text);
                 continue;
            }
            for (ASTNode* dec = spec->sibling->children; dec!= NULL; dec = dec->sibling->sibling->children) {
                if (isRedefined(dec->text)) {
                    printf("Error type 3 at line %d: Redefined varible \"%s\"\n",dec->lineno,dec->text);
                    if(dec->sibling == NULL) break;
                    else continue;
                }
                if (dec->children->sibling == NULL) { //No assign after VarDec
                    Symbol* s = varDectoSym(spec, dec->children);
                    insertSymbol(&varList, s);
                }
                else {  //Have assign after VarDec
                    /*
                    if (isEqualType(&spec->ntype, &dec->children->sibling->sibling->ntype)) {
                        Symbol* s = varDectoSym(spec, dec);
                        insertSymbol(&varList, s);
                    }
                    else {
                       	printf("Error type 5 at line %d: Type mismatched\n", dec->lineno);
                    }
                     */
                    //Fix bug 
                    Symbol* s = varDectoSym(spec, dec->children);
                    insertSymbol(&varList, s);
                }
                if (dec->sibling == NULL) {
                    break;
                }
            }
            
            if (def->sibling->lineno == 0) {
                    break;
            }
        }
    }
}
/* Need to remember, the para list is consist via next, bc function insertSymbol */
/* Do not check the stmt in if stmt or while stmt, only the basic check */
bool checkReturnValid(Type type, ASTNode* comp) {
    ASTNode* stmt = comp->children->sibling->sibling;
    if (stmt == NULL) {
        printf("Error type 8 at line %d: The return type mismatched \n",yylineno);
        return false;
    }
    for (stmt = stmt->children; stmt != NULL; stmt = stmt->sibling->children) {
        if (strncmp(stmt->children->type, "RETURN", 6) == 0) {
            if ((stmt->children->sibling->ntype.u.basic != -1) &&
                (memcmp(&(stmt->children->sibling->ntype), &type, sizeof(Type)) != 0)) {
                printf("Error type 8 at line %d: The return type mismatched \n", stmt->lineno);
                return false;
            }
        }
        
        if (stmt->sibling->lineno == 0) {
            break;
        }
    }
    return true;
}
/* Check if the two Exp have the same type and suitable for the operands like '+' or '*' */
Type* handleExp(ASTNode* n1, ASTNode *n2) {
    if (n1->ntype.u.basic == -1 || n2->ntype.u.basic == -1) {
        return NULL;
    }
    if(n1->ntype.kind != basic || n2->ntype.kind != basic) {
        printf("Error type 7 at line %d: Operands type mismatched\n", n1->lineno);
        return NULL;
    }
    if (isEqualType(&(n1->ntype), &(n2->ntype)) == true){
        return &(n1->ntype);
    }
    else {
        printf("Error type 7 at line %d: Operands type mismatched\n", n1->lineno);
        printf("Type is not equal\n");
        return NULL;
    }
}
/* Get str of a type */
/* Because in a reverse order, have to use array instead of stack to store the type */
char* getStr(Type *type, char* str) {
	memset(str, 0, 32);
    if (type->kind == basic) {
        if (type->u.basic == 1)
            memcpy(str, "int", 3);
        else
            memcpy(str, "float", 5);
    }
    else if(type->kind == array) {
        int count = 0;
        while (type->kind == array) {
            count++;
            type = type->u.array.elem;
        }
        str = getStr(type, str);
        while (count--) {
            strcat(str,"[]");
        }
    }
    else {
        memcpy(str, "struct ", 7);
        char* s = type->u.structure->name;
        if (s[0] < 'A')
            strcat(str, "Anonymous" );
        else
            strcat(str, s);
    }
    return str;
    
}
/* if the arg are same, return NULL; else return the exp list */
/* Because in a reverse order, have to use array instead of stack to store the type */
char* checkArg(Symbol* func, ASTNode* exp) {
    Type* exptype[MAX_ARG];
    memset(exptype, 0, sizeof(Type*) * MAX_ARG);
    char *str = (char*)malloc((MAX_NAME + 7) * MAX_ARG * sizeof(char));
    memset(str, 0, sizeof(char) * MAX_ARG * (MAX_NAME + 7));
    int count_exp = 0;
    
    for (ASTNode* exp2 = exp; exp2 != NULL; exp2 = exp2->sibling->sibling->children) {
        if (exp2->ntype.u.basic == -1) {
            /* for the free() part */
            strcpy(str, "The part before is wrong");
            return str;
        }
        exptype[count_exp++] = &exp2->ntype;
        if(exp2->sibling == NULL)
            break;
    }
    /* Generate the str of args into str */
    char type_str[MAX_NAME + 7] = "";
    int count = count_exp;
    do {
        strncat(str, getStr(exptype[--count], type_str), MAX_NAME + 7);
        if (count)
            strcat(str, ", ");
    }
    while (count);
    /* Check if equal to the para */
    for (Symbol* para = func->para;; para = para->next) {
        Type* type_para = &para->type;
        if (para == NULL) {
            if(count_exp == 0)
                return NULL;
            else
                return str;
        }
        else {
            if (count_exp == 0) {
                return str;
            }
            if (!isEqualType(type_para, exptype[--count_exp])) {
                return str;
            }
        }
    }
    return NULL;
}
/* Get str of arg of a function */
/* Because in a reverse order, have to use array instead of stack to store the type */
char* getArg(Symbol* func) {
    char str[MAX_ARG][MAX_NAME + 7];//7 for "struct"
    memset(&str[0][0], 0, MAX_ARG * (MAX_NAME + 7) * sizeof(char));
    char temp[MAX_NAME] = "";
    int count = 0;
    for (Symbol* para = func->para; para != NULL; para = para->next) {
        char s[MAX_NAME + 7];
		memset(s, 0, MAX_NAME + 7);
        strncpy(str[count], getStr(&para->type, s), MAX_NAME + 7);
        count++;
    }
    char* s = (char*)malloc(MAX_ARG * (MAX_NAME + 9) * sizeof(char));
    memset(s, 0, MAX_ARG * (MAX_NAME + 9) *sizeof(char));
    do {
        strncat(s, str[--count], MAX_NAME + 7);
        if (count)
        strcat(s, ", ");
    }
    while (count);
    return s;
}
/* Insert a func symbol into funcList */
void defFunc(ASTNode *spec, ASTNode* func) {
    //debug("In defFunc\n");
    if (spec->ntype.kind == structure && spec->ntype.u.structure == NULL) {
        printf("Error type 17 at line %d: Undefined struct \"%s\"  \n", spec->lineno, spec->text);
    }
    // Check Redefine
    for (Symbol *s = funcList; s != NULL; s = s->next) {
        if (strncmp(s->name, func->children->text, MAX_NAME) ==0) {
            printf("Error type 4 at line %d: Redefined function \"%s\"\n",func->lineno, s->name);
            return;
        }
    }
    
    Symbol *newFunc = (Symbol*)malloc(sizeof(Symbol));
    newFunc->type = spec->ntype;
    strncpy(newFunc->name, func->children->text, MAX_NAME);
    newFunc->lineno = func->lineno;
    // Register paras
    // Two copy of one symbol, one is inserted into varList, another into (Symbol*)func->para, next para is para->next
    bool succ = true;
    if (strncmp(func->children->sibling->sibling->type, "RP", 2) == 0) {
        newFunc->para = NULL;
    }
    else {
        for (ASTNode* param = func->children->sibling->sibling->children; param != NULL; param = param->sibling->sibling->children) {
            Symbol *para = varDectoSym(param->children, param->children->sibling);
            succ = insertSymbol(&newFunc->para, para);
            if (succ) {
                Symbol *para2 = (Symbol*)malloc(sizeof(Symbol));
                memcpy(para2, para, sizeof(Symbol));
                insertSymbol(&varList, para2);
            }
            else {
                while (newFunc->para != NULL) {
                    Symbol *t = newFunc->para;
                    newFunc->para = newFunc->para->next;
                    free(t);
                }
                break;
            }
            if(param->sibling == NULL)
                break;
        }
    }
    if (!succ) {
        free(newFunc);
    }
    else {
        insertSymbol(&funcList, newFunc);
    }
    //debug("Out defFunc\n");
}

void defStruct(char *name, ASTNode* deflist) {
    //debug("In defStruct\n");
    if (isRedefined(name)) {
       printf("Error type 16 at line %d: Duplicated name \"%s\"\n", yylineno, name);
        return;
    }
    FieldList newStruct = (FieldList)malloc(sizeof(struct FieldList_));
    memset(newStruct, 0, sizeof(struct FieldList_));
    strncpy(&(newStruct->name[0]), name, MAX_NAME);
    
    for (ASTNode* def = deflist->children; def != NULL; def = def->sibling->children) {
        ASTNode* spec = def->children;
        for (ASTNode* dec = spec->sibling->children; dec != NULL; dec = (dec->sibling->sibling->children) ) {
            ASTNode* var = dec->children;
            if (var->sibling != NULL) {
                if (strcmp(var->sibling->type, "ASSIGNOP") == 0) {
                    printf("Error type 15 at line %d: \"%s\" can not be initialed or redefined\n", var->lineno, var->text);
                }
            }
            Symbol *s = varDectoSym(spec, var); //Only use it's type
            // Check repeated filed name within struct
            bool repeated = false;
            for (StructField *s2 = newStruct->tail; s2 != NULL; s2 = s2->next) {
                if (strncmp(s->name, s2->name, MAX_NAME) == 0) {
                    printf("Error type 15 at line %d: \"%s\" can not be initialed or redefined\n", var->lineno, var->text);
                    free(s);
                    repeated = true;
                    break;
                }
            }
            if (repeated) {
                break;
            }
            // Insert into the struct list
            struct StructField* field = (struct StructField*)malloc(sizeof(struct StructField));
            memset(field, 0, sizeof(StructField));
            strncpy(field->name, s->name, MAX_NAME);
            field->type = s->type;
            if(newStruct->tail == NULL) {
                newStruct->tail = field;
            }
            else {
                field->next = newStruct->tail;
                newStruct->tail = field;
            }
            free(s);
            if (dec->sibling == NULL) {
                break;
            }
        }
        if (def->sibling->lineno == 0) {
            break;
        }
    }
    /* Insert into the whole defined struct list */
    StructList *slist = (StructList*)malloc(sizeof(StructList));
    memset(slist, 0, sizeof(StructList));
    slist->fieldlist = newStruct;
    if (structList == NULL) {
        structList = slist;
    }
    else {
        slist->next = structList;
        structList = slist;
    }
    //debug("Out defStruct\n");
}





ASTNode* connTree(char *type,int num,...) {
	int i;
	struct ASTNode *cur = (struct ASTNode *)malloc(sizeof(struct ASTNode));
    memset(cur, 0, sizeof(ASTNode));
	cur->terminal = 1;
    /* Get the arg list */
  	va_list ap;
  	va_start(ap, num);
    ASTNode *var = va_arg(ap, struct ASTNode*);
	cur->lineno = var->lineno;
	strncpy(cur->type, type, 31);
	cur->children = var;
	if(num == 1) {
		var->sibling=NULL;
    }
    else {
        for(i = 1; i < num; i++) {
            var->sibling= va_arg(ap, struct ASTNode*);
            if(var->sibling == NULL)
                continue;
            var = var->sibling;
        }
        var->sibling=NULL;
    }
  	va_end(ap);
	return cur;
}

/* Display the ASTtree as required */
void display(struct ASTNode *t, int level, FILE *f)
{
	int i;
	if(t == NULL)
        return ;
	for(i = 0; i < level; i++)
		fprintf(f, "  ");
	if(t->terminal == 0) {
		if(strcmp(t->type, "TYPE")==0||strcmp(t->type, "ID")==0)
			fprintf(f,"%s: %s\n", t->type, t->text);
		else if(strcmp(t->type,"INT") == 0)
			fprintf(f,"INT: %d\n", *(int*)(t->text));
		else if(strcmp(t->type,"FLOAT") == 0)
			fprintf(f,"FLOAT: %f\n", *(float*)(t->text));
		else fprintf(f,"%s\n", t->type);
	}
	else if(t->terminal == 1) {
		fprintf(f,"%s (%d)\n", t->type, t->lineno);
		display(t->children, level + 1, f);
	}
	display(t->sibling,level,f);
}
#endif
