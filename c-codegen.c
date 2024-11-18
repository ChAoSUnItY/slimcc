#include "slimcc.h"

FILE *output_file;

int indent = 0;
bool newline = true;

FMTCHK(1, 2)
void print(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (newline)
    {
        for (int i = 0; i < indent; i++)
            fprintf(output_file, "  ");
        newline = false;
    }
    vfprintf(output_file, fmt, ap);
    va_end(ap);
}

FMTCHK(1, 2)
void println(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(output_file, fmt, ap);
    va_end(ap);
    fprintf(output_file, "\n");
    newline = true;
}

bool is_string(Type *ty)
{
    return (ty->kind == TY_ARRAY || ty->kind == TY_PTR) && ty->base && ty->base->kind == TY_PCHAR;
}

/* To match original declaration sequence */
Obj *reverse(Obj *prog)
{
    Obj *cur = prog, *prev = NULL, *next;

    while (cur != NULL)
    {
        next = cur->next;
        cur->next = prev;

        prev = cur;
        cur = next;
    }

    return prev;
}

void emit_type(Type *typ)
{
    switch (typ->kind)
    {
    case TY_VOID:
    {
        print("void ");
        break;
    }
    case TY_BOOL:
    {
        print("bool ");
        break;
    }
    case TY_PCHAR:
    case TY_CHAR:
    {
        print("char ");
        break;
    }
    case TY_SHORT:
    {
        print("short ");
        break;
    }
    case TY_INT:
    case TY_ENUM:
    {
        print("int ");
        break;
    }
    case TY_LONG:
    {
        error("Unsupported data type \"long\"");
        print("long ");
        break;
    }
    case TY_LONGLONG:
    {
        error("Unsupported data type \"long long\"");
        print("long long ");
        break;
    }
    case TY_FLOAT:
    {
        error("Unsupported data type \"long float\"");
        print("float ");
        break;
    }
    case TY_DOUBLE:
    {
        error("Unsupported data type \"long double\"");
        print("double ");
        break;
    }
    case TY_LDOUBLE:
    {
        error("Unsupported data type \"long doublie\"");
        print("long double ");
        break;
    }
    case TY_PTR:
    {
        int level = 0;

        while (typ->base)
        {
            level++;
            typ = typ->base;
        }

        emit_type(typ);

        for (int i = 0; i < level; i++)
            print("*");
        break;
    }
    case TY_VLA:
    {
        error("Unsupported data type \"variable length array\"");
        break;
    }
    case TY_FUNC:
    {
        emit_type(typ->return_ty);
        break;
    }
    case TY_ARRAY:
    {
        emit_type(typ->base);
        break;
    }
    default:
        break;
    }
}

void emit_type_suffix(Type *typ)
{
    switch (typ->kind)
    {
    case TY_ARRAY:
    {
        print("[%d]", (int)typ->array_len);
        emit_type_suffix(typ->base);
        break;
    }
    default:
        break;
    }
}

void emit_type_with_id(Type *typ, char *id)
{
    emit_type(typ);
    print("%s", id);
    emit_type_suffix(typ);
}

void emit_string(char *data, int size)
{
    char *s = malloc(size * 2 + 1), c;
    int i = 0, si = 0;

    while (data[i])
    {
        c = data[i];

        switch (c)
        {
        case '\a':
            s[si++] = '\\';
            s[si++] = 'a';
            break;
        case '\b':
            s[si++] = '\\';
            s[si++] = 'b';
            break;
        case '\t':
            s[si++] = '\\';
            s[si++] = 't';
            break;
        case '\n':
            s[si++] = '\\';
            s[si++] = 'n';
            break;
        case '\v':
            s[si++] = '\\';
            s[si++] = 'v';
            break;
        case '\f':
            s[si++] = '\\';
            s[si++] = 'f';
            break;
        case '\r':
            s[si++] = '\\';
            s[si++] = 'r';
            break;
        case '\\':
            s[si++] = '\\';
            s[si++] = '\\';
            break;
        case '"':
            s[si++] = '\\';
            s[si++] = '"';
            break;
        default:
            s[si++] = c;
            break;
        }

        i++;
    }

    s[si] = '\0';
    print("\"%s\"", s);
}

void emit_expr(Node *expr)
{
    if (expr->wrap)
        print("(");

    switch (expr->kind)
    {
    case ND_ADD:
        emit_expr(expr->lhs);
        print(" + ");
        emit_expr(expr->rhs);
        break;
    case ND_SUB:
        emit_expr(expr->lhs);
        print(" - ");
        emit_expr(expr->rhs);
        break;
    case ND_MUL:
        emit_expr(expr->lhs);
        print(" * ");
        emit_expr(expr->rhs);
        break;
    case ND_DIV:
        emit_expr(expr->lhs);
        print(" / ");
        emit_expr(expr->rhs);
        break;
    case ND_POS:
        print("+");
        emit_expr(expr->lhs);
        break;
    case ND_VAR:
        print("%s", expr->var->name);
        break;
    case ND_NUM:
        print("%d", (int)expr->val);
        break;
    default:
        break;
    }
    
    if (expr->wrap)
        print(")");
}

void emit_stmt(Node *stmt)
{
    for (Node *node = stmt; node; node = node->next)
    {
        switch (node->kind)
        {
        case ND_RETURN:
            print("return");
            if (node->lhs)
            {
                print(" ");
                emit_expr(node->lhs);
            }
            println(";");
            break;
        case ND_BLOCK:
            indent++;
            println("{");
            emit_stmt(node->body);
            println("}");
            indent--;
            break;
        default:
            break;
        }
    }
}

Obj *emit_definition(Obj *var)
{
    if (var->ty->kind == TY_FUNC)
    {
        emit_type(var->ty->return_ty);
        print("%s(", var->name);

        Obj *head_param = var->ty->param_list;
        for (Obj *param = head_param; param; param = param->param_next)
        {
            if (param != head_param)
                print(",");

            emit_type_with_id(param->ty, param->name);
        }

        print(")");

        if (!var->is_definition) {
            println(";");
            return var;
        }

        print(" ");
        emit_stmt(var->body);
    }
    else
    {
        Type *var_ty = var->ty;
        emit_type_with_id(var->ty, var->name);

        if (var->rel)
            /* The actual init_data is at the relocation */
            var = var->next;

        if (!var)
            error("Unexpected error while generating relocation init data");

        if (var->init_data)
        {
            print(" = ");

            if (is_string(var_ty))
            {
                /* Must be string, allocates at most twice large than original size,
                 * which consider the worst case.
                 */
                emit_string(var->init_data, var->ty->size);
            }
            else
            {
                int pos = 0, val = 0;
                while (pos < var->ty->size)
                {
                    val += (var->init_data[pos] & 0xFF) << (pos * 8);
                    pos++;
                }

                print("%d", val);
            }
        }

        println(";");
    }

    return var;
}

void emit_data_c(Obj *prog)
{
    for (Obj *var = prog; var; var = var->next)
    {
        if (var->is_static)
            printf("WARNING: Unsupported specifier \"static\", this may cause unexpected compilation issue\n");
        if (var->is_inline)
            printf("WARNING: Unsupported specifier \"inline\"\n");

        var = emit_definition(var);
            
    }
}

void codegen_c(Obj *prog, FILE *out)
{
    output_file = out;

    prog = reverse(prog);
    emit_data_c(prog);
}
