lex lexer.l
yacc -d grammar.y
gcc -g -o alma alma.c types.c errors.c anilib.c y.tab.c lex.yy.c util.c
