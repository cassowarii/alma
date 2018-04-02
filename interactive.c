#include "interactive.h"

void interactive_mode() {
    printf("Alma version "ALMA_VERSION" “"ALMA_VNAME"” "
           "["__DATE__", "__TIME__"]\n");

    AInteractive* inter_state = malloc(sizeof(AInteractive));
    inter_state->is_interactive = 1;
    inter_state->at_eof = 0;
    inter_state->beginning = 1;

    yyscan_t scanner;
    yylex_init(&scanner);
    yyset_extra(inter_state, scanner);

    char *p = NULL;

    char whole_line[2048]; // max line length

    struct yy_buffer_state *buf = NULL;

    do {
        ADeclSeqNode *program = NULL;
        ASymbolTable symtab = NULL;

        /* set whole_line to the empty string */
        whole_line[0] = 0;

        inter_state->beginning = 1;
        do {
            /* Reset everything to do a new scan. */
            inter_state->nested_comments = 0;
            inter_state->nested_blocks = 0;
            inter_state->nested_lists = 0;
            inter_state->nested_parens = 0;
            inter_state->nested_colons = 0;

            if (inter_state->beginning) {
                p = readline("alma> ");
            } else {
                p = readline("... > ");
            }
            if (p == NULL) break;

            strcat(whole_line, p);
            printf("whole line: '%s'\n", whole_line);

            /* editline doesn't give a newline at end of input
             * so we have to add it ourselves */
            strcat(whole_line, "\n");

            buf = yy_scan_string(whole_line, scanner);
            yy_switch_to_buffer(buf, scanner);
            /* Parse the string so far to look for syntax errors.
             * This calls yylex which will let us know if we should
             * keep adding stuff or not. */
            if (buf != NULL) yy_flush_buffer(buf, scanner);
            yyparse(scanner, &program, &symtab);
        } while (!inter_state->beginning);
    } while (p != NULL);

    yylex_destroy(scanner);
}
