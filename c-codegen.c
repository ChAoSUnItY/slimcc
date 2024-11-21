#include "slimcc.h"

FILE *output_file;

int indent = 0;
bool newline = true;

int tmp_var_cnt = 0;

FMTCHK(1, 2)
void print(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (indent > 0 && newline)
    for (int i = 0; i < indent; i++)
      fprintf(output_file, "  ");
  vfprintf(output_file, fmt, ap);
  va_end(ap);
  newline = false;
}

FMTCHK(1, 2)
void println(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (indent > 0 && newline)
    for (int i = 0; i < indent; i++)
      fprintf(output_file, "  ");
  vfprintf(output_file, fmt, ap);
  va_end(ap);
  fprintf(output_file, "\n");
  newline = true;
}

bool is_string(Type *ty) {
  return (ty->kind == TY_ARRAY || ty->kind == TY_PTR) && ty->base && ty->base->kind == TY_PCHAR;
}

/* Synthesizers */

char *tmp_var() {
  char *var_name = calloc(16, sizeof(char));
  snprintf(var_name, 15, "_tmp_%d", tmp_var_cnt++);
  return var_name;
}

/* To match original declaration sequence */
Obj *reverse(Obj *prog) {
  Obj *cur = prog, *prev = NULL, *next;

  while (cur != NULL) {
    next = cur->next;
    cur->next = prev;

    prev = cur;
    cur = next;
  }

  return prev;
}

void emit_type(Type *ty) {
  switch (ty->kind) {
    case TY_VOID: {
      print("void ");
      break;
    }
    case TY_BOOL: {
      print("bool ");
      break;
    }
    case TY_PCHAR:
    case TY_CHAR: {
      print("char ");
      break;
    }
    case TY_SHORT: {
      print("short ");
      break;
    }
    case TY_INT:
    case TY_ENUM: {
      print("int ");
      break;
    }
    case TY_LONG: {
      error("Unsupported data type \"long\"");
      print("long ");
      break;
    }
    case TY_LONGLONG: {
      error("Unsupported data type \"long long\"");
      print("long long ");
      break;
    }
    case TY_FLOAT: {
      error("Unsupported data type \"long float\"");
      print("float ");
      break;
    }
    case TY_DOUBLE: {
      error("Unsupported data type \"long double\"");
      print("double ");
      break;
    }
    case TY_LDOUBLE: {
      error("Unsupported data type \"long doublie\"");
      print("long double ");
      break;
    }
    case TY_PTR: {
      int level = 0;

      while (ty->base) {
        level++;
        ty = ty->base;
      }

      emit_type(ty);

      for (int i = 0; i < level; i++)
        print("*");
      break;
    }
    case TY_VLA: {
      error("Unsupported data type \"variable length array\"");
      break;
    }
    case TY_FUNC: {
      emit_type(ty->return_ty);
      break;
    }
    case TY_ARRAY: {
      emit_type(ty->base);
      break;
    }
    default:
      break;
  }
}

void emit_type_suffix(Type *ty) {
  switch (ty->kind) {
    case TY_ARRAY: {
      print("[%d]", (int)ty->array_len);
      emit_type_suffix(ty->base);
      break;
    }
    default:
      break;
  }
}

void emit_type_with_id(Type *typ, char *id, bool ignore_type) {
  if (!ignore_type)
    emit_type(typ);
  print("%s", id);
  emit_type_suffix(typ);
}

