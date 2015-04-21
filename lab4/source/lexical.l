%option yylineno
yylineno = 1;
%{
#include "syntax.tab.h"
	#include "ASTNode.h"	
	#include "common.h"
	void initTree(char* type, char* text);
	void initIntTree(char* type, char* text);
	void handleError();
	void initXIntTree(char* type, char* text);
	void initHIntTree(char* type, char* text);
	void initFloatTree(char* type, char* text);
	extern int error;



%}

enter  	(\n)|(\r)
ws	[\t]
blank	[ ]+
letter  [A-Za-z]
digit  	[0-9] 
type	int|float
id     	({letter}|_)({letter}|{digit}|_)*

x_digit	[0-9A-Fa-f]
o_digit	[0-7]
int     ([1-9][0-9]*)|0
int_x	(0x|0X){x_digit}+
int_h	0(([1-7][0-7]*)|0)
float   [1-9][0-9]*\.{digit}+|0\.{digit}+

relop	>|<|>=|<=|==|!=


%%
{ws}		{}
{blank}		{}
{enter}		{}
{int_h}         {initHIntTree("INT",yytext);return INT;}
{int}           {initIntTree("INT",yytext);return INT;}
{int_x}         {initXIntTree("INT",yytext);return INT;}
{float}         {initFloatTree("FLOAT",yytext);return FLOAT;}
{type}	        {initTree("TYPE",yytext);return TYPE;}
struct		{initTree("STRUCT",yytext);return STRUCT;}
return       	{initTree("RETURN",yytext);return RETURN;}
if         	{initTree("IF",yytext);return IF;}
else         	{initTree("ELSE",yytext);return ELSE;}
while	        {initTree("WHILE",yytext);return WHILE;}
{relop}	        {initTree("RELOP",yytext);return RELOP;}
{id}		{initTree("ID",yytext);return ID;}
","		{initTree("COMMA",yytext);return COMMA;}
"="		{initTree("ASSIGNOP",yytext);return ASSIGNOP;}
"+"		{initTree("PLUS",yytext);return PLUS;}
"-"		{initTree("MINUS",yytext);return MINUS;}
"*"		{initTree("STAR",yytext);return STAR;}
"/"		{initTree("DIV",yytext);return DIV;}
"||"	{initTree("OR",yytext);return OR;}
"&&"		{initTree("AND",yytext);return AND;}
"."		{initTree("DOT",yytext);return DOT;}
"!"		{initTree("NOT",yytext);return NOT;}
";"		{initTree("SEMI",yytext);return SEMI;}
"("		{initTree("LP",yytext);return LP;}
")"		{initTree("RP",yytext);return RP;}
"["		{initTree("LB",yytext);return LB;}
"]"		{initTree("RB",yytext);return RB;}
"{"		{initTree("LC",yytext);return LC;}
"}"		{initTree("RC",yytext);return RC;}
.		{handleError();}

%%
int intpow(int base, int times) {
	int sum = 1;
	for(; times > 0; times--) {
		sum *= base;
	}
	return sum;
}

int atoxtoi(char* text) {
	int i = 2, sum = 0, temp = 0;
	for(; i < strlen(text); i++) {
		char c = *(text + i);
		if(c >= 'a')
			temp = c - 'a' + 10;
		else if(c >= 'A') 
			temp = c - 'A' + 10;
		else
			temp = c - '0';
		sum += temp * intpow(16, strlen(text) - i - 1);
	}
	return sum;
}

int atohtoi(char * text) {
	int i = 1, sum = 0;
	for(; i < strlen(text); i++) {
		sum += (*(text + i) - '0') * intpow(8, strlen(text) - i - 1);
	}
	return sum;
}


void handleError() {
/*
	if(yytext[0] == '0' && (yytext[1] == 'x' || yytext[1] == 'X')) {
	printf("Error type A at line %d: Illegal hexadecimal number \'%s\'\n",yylineno,yytext);
	error++;
	return;
	}
	if(yytext[0] == '0') {
	printf("Error type A at line %d: Illegal octal number \'%s\'\n",yylineno,yytext);
	}
	else {
*/	printf("Error type A at line %d: Mysterious character \'%s\'\n",yylineno,yytext);
//	}
	error++;
}
/* Add the common info to yylval and pass the values to syntax.y, is a basic func used by following funcs */
void initTreeCommon(char* type, char* text) {
	yylval.iNode = (struct ASTNode *)malloc(sizeof(struct ASTNode));	
	memset(yylval.iNode, 0, sizeof(struct ASTNode));
	yylval.iNode->terminal = 0;
	yylval.iNode->lineno = yylineno;
    strncpy(yylval.iNode->type, type, MAX_NAME);	
	yylval.iNode->children = NULL;
	yylval.iNode->sibling = NULL;	
}

/* Init the normal tree, and others are for the int or float */
void initTree(char* type,char* text)
{
	initTreeCommon(type, text);
	strncpy(yylval.iNode->text, text, MAX_NAME);
//	printf("HERE%s\n", text);
}

void initIntTree(char* type, char* text) {
	initTreeCommon(type, text);
	int temp = atoi(text);
	memcpy(yylval.iNode->text, &temp, sizeof(int));
//	printf("%s:%d\n", yylval.iNode->type, *(int*)yylval.iNode->text);
}
void initXIntTree(char* type, char* text) {
	initTreeCommon(type, text);
	int temp = atoxtoi(text);
	memcpy(yylval.iNode->text, &temp, sizeof(int));
//	printf("HEREX%d\n", temp);
}
void initHIntTree(char* type, char* text) {
	initTreeCommon(type, text);
	int temp = atohtoi(text);
//	printf("HEREH%d\n", temp);
	memcpy(yylval.iNode->text, &temp, sizeof(int));
}
void initFloatTree(char* type, char* text) {
	initTreeCommon(type, text);
	float temp = atof(text);
	memcpy(yylval.iNode->text, &temp, sizeof(float));
//	printf("HEREF%f\n", temp);
}


 int yywrap()
 {
	return 1;
 }


