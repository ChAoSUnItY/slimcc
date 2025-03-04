#include "slimcc.h"

static FILE *output_file;
static int indent = 0;

FMTCHK(1, 2)
static void println(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(output_file, fmt, ap);
  va_end(ap);
  fprintf(output_file, "\n");
}

FMTCHK(1, 2)
static void print(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(output_file, fmt, ap);
  va_end(ap);
}

void write_indent() {
  for (size_t i = 0; i < indent; i++)
    print("  ");
}

/**
 * @param init_data, initialization data retrieved from Obj, can be null
 * @param size, size of init_data
 * @param ty, type of emitted data
 */
void emit_init_data(char *init_data, int offset, Type *ty) {
  if (!init_data) {
    int size = ty->kind == TY_PCHAR || ty->kind == TY_PTR ? 4 : ty->size;

    print("z %d", size);
    return;
  }

  switch (ty->kind) {
    case TY_ARRAY: {
      for (int i = 0; i < ty->array_len; i++) {
        emit_init_data(init_data, offset + i * ty->base->size, ty->base);

        if (i != ty->array_len - 1)
          print(", ");
      }
      break;
    }
    case TY_PCHAR:
    case TY_SHORT:
    case TY_INT:
    case TY_PTR:
    case TY_ENUM: {
      int val = 0;

      for (int i = offset; i < offset + 4; i++)
        val |= init_data[i] << (i * 8);

      print("w %d", val);
      break;
    }
    case TY_STRUCT: {
      error("Struct type initialization as gvar is not yet supported");
      break;
    }
    case TY_LONG:
    case TY_LONGLONG:
    case TY_FLOAT:
    case TY_DOUBLE:
    case TY_LDOUBLE:
    case TY_VLA:
    case TY_UNION: {
      error("Type kind %d is not supported in QBE-SIL.", ty->kind);
      break;
    }
    default: {
      print("b %d", init_data[offset]);
      break;
    }
  }
}

void emit_data(Obj *prog) {
  for (Obj *var = prog; var; var = var->next) {
    if (var->ty->kind != TY_FUNC) {
      print("data $%s = { ", var->name);

      switch (var->ty->kind) {
        case TY_ARRAY: {
          emit_init_data(var->init_data, 0, var->ty);
          break;
        }
        case TY_LONG:
        case TY_LONGLONG:
        case TY_FLOAT:
        case TY_DOUBLE:
        case TY_LDOUBLE:
        case TY_VLA:
        case TY_UNION: {
          error("Type kind %d is not supported in QBE-SIL.", var->ty->kind);
          break;
        }
        default: {
          emit_init_data(var->init_data, 0, var->ty);
          break;
        }
      }

      println(" }");
    } else {
      println("%s", var->name);
    }
  }
}

void gen_qbe(Obj *prog, FILE *out) {
  output_file = out;

  emit_data(prog);
}
