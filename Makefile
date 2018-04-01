#CC=gcc-
CFLAGS=-std=c99 -Wall -pedantic -g -Og -D_GNU_SOURCE

ALMALIBS=lib_func.o lib_op.o lib_stack.o lib_control.o
ALMAREQS=ustrings.o symbols.o value.o ast.o stack.o scope.o eval.o $(ALMALIBS) lib.o registry.o vars.o compile.o grammar.tab.o lex.yy.o parse.o interactive.o

LIBS=-ledit

all: alma test

alma: $(ALMAREQS) alma.o
	$(CC) $(LDIRS) $(CFLAGS) -o $@ $^ $(LIBS)

grammar.tab.c: grammar.y
	bison -d -v grammar.y

lex.yy.c: lexer.l
	flex lexer.l

%.o.c: %.c %.h
	$(CC) $(LDIRS) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f *.o test_alma lex.yy.* grammar.tab.*

test_alma: $(ALMAREQS) test.o
	$(CC) $(LDIRS) $(CFLAGS) -o $@ $^ `pkg-config --cflags --libs check` $(LIBS)

test: test_alma
	@echo ""
	@echo "== SELF TEST =="
	./test_alma
