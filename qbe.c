#include "slimcc.h"

static FILE *output_file;
static bool beginning_of_line = true;
static int indent = 0;

static int tmp_var_idx = 0;

static bool is_last_insn_jmp = false;

char *tmp_var() {
  return format("%%t%d", tmp_var_idx++);
}

char *as_tmp_var(int idx) {
  return format("%%t%d", idx);
}

void write_indent();

Obj *reverse_objs(Obj *head) {
  Obj *curr = head, *prev = NULL, *next;

  while (curr) {
    next = curr->next;
    curr->next = prev;
    prev = curr;
    curr = next;
  }

  return prev;
}

FMTCHK(1, 2)
static void println(char *fmt, ...) {
  if (beginning_of_line)
    write_indent();

  va_list ap;
  va_start(ap, fmt);
  vfprintf(output_file, fmt, ap);
  va_end(ap);
  fprintf(output_file, "\n");
  beginning_of_line = true;
}

FMTCHK(1, 2)
static void print(char *fmt, ...) {
  if (beginning_of_line)
    write_indent();

  va_list ap;
  va_start(ap, fmt);
  vfprintf(output_file, fmt, ap);
  va_end(ap);
  beginning_of_line = false;
}

static void print_escaped_str(char *str) {
  print("\"");

  while (*str) {
    switch (*str) {
      case '\a': {
        fprintf(output_file, "\\a");
        break;
      }
      case '\b': {
        fprintf(output_file, "\\b");
        break;
      }
      case '\f': {
        fprintf(output_file, "\\f");
        break;
      }
      case '\n': {
        fprintf(output_file, "\\n");
        break;
      }
      case '\r': {
        fprintf(output_file, "\\r");
        break;
      }
      case '\t': {
        fprintf(output_file, "\\t");
        break;
      }
      case '\v': {
        fprintf(output_file, "\\v");
        break;
      }
      case '\\': {
        fprintf(output_file, "\\\\");
        break;
      }
      case '\'': {
        fprintf(output_file, "\\'");
        break;
      }
      case '\"': {
        fprintf(output_file, "\\\"");
        break;
      }
      case '\?': {
        fprintf(output_file, "\\?");
        break;
      }
      case '\0': {
        fprintf(output_file, "\\0");
        break;
      }
      default: {
        fputc(*str, output_file);
        break;
      }
    }

    str++;
  }

  print("\"");
}

int count() {
  static int i = 0;
  return i++;
}

