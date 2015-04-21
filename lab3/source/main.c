#include <stdio.h>
#include "syntax.tab.h"
#include "translate.h"
#include "optimize.h"
extern struct ASTNode* root ;
extern int error;

int main(int argc, char** argv)
{ 
  	if (argc <= 1)
        return -1;
	FILE* f = fopen(argv[1], "r"); 
  	if (!f) {
    		perror(argv[1]); 
    		return 1; 
  	}
  	yyrestart(f); 
  	yyparse();
	//if(error == 0)
	//	display(root, 0, stdout);
	if(error == 0)
		translate(root);
	optimize();
	printInterCode("./code.ir"); 
  	return 0; 
} 
