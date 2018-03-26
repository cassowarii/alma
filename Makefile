#CC=gcc
CFLAGS=-std=c99 -Wall -pedantic -g -D_GNU_SOURCE

ALMAREQS=ustrings.o symbols.o value.o ast.o stack.o scope.o eval.o parse.o lib.o grammar.tab.o lex.yy.o

all: alma test

alma: $(ALMAREQS) alma.o
	$(CC) $(CFLAGS) -o $@ $^

grammar.tab.c: grammar.y
	bison -d -v grammar.y

lex.yy.c: lexer.l
	flex lexer.l

%.o.c: %.c %.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o alma lex.yy.* grammar.tab.*

test_alma: $(ALMAREQS) test.o
	$(CC) $(CFLAGS) -o $@ $^ `pkg-config --cflags --libs check`

test: test_alma
	./test_alma
