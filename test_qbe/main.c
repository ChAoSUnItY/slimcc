#include "config"

#include "c.c"

#include "defs.h"

#include "lexer.c"

/* #include "elf.c"

#include "parser.c" */

/* typedef struct {
    char name[32];
} var_t;

typedef struct {
    var_t return_def;
} func_t; */

int main(int argc, char *argv[]) {
    /* global_init();
    
    parse("test_qbe/test.c"); */
    printf("KEK\n");

    func_t *func = 0;
    char *var_name = func->return_def.var_name;

    return 0;
}
