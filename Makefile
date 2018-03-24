CC=gcc
CFLAGS=-std=c99 -Wall -pedantic -g -D_GNU_SOURCE

alma: ustrings.o symbols.o value.o ast.o stack.o grammar.tab.o lex.yy.o alma.o
	$(CC) $(CFLAGS) -o $@ $^

grammar.tab.c: grammar.y
	bison -d -v grammar.y

lex.yy.c: lexer.l
	flex lexer.l

%.o.c: %.c %.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o alma lex.yy.* grammar.tab.*
