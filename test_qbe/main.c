#include "c.c"

#include "defs.h"

#include "lexer.c"

int main(void) {
    global_init();
    
    SOURCE->size = 0;
    SOURCE->elements = "int main(void) { return 0; }\0";
    next_char = SOURCE->elements[0];
    lex_expect(T_start);

    do {
        printf("%d\n", next_token);
        lex_accept(next_token);
    } while ((!lex_accept(T_eof)));

    return 0;
}
