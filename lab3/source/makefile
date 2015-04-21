CFILES  = $(shell find . -name "*.c") 
OBJS    = $(CFILES:.c=.o)

parser: $(CFILES) lexical.l syntax.y test2.cmm
	flex lexical.l
	bison -d syntax.y
	flex lexical.l
	gcc translate.c main.c optimize.c ASTNode.c syntax.tab.c -g -std=c99 -ll -o parser
	./parser test2.cmm
	bash irsim.sh

opt: $(CFILES) lexical.l syntax.y test.cmm
	flex lexical.l
	bison -d syntax.y
	flex lexical.l
	gcc translate.c main2.c ASTNode.c syntax.tab.c optimize.c -g -std=c99 -ll -o parser2
	./parser2 test.cmm
	bash irsim.sh
	
compile:$(CFILES) lexical.l syntax.y test.cmm
	flex lexical.l
	bison -d syntax.y
	flex lexical.l
	gcc translate.c main.c ASTNode.c optimize.c syntax.tab.c -g -std=c99 -ll -o parser
	./parser test.cmm
clean:
	rm -f parser parser2
	rm *.o