void write_indent() {
  for (size_t i = 0; i < indent; i++)
    fprintf(output_file, "  ");
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

char ty_specifier(Type *ty) {
  switch (ty->kind) {
    case TY_VOID:
      return 'v';
    case TY_LONG:
    case TY_LONGLONG:
    case TY_SHORT:
    case TY_INT:
    case TY_PTR:
    case TY_ENUM:
      return 'w';
    case TY_STRUCT: {
      error("Struct parameter is not yet supported");
      break;
    }
    case TY_FLOAT:
    case TY_DOUBLE:
    case TY_LDOUBLE:
    case TY_VLA:
    case TY_UNION: {
      error("Type kind %d is not supported in QBE-SIL.", ty->kind);
      break;
    }
    default:
      return 'b';
  }
}

char *emit_addr(Node *node);
char *emit_expr(Node *expr);
void emit_stmt(Node *stmt);

char *emit_var_expr(Node *expr) {
  if (expr->var->ty->kind == TY_FUNC) {
    char *tmp = tmp_var();

    println("%s =w loadw $%s", tmp, expr->var->name);
    return tmp;
  }

  return format(expr->var->is_local ? "%%%s" : "$%s", expr->var->name);
}

char *emit_arith_assign(Node *expr) {
  char *lhs, *rhs, *var, *addr, *op, ty_spec = ty_specifier(expr->ty);

  if (expr->lhs->kind == ND_VAR) {
    lhs = emit_var_expr(expr->lhs);
    rhs = emit_expr(expr->rhs);
    var = lhs;
  } else {
    addr = emit_addr(expr->lhs);
    rhs = emit_expr(expr->rhs);
    lhs = tmp_var();
    var = tmp_var();

    println("%s =%c loadw %s", lhs, ty_spec, addr);
  }

  switch (expr->arith_kind) {
    case ND_ADD: {
      op = "add";
      break;
    }
    case ND_SUB: {
      op = "sub";
      break;
    }
    case ND_MUL: {
      op = "mul";
      break;
    }
    case ND_DIV: {
      op = "div";
      break;
    }
    case ND_MOD: {
      op = "rem";
      break;
    }
    case ND_BITAND: {
      op = "and";
      break;
    }
    case ND_BITOR: {
      op = "or";
      break;
    }
    case ND_BITXOR: {
      op = "xor";
      break;
    }
    case ND_SHL: {
      op = "shl";
      break;
    }
    case ND_SHR: {
      op = "shr";
      break;
    }
    case ND_SAR: {
      op = "sar";
      break;
    }
    default: {
      error("Unexpected arith kind %d in arith assisn", expr->arith_kind);
      break;
    }
  }

  if (expr->lhs->kind == ND_VAR) {
    println("%s =%c %s %s, %s", var, ty_spec, op, lhs, rhs);
  } else {
    println("%s =%c %s %s, %s", var, ty_spec, op, lhs, rhs);
    println("store%c %s, %s", ty_spec, addr, var);
  }

  return var;
}

char *emit_binary_expr(Node *expr, char *op) {
  char *lhs = emit_expr(expr->lhs), *rhs = emit_expr(expr->rhs), *var = tmp_var();
  println("%s =%c %s %s, %s", var, ty_specifier(expr->ty), op, lhs, rhs);
  return var;
}

char *emit_addr(Node *node) {
  char *var = tmp_var(), *addr;

  switch (node->kind) {
    case ND_VAR: {
      if (node->var->is_local) {
        // local variable
        println("%s =w addr %%%s", var, node->var->name);
        return var;
      }

      if (node->ty->kind == TY_FUNC) {
        // Function ptr
        println("%s =w loadw $%s", var, node->var->name);
        return var;
      }

      // global variable
      println("%s =w gaddr %%%s", var, node->var->name);
      return var;
    }
    case ND_DEREF:
      var = emit_expr(node->lhs);
      return var;
    case ND_CHAIN:
    case ND_COMMA:
      var = emit_expr(node->rhs);
      return var;
    case ND_MEMBER:
      switch (node->lhs->kind) {
        case ND_FUNCALL:
        case ND_ASSIGN:
        case ND_COND:
        case ND_STMT_EXPR:
        case ND_VA_ARG:
          addr = emit_expr(node->lhs);
          println("%s =w add %s, %d", var, addr, node->member->offset);
          return var;
        default:
          addr = emit_addr(node->lhs);
          println("%s =w add %s, %d", var, addr, node->member->offset);
          return var;
      }
  }

  error_tok(node->tok, "not an lvalue");
  return var;
}

char *emit_expr(Node *expr) {
  char *var = "";

  switch (expr->kind) {
    case ND_NULL_EXPR: {
      if (expr->next)
        error("Expect no remaining expressions.");
      break;
    }
    case ND_ADD: {
      var = emit_binary_expr(expr, "add");
      break;
    }
    case ND_SUB: {
      var = emit_binary_expr(expr, "sub");
      break;
    }
    case ND_MUL: {
      var = emit_binary_expr(expr, "mul");
      break;
    }
    case ND_DIV: {
      var = emit_binary_expr(expr, "div");
      break;
    }
    case ND_POS: {
      var = emit_expr(expr->lhs);
      break;
    }
    case ND_NEG: {
      var = emit_expr(expr->lhs);
      println("%s =%c neg %s", var, ty_specifier(expr->ty), var);
      break;
    }
    case ND_MOD: {
      var = emit_binary_expr(expr, "rem");
      break;
    }
    case ND_BITAND: {
      var = emit_binary_expr(expr, "and");
      break;
    }
    case ND_BITOR: {
      var = emit_binary_expr(expr, "or");
      break;
    }
    case ND_BITXOR: {
      var = emit_binary_expr(expr, "xor");
      break;
    }
    case ND_SHL: {
      var = emit_binary_expr(expr, "shl");
      break;
    }
    case ND_SHR: {
      var = emit_binary_expr(expr, "shr");
      break;
    }
    case ND_SAR: {
      var = emit_binary_expr(expr, "sar");
      break;
    }
    case ND_EQ: {
      var = emit_binary_expr(expr, "ceq");
      break;
    }
    case ND_NE: {
      var = emit_binary_expr(expr, "cne");
      break;
    }
    case ND_LT: {
      var = emit_binary_expr(expr, "clt");
      break;
    }
    case ND_LE: {
      var = emit_binary_expr(expr, "cle");
      break;
    }
    case ND_GT: {
      var = emit_binary_expr(expr, "cgt");
      break;
    }
    case ND_GE: {
      var = emit_binary_expr(expr, "cge");
      break;
    }
    case ND_ASSIGN: {
      if (expr->lhs->kind == ND_DEREF) {
        char *rhs = emit_expr(expr->rhs);
        char *lhs = emit_expr(expr->lhs->lhs);

        println("store%c %s, %s", ty_specifier(expr->rhs->ty), rhs, lhs);
        break;
      }

      var = emit_expr(expr->lhs);
      char *rhs = emit_expr(expr->rhs);
      print("%s =%c copy %s", var, ty_specifier(expr->lhs->ty), rhs);
      println("");
      break;
    }
    case ND_COND: {
      int c = count();
      var = emit_expr(expr->cond);
      println("jnz %s, @L_cond_true_%d, @L_cond_false%d", var, c, c);
      println("@L_cond_true_%d", c);
      emit_expr(expr->then);
      println("jmp @L_cond_end_%d", c);
      println("@L_cond_false_%d", c);
      emit_expr(expr->els);
      println("@L_cond_end_%d", c);
      break;
    }
    case ND_COMMA: {
      emit_expr(expr->rhs);
      break;
    }
    case ND_MEMBER: {
      char *addr = emit_addr(expr), ty_spec = ty_specifier(expr->member->ty);
      var = tmp_var();

      println("%s =%c load%c %s", var, ty_spec, ty_spec, addr);
      break;
    }
    case ND_ADDR: {
      var = emit_addr(expr->lhs);
      break;
    }
    case ND_DEREF: {
      char *offset_var = emit_expr(expr->lhs), ty_spec = ty_specifier(expr->lhs->ty->base);
      var = tmp_var();

      println("%s =%c load%c %s", var, ty_spec, ty_spec, offset_var);
      break;
    }
    case ND_NOT: {
      char *lhs = emit_expr(expr->lhs);
      var = tmp_var();

      println("%s =b ceq %s, 0", var, lhs);
      break;
    }
    case ND_BITNOT: {
      char *lhs = emit_expr(expr->lhs);
      var = tmp_var();

      println("%s =b xor %s, -1", var, lhs);
      break;
    }
    case ND_LOGAND: {
      var = emit_binary_expr(expr, "and");
      break;
    }
    case ND_LOGOR: {
      var = emit_binary_expr(expr, "or");
      break;
    }
    case ND_FUNCALL: {
      bool has_ret_val = expr->ty->kind != TY_VOID;
      int start_tmp_idx = tmp_var_idx;

      for (Obj *arg = expr->args; arg; arg = arg->next) {
        var = emit_expr(arg->arg_expr);
        print("%s =%c copy %s", tmp_var(), ty_specifier(arg->arg_expr->ty), var);
        println("");
      }

      if (has_ret_val) {
        var = tmp_var();
        print("%s =%c call $%s(", var, ty_specifier(expr->ty), expr->lhs->var->name);
      } else
        print("call $%s(", expr->lhs->var->name);

      for (Obj *arg = expr->args; arg; arg = arg->next) {
        print("%c %s", ty_specifier(arg->arg_expr->ty), as_tmp_var(start_tmp_idx++));

        if (arg->next)
          print(", ");
      }

      println(")");

      break;
    }
    case ND_EXPR_STMT: {
      for (Node *node = expr->body; node; node = node->next) {
        if (!node->next && node->kind == ND_EXPR_STMT)
          emit_expr(node->lhs);
        else
          emit_stmt(node);
      }

      // TODO: Defer
      break;
    }
    case ND_VAR: {
      var = emit_var_expr(expr);
      break;
    }
    case ND_NUM: {
      var = tmp_var();
      println("%s =%c copy %d", var, ty_specifier(expr->ty), (int)expr->val);
      break;
    }
    case ND_CAST: {
      return emit_expr(expr->lhs);
    }
    case ND_INIT_AGG: {
      break;
    }
    case ND_VA_START: {
      break;
    }
    case ND_VA_COPY: {
      break;
    }
    case ND_VA_ARG: {
      break;
    }
    case ND_CHAIN: {
      break;
    }
    case ND_ALLOCA: {
      break;
    }
    case ND_ARITH_ASSIGN:
    case ND_POST_INCDEC: {
      var = emit_arith_assign(expr);
      break;
    }
  }

  is_last_insn_jmp = false;
  return var;
}

char *emit_cond(Node *cond) {
  return emit_expr(cond);
}

void emit_stmt(Node *stmt) {
  bool cond;

  switch (stmt->kind) {
    case ND_NULL_STMT: {
      if (stmt->next)
        error("Expect no remaining statements.");
      break;
    }
    case ND_RETURN: {
      if (!stmt->lhs) {
        println("ret");
        is_last_insn_jmp = true;
        return;
      }

      char *result_var = emit_expr(stmt->lhs);
      println("ret %s", result_var);
      is_last_insn_jmp = true;
      break;
    }
    case ND_IF: {
      int c = count();
      char *result_var = emit_cond(stmt->cond);
      println("jnz %s, @L_then_%d, @L_else_%d", result_var, c, c);
      println("@L_then_%d", c);
      emit_stmt(stmt->then);
      if (!(cond = is_last_insn_jmp))
        println("jmp @L_end_%d", c);
      println("@L_else_%d", c);
      if (stmt->els)
        emit_stmt(stmt->els);
      if (!cond)
        println("@L_end_%d", c);
      break;
    }
    case ND_FOR: {
      int c = count();

      if (stmt->init)
        emit_stmt(stmt->init);

      println("@L_begin_%d", c);
      char *result_var = emit_cond(stmt->cond);
      println("jnz %s, @L_then_%d, @%s", result_var, c, stmt->brk_label);
      println("@L_then_%d", c);
      emit_stmt(stmt->then);
      println("@%s", stmt->cont_label);

      if (stmt->inc)
        emit_expr(stmt->inc);

      println("jmp @L_begin_%d", c);
      println("@%s", stmt->brk_label);
      break;
    }
    case ND_DO: {
      int c = count();

      println("@L_begin_%d", c);
      emit_stmt(stmt->then);
      println("@%s", stmt->cont_label);
      char *result_var = emit_expr(stmt->cond);
      println("jnz %s, @%s, %s", result_var, stmt->cont_label, stmt->brk_label);
      println("@%s", stmt->brk_label);
      break;
    }
    case ND_SWITCH: {
      char *cond = emit_expr(stmt->cond), *cond_var = tmp_var();
      int c = count();

      for (Node *case_nd = stmt->case_next; case_nd; case_nd = case_nd->case_next) {
        if (stmt->case_next != case_nd) {
          println("@L_case_%d", c);
          c = count();
        }

        if (case_nd->begin == case_nd->end) {
          println("%s =w ceq %s, %d", cond_var, cond, (int)case_nd->begin);
          println("jnz %s, @%s, @L_case_%d", cond_var, case_nd->label, c);
          continue;
        }

        if (case_nd->begin == 0) {
          println("%s =w cle %s, %d", cond_var, cond, (int)(case_nd->end - case_nd->begin));
          println("jnz %s, @%s, @L_case_%d", cond_var, case_nd->label, c);
          continue;
        }

        println("%s =w cle %s, %d", cond_var, cond, (int)case_nd->end);
        println("jnz %s, @%s, @L_case_%d", cond_var, case_nd->label, c);
      }

      if (stmt->default_case)
        println("jmp @%s", stmt->default_case->label);

      println("jmp @%s", stmt->brk_label);
      emit_stmt(stmt->then);
      println("@%s", stmt->brk_label);
      break;
    }
    case ND_CASE: {
      println("@%s", stmt->label);
      if (stmt->lhs)
        emit_stmt(stmt->lhs);
      break;
    }
    case ND_BLOCK: {
      for (Node *node = stmt->body; node; node = node->next)
        emit_stmt(node);
      break;
    }
    case ND_GOTO: {
      println("jmp @%s", stmt->unique_label);
      is_last_insn_jmp = true;
      break;
    }
    case ND_GOTO_EXPR: {
      error("goto label-as-value is permitted on QBE-SIL backend.");
      break;
    }
    case ND_LABEL: {
      println("@%s", stmt->unique_label);
      if (stmt->lhs)
        emit_stmt(stmt->lhs);
      break;
    }
    case ND_EXPR_STMT: {
      emit_expr(stmt->lhs);
      break;
    }
    case ND_ASM:
      error("ASM block is not supported on QBE-SIL backend.");
    default: {
      break;
    }
  }
}

void emit_function(Obj *prog) {
  for (Obj *var = prog; var; var = var->next) {
    if (var->ty->kind != TY_FUNC)
      continue;

    Node *body = var->body;
    Type *return_ty = var->ty->return_ty;

    print("function ");

    if (return_ty)
      print("%c ", ty_specifier(return_ty));

    print("$%s(", var->name);

    for (Obj *param = var->ty->param_list; param; param = param->param_next) {
      print("%c %%%s", ty_specifier(param->ty), param->name);

      if (param->param_next)
        print(", ");
    }

    println(") {");
    indent++;

    if (body)
      emit_stmt(body);
    else
      println("# DECLARATION ONLY");

    /* Generates implicit return */
    if (return_ty->kind == TY_VOID)
      println("ret");

    indent--;
    println("}");
  }
}

void emit_data(Obj *prog) {
  for (Obj *var = prog; var; var = var->next) {
    if (!var->is_definition)
      continue;

    if (var->ty->kind == TY_FUNC) {
      if (var->is_live && var->static_lvars)
        emit_data(var->static_lvars);
      continue;
    }

    print("data $%s = { ", var->name);

    if (var->ty->kind == TY_ARRAY && strncmp(var->name, "L_", 2) == 0 && var->init_data) {
      // String global variable
      print("b ");
      print_escaped_str(var->init_data);
      println(" }");
      continue;
    }

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
  }
}

void gen_qbe(Obj *prog, FILE *out) {
  output_file = out;
  prog = reverse_objs(prog);

  emit_data(prog);
  emit_function(prog);
}
