CFILES  = $(shell find . -name "*.c") 
OBJS    = $(CFILES:.c=.o)

parser: $(CFILES) lexical.l syntax.y test.cmm
	flex lexical.l
	bison -d syntax.y
	flex lexical.l
	gcc translate.c genasm.c main.c optimize.c ASTNode.c syntax.tab.c -g -std=c99 -ll -o parser
	./parser test.cmm
	cp code.s ~/Documents/c.s
	
compile:$(CFILES) lexical.l syntax.y test.cmm
	flex lexical.l
	bison -d syntax.y
	flex lexical.l
	gcc genasm.c translate.c main.c ASTNode.c optimize.c syntax.tab.c -g -std=c99 -ll -o parser
clean:
	rm -f parser parser2
