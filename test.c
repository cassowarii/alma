#include "alma.h"

int main(int argc, char **argv) {
    init_types();
    init_library(&lib);
    node_t *nn = _node_lineno(N_COMPOSED,
                _node_lineno(N_BLOCK,
                    node_word("println", 2), NULL, 5
                ),
                node_word("copy", 4), 10);
                
    value_type *t = infer_type(nn);
    if (t->tag == V_ERROR) {
        print_error(t->content.err);
    } else {
        print_type(t);
    }
    free_type(t);
}
