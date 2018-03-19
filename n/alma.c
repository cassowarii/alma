#include <stdio.h>
#include "grammar.tab.h"

int main (int argc, char **argv) {
    yyparse();
}
