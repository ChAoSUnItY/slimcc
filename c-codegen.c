#include "slimcc.h"

static FILE *output_file;

FMTCHK(1, 2)
static void print(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(output_file, fmt, ap);
    va_end(ap);
}

FMTCHK(1, 2)
static void println(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(output_file, fmt, ap);
    va_end(ap);
    fprintf(output_file, "\n");
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
        print("void");
        break;
    }
    case TY_BOOL:
    {
        print("bool");
        break;
    }
    case TY_PCHAR:
    {
        print("char *");
        break;
    }
    case TY_CHAR:
    {
        print("char");
        break;
    }
    case TY_SHORT:
    {
        print("short");
        break;
    }
    case TY_INT:
    case TY_ENUM:
    {
        print("int");
        break;
    }
    case TY_LONG:
    {
        print("long");
        break;
    }
    case TY_LONGLONG:
    {
        print("long long");
        break;
    }
    case TY_FLOAT:
    {
        print("float");
        break;
    }
    case TY_DOUBLE:
    {
        print("double");
        break;
    }
    case TY_LDOUBLE:
    {
        print("long double");
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
        error("Unsupported VLA type");
        break;
    }
    case TY_FUNC:
    {
        emit_type(typ->return_ty);
        break;
    }
    default:
        break;
    }
}

void emit_definition(Obj *var)
{
    if (var->ty->kind == TY_FUNC)
    {
        emit_type(var->ty->return_ty);
        print(" %s(", var->name);

        Obj *head_params = var->ty->param_list;
        for (Obj *cur_param = head_params; cur_param; cur_param = cur_param->next)
        {
            emit_type(cur_param->ty);
            print(" %s", cur_param->name);

            if (cur_param != head_params)
                print(",");
        }

        println(");");
    }
    else
    {
        emit_type(var->ty);
        print(" %s", var->name);

        if (var->init_data)
        {
            print(" = ");

            if (var->rel && false) {
                error("Currently static initailized string is not supported");
            }

            int pos = 0, val = 0;
            while (pos < var->ty->size)
            {
                val += (var->init_data[pos] & 0xFF) << (pos * 8);
                pos++;
            }

            print("%d", val);
        }

        println(";");
    }
}

void emit_data_c(Obj *prog)
{
    for (Obj *var = prog; var; var = var->next)
    {
        if (var->is_static)
            print("static ");
        if (var->is_definition)
            emit_definition(var);
    }
}

void codegen_c(Obj *prog, FILE *out)
{
    output_file = out;

    prog = reverse(prog);
    emit_data_c(prog);
}