void emit_string(char *data, int size) {
  char *s = malloc(size * 2 + 1), c;
  int i = 0, si = 0;

  while (data[i]) {
    c = data[i];

    switch (c) {
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

void emit_lvalue(Node *expr) {
}

void emit_expr(Node *expr) {
  if (expr->wrap)
    print("(");

  switch (expr->kind) {
    case ND_ADD: {
      /* Check if it's a inc or dec */
      if (expr->lhs->kind == ND_ARITH_ASSIGN) {
        emit_expr(expr->lhs->lhs);

        /* Indicate if it's inc or dec by checking val */
        if (expr->rhs->val == -1)
          print("++");
        else if (expr->rhs->val == 1)
          print("--");
        break;
      }

      emit_expr(expr->lhs);
      print(" + ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_SUB: {
      emit_expr(expr->lhs);
      print(" - ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_MUL: {
      emit_expr(expr->lhs);
      print(" * ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_DIV: {
      emit_expr(expr->lhs);
      print(" / ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_POS: {
      print("+");
      emit_expr(expr->lhs);
      break;
    }
    case ND_NEG: {
      print("-");
      emit_expr(expr->lhs);
      break;
    }
    case ND_MOD: {
      emit_expr(expr->lhs);
      print(" %% ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_BITAND: {
      emit_expr(expr->lhs);
      print(" & ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_BITOR: {
      emit_expr(expr->lhs);
      print(" | ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_BITXOR: {
      emit_expr(expr->lhs);
      print(" ^ ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_SHL: {
      emit_expr(expr->lhs);
      print(" << ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_SHR:
    case ND_SAR: {
      /* TODO: Investigate if shecc support SAR. */
      emit_expr(expr->lhs);
      print(" >> ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_EQ: {
      emit_expr(expr->lhs);
      print(" == ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_NE: {
      emit_expr(expr->lhs);
      print(" != ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_LT: {
      emit_expr(expr->lhs);
      print(" < ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_LE: {
      emit_expr(expr->lhs);
      print(" <= ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_GT: {
      emit_expr(expr->lhs);
      print(" > ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_GE: {
      emit_expr(expr->lhs);
      print(" >= ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_ASSIGN: {
      emit_expr(expr->lhs);

      if (expr->rhs) {
        print(" = ");

        switch (expr->rhs->arith_kind) {
          case ND_ADD: {
            emit_expr(expr->rhs->lhs);
            print(" + ");
            emit_expr(expr->rhs->rhs);
            break;
          }
          default: {
            emit_expr(expr->rhs);
            break;
          }
        }
      }
      break;
    }
    case ND_COND: {
      emit_expr(expr->cond);
      print(" ? ");
      emit_expr(expr->then);
      print(" : ");
      emit_expr(expr->els);
      break;
    }
    case ND_COMMA: {
      emit_expr(expr->lhs);
      print(", ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_MEMBER: {
      printf("TODO ND_MEMBER\n");
      break;
    }
    case ND_NOT: {
      print("!");
      emit_expr(expr->lhs);
      break;
    }
    case ND_BITNOT: {
      print("~");
      emit_expr(expr->lhs);
      break;
    }
    case ND_LOGAND: {
      emit_expr(expr->lhs);
      print(" && ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_LOGOR: {
      emit_expr(expr->lhs);
      print(" || ");
      emit_expr(expr->rhs);
      break;
    }
    case ND_ADDR: {
      print("&");
      emit_expr(expr->lhs);
      break;
    }
    case ND_DEREF: {
      Node *index_expr = expr->lhs->rhs;

      emit_expr(expr->lhs->lhs);
      print("[");

      if (index_expr->kind == ND_CAST && index_expr->lhs->kind == ND_MUL) {
        /* Ignore scaling */
        emit_expr(index_expr->lhs->lhs);
      } else {
        printf("WARNING: Unhandled deref rhs");
        emit_expr(index_expr);
      }

      print("]");
      break;
    }
    case ND_VAR: {
      if (expr->var->is_static) {
        emit_string(expr->var->init_data, expr->var->ty->size);
      } else {
        print("%s", expr->var->name);
      }
      break;
    }
    case ND_NUM: {
      print("%d", (int)expr->val);
      break;
    }
    case ND_CAST: {
      /* TODO: Handle more cases as much as possible */
      emit_expr(expr->lhs);
      break;
    }
    case ND_CHAIN: {
      if (expr->lhs->kind == ND_DEFINE && expr->rhs->kind == ND_ASSIGN) {
        emit_expr(expr->lhs);
        print(" = ");
        emit_expr(expr->rhs->rhs);
      } else {
        emit_expr(expr->lhs);
        print(", ");
        emit_expr(expr->rhs);
      }
      break;
    }
    case ND_ARITH_ASSIGN: {
      if (expr->rhs->kind == ND_NUM && expr->rhs->val == 1) {
        if (expr->arith_kind == ND_ADD) {
          print("++");
        } else if (expr->arith_kind == ND_SUB) {
          print("--");
        }

        emit_expr(expr->lhs);
        break;
      }

      emit_expr(expr->lhs);

      if (expr->arith_kind == ND_ADD) {
        print(" += ");
      } else if (expr->arith_kind == ND_SUB) {
        print(" -= ");
      }

      emit_expr(expr->rhs);
      break;
    }
    case ND_DEFINE: {
      Obj *var = expr->lhs->var;

      switch (expr->var_def_kind) {
        case VDK_HEAD: {
          emit_type_with_id(expr->ty, var->name, false);
          break;
        }
        case VDK_TAIL_LEFT: {
          emit_type_with_id(expr->ty, var->name, true);
          break;
        }
        case VDK_NONE: {
          emit_expr(expr->lhs);
          break;
        }
      }
      break;
    }
    default:
      break;
  }

  if (expr->wrap)
    print(")");
}

void emit_stmt(Node *stmt) {
  for (Node *node = stmt; node; node = node->next) {
    switch (node->kind) {
      case ND_RETURN: {
        print("return");
        if (node->lhs) {
          print(" ");
          emit_expr(node->lhs);
        }
        println(";");
        break;
      }
      case ND_IF: {
        print("if (");
        emit_expr(node->cond);
        println(") ");
        if (node->then->body->kind != ND_BLOCK)
          indent++;
        emit_stmt(node->then->body); /* Omit outer synthetic block */
        if (node->then->body->kind != ND_BLOCK)
          indent--;
        if (node->els) {
          print("else ");
          emit_stmt(node->els->body); /* Omit outer synthetic block */
        }
        break;
      }
      case ND_FOR: {
        Node *init = node->init, *cond = node->cond, *inc = node->inc, *body = node->then->body;

        if (!init && cond && !inc) {
          print("while (");
          emit_expr(cond);
        } else {
          print("for (");
          if (node->init)
            emit_expr(node->init->lhs); /* Omit outer synthetic expr_stmt */

          if (node->cond) {
            print("; ");
            emit_expr(node->cond);
          } else
            print(";");
          if (node->inc) {
            print("; ");
            emit_expr(node->inc);
          } else
            print(";");
        }

        if (body->kind == ND_NULL_STMT) {
          println(");");
          continue;
        } else
          println(") ");

        if (body->kind != ND_BLOCK)
          indent++;

        emit_stmt(body); /* Omit outer synthetic block */

        if (body->kind != ND_BLOCK)
          indent--;
        break;
      }
      case ND_BLOCK: {
        println("{");
        indent++;
        emit_stmt(node->body);
        indent--;
        println("}");
        break;
      }
      case ND_EXPR_STMT: {
        emit_expr(node->lhs);
        println(";");
        break;
      }
      case ND_CHAIN: {
        emit_expr(node->lhs);
        emit_expr(node->rhs);
        break;
      }
      default:
        break;
    }
  }
}

Obj *emit_definition(Obj *var) {
  if (var->ty->kind == TY_FUNC) {
    emit_type(var->ty->return_ty);
    print("%s(", var->name);

    Obj *head_param = var->ty->param_list;
    for (Obj *param = head_param; param; param = param->param_next) {
      if (param != head_param)
        print(", ");

      emit_type_with_id(param->ty, param->name, false);
    }

    print(")");

    if (!var->is_definition) {
      println(";");
      return var;
    }

    print(" ");
    emit_stmt(var->body);
  } else {
    Type *var_ty = var->ty;
    emit_type_with_id(var->ty, var->name, false);

    if (var->rel)
      /* The actual init_data is at the relocation */
      var = var->next;

    if (!var)
      error("Unexpected error while generating relocation init data");

    if (var->init_data) {
      print(" = ");

      if (is_string(var_ty)) {
        /* Must be string, allocates at most twice large than original size,
         * which consider the worst case.
         */
        emit_string(var->init_data, var->ty->size);
      } else {
        int pos = 0, val = 0;
        while (pos < var->ty->size) {
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

void emit_data_c(Obj *prog) {
  for (Obj *var = prog; var; var = var->next) {
    if (var->is_static)
      printf("WARNING: Unsupported specifier \"static\", this may cause unexpected compilation issue\n");
    if (var->is_inline)
      printf("WARNING: Unsupported specifier \"inline\"\n");

    var = emit_definition(var);
  }
}

void codegen_c(Obj *prog, FILE *out) {
  output_file = out;

  prog = reverse(prog);
  emit_data_c(prog);
}
