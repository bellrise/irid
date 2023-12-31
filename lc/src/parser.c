/* Leaf compiler
   Copyright (c) 2023 bellrise */

#include "lc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

struct parser *parser_new()
{
    return ac_alloc(global_ac, sizeof(struct parser));
}

static struct tok *tokcur(struct parser *self)
{
    return &self->tokens->tokens[self->sel_tok];
}

static struct tok *toknext(struct parser *self)
{
    if (self->sel_tok == self->tokens->n_tokens - 1)
        self->sel_tok -= 1;
    return &self->tokens->tokens[++self->sel_tok];
}

static struct tok *tokpeek(struct parser *self)
{
    if (self->sel_tok == self->tokens->n_tokens - 1)
        self->sel_tok -= 1;
    return &self->tokens->tokens[self->sel_tok + 1];
}

static int count_line_of_token(struct parser *self, struct tok *tok)
{
    const char *p;
    int line;

    line = 1;
    p = self->tokens->source;

    while (p != tok->pos) {
        if (*p == '\n')
            line++;
        p++;
    }

    return line;
}

static const char *find_start_of_line(struct parser *self,
                                      const char *from_here)
{
    const char *p;

    p = from_here;
    while (p > self->tokens->source) {
        if (p[-1] == '\n')
            return p;
        p--;
    }

    return p;
}

static int line_len(const char *from_here)
{
    const char *p;

    p = from_here;
    while (*p != '\n' && *p != 0)
        p++;

    return (int) ((size_t) p - (size_t) from_here);
}

static void pe_base(struct parser *self, struct tok *here, int offset,
                    const char *fix_str, const char *help_str, const char *fmt,
                    va_list args)
{
    const char *line_start;
    int line_off;
    int line_num;
    int len;

    line_start = find_start_of_line(self, here->pos);
    line_num = count_line_of_token(self, here);

    printf("irid-lc: \033[1;98merror at %s:%d\033[0m\n\n",
           self->tokens->source_name, line_num);

    if (fix_str) {
        printf("    | ");
        line_off = offset + (int) ((size_t) here->pos - (size_t) line_start);
        for (int i = 0; i < line_off; i++)
            fputc(' ', stdout);
        for (int i = 0; i < offset; i++)
            fputc(' ', stdout);
        printf("\033[3;32m%s\033[0m\n    | \033[3;32m", fix_str);

        for (int i = 0; i < line_off; i++)
            fputc(' ', stdout);
        len = here->len - offset;
        for (int i = 0; i < len; i++)
            fputc('v', stdout);

        printf("\033[0m\n");
    }

    else {
        printf("    |\n");
    }

    len = line_len(line_start);

    printf("% 3d | ", line_num);

    len = here->pos - line_start;
    printf("%.*s", len, line_start);
    len = here->len - offset;
    printf("\033[31m%.*s\033[0m", len, here->pos + offset);
    len = line_len(here->pos + here->len);
    printf("%.*s", len, here->pos + here->len);

    printf("\n    | \033[31m");

    line_off = offset + (int) ((size_t) here->pos - (size_t) line_start);
    for (int i = 0; i < line_off; i++)
        fputc(' ', stdout);
    fputc('^', stdout);

    len = here->len - offset - 1;
    for (int i = 0; i < len; i++)
        fputc('~', stdout);

    fputc(' ', stdout);
    vprintf(fmt, args);
    printf("\033[0m\n");

    if (help_str)
        printf("    |\n    | \033[36m%s\033[0m\n", help_str);

    fputc('\n', stdout);
    va_end(args);
    exit(1);
}

static void pe(struct parser *self, struct tok *here, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    pe_base(self, here, 0, NULL, NULL, fmt, args);
}

static void pef(struct parser *self, struct tok *here, const char *fix_str,
                const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    pe_base(self, here, 0, fix_str, NULL, fmt, args);
}

static void peh(struct parser *self, struct tok *here, const char *help_str,
                const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    pe_base(self, here, 0, NULL, help_str, fmt, args);
}

static void pefh(struct parser *self, struct tok *here, const char *fix_str,
                 const char *help_str, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    pe_base(self, here, 0, fix_str, help_str, fmt, args);
}

static struct type *parse_type(struct parser *self)
{
    struct type_pointer *pointer_type;
    struct type *type;
    struct tok *tok;
    char *type_name;

    tok = tokcur(self);
    if (tok->type != TOK_SYM)
        pef(self, tok, "int", "expected type name");

    type_name = string_copy(tok->pos, tok->len);
    type = type_register_resolve(&self->types, type_name);

    if (!type)
        pe(self, tok, "unknown type `%s`", type_name);

    tok = tokpeek(self);
    if (tok->type == TOK_AMPERSAND) {
        /* Wrap the type in a pointer type. */
        pointer_type = type_register_add_pointer(&self->types, type);
        type = (struct type *) pointer_type;
        toknext(self);
    }

    return type;
}

static void func_decl_add_param(struct node_func_decl *decl, struct type *type,
                                const char *name)
{
    decl->param_types =
        ac_realloc(global_ac, decl->param_types,
                   sizeof(struct type *) * (decl->n_params + 1));
    decl->param_names = ac_realloc(global_ac, decl->param_names,
                                   sizeof(const char *) * (decl->n_params + 1));

    decl->param_types[decl->n_params] = type;
    decl->param_names[decl->n_params++] = name;
}

static void func_decl_collect_params(struct parser *self,
                                     struct node_func_decl *decl)
{
    struct tok *tok;
    struct type *param_type;
    const char *param_name;

    tok = toknext(self);

    while (tok->type != TOK_RPAREN) {
        tok = tokcur(self);
        if (tok->type != TOK_SYM)
            pef(self, tok, "int", "expected type name");

        param_type = parse_type(self);

        tok = toknext(self);
        if (tok->type != TOK_SYM)
            pef(self, tok, "param", "missing parameter name");

        param_name = string_copy(tok->pos, tok->len);

        func_decl_add_param(decl, param_type, param_name);

        tok = toknext(self);
        if (tok->type == TOK_RPAREN)
            break;
        if (tok->type == TOK_COMMA) {
            self->sel_tok++;
            continue;
        }

        pef(self, tok, ",", "expected comma before next parameter");
    }
}

static struct node_func_decl *parse_func_head(struct parser *self,
                                              struct node *parent)
{
    struct node_func_decl *decl;
    struct tok *funcname;
    struct tok *tok;

    tok = toknext(self);
    if (tok->type != TOK_SYM)
        pe(self, tok, "expected function name");
    funcname = tok;

    tok = toknext(self);
    if (tok->type != TOK_LPAREN)
        pef(self, tok, "(", "expected opening parenthesis");

    decl = node_alloc(parent, NODE_FUNC_DECL);
    decl->name = string_copy(funcname->pos, funcname->len);
    func_decl_collect_params(self, decl);

    tok = tokcur(self);
    if (tok->type != TOK_RPAREN)
        pef(self, tok, ")", "expected closing parenthesis");

    tok = tokpeek(self);

    /* Optional return type. */

    if (tok->type != TOK_ARROW)
        return decl;

    tok = toknext(self);
    tok = toknext(self);
    if (tok->type != TOK_SYM)
        pe(self, tok, "expected return type");

    decl->return_type = parse_type(self);
    return decl;
}

static void parse_var_decl(struct parser *self, struct node *parent)
{
    struct node_var_decl *decl;
    struct tok *tok;

    tok = tokcur(self);
    if (tok->type != TOK_KW_LET)
        pef(self, tok, "let", "expected let keyword");

    tok = toknext(self);
    if (tok->type != TOK_SYM)
        pef(self, tok, "int", "expected variable type");

    decl = node_alloc(parent, NODE_VAR_DECL);
    decl->var_type = parse_type(self);

    tok = toknext(self);
    if (tok->type != TOK_SYM)
        pef(self, tok, "var", "expected variable name");

    decl->name = string_copy(tok->pos, tok->len);
}

static struct node *parse_expr_inside(struct parser *self, struct node *parent);

static void parse_call_args(struct parser *self, struct node *call)
{
    struct tok *tok;

    tok = tokcur(self);

    while (1) {
        if (tok->type == TOK_RPAREN)
            break;

        node_add_child(call, parse_expr_inside(self, call));

        tok = toknext(self);
        if (tok->type == TOK_RPAREN)
            break;

        if (tok->type == TOK_COMMA) {
            tok = toknext(self);
            continue;
        }

        pe(self, tok,
           "missing comma or parenthesis ending the call expression");
    }
}

static struct node *parse_expr_inside(struct parser *self, struct node *parent)
{
    /* Parsing expressions is the most complex thing in this parser, so here is
       a snippet from the grammar file:

    expr:
        | '(' expr ')'
        | expr ( '+' | '-' | '*' | '/' | '%' ) expr
        | expr ( '==' | '!=' ) expr
        | expr '(' [ expr [ ',' expr ]* ] ')'
        | expr '=' expr
        | symbol
        | literal
    */

    struct node_value *value_node;
    struct node_literal *literal_node;

    struct node *expr;
    struct node *left_expr;
    struct node *right_expr;
    struct tok *tok;
    char *tmpstr;

    tok = tokcur(self);

    /* '(' expr ')' */

    if (tok->type == TOK_LPAREN) {
        toknext(self);
        left_expr = parse_expr_inside(self, parent);
        toknext(self);
    }

    /* symbol */

    else if (tok->type == TOK_SYM) {
        value_node = node_alloc(NULL, NODE_VALUE);
        value_node->name = string_copy(tok->pos, tok->len);
        left_expr = (struct node *) value_node;
    }

    /* literal */

    else if (tok->type == TOK_NUM) {
        literal_node = node_alloc(NULL, NODE_LITERAL);
        literal_node->type = LITERAL_INTEGER;

        tmpstr = string_copy(tok->pos, tok->len);
        literal_node->integer_value = strtoll(tmpstr, NULL, 0);
        left_expr = (struct node *) literal_node;
    }

    else if (tok->type == TOK_STR) {
        literal_node = node_alloc(NULL, NODE_LITERAL);
        literal_node->type = LITERAL_STRING;

        literal_node->string_value = string_copy(tok->pos, tok->len);
        left_expr = (struct node *) literal_node;
    }

    /* Now, the complicated part starts. We have to check for any operators
       after the first value. This means that the expression at hand is a
       complex expression which is either a two-hand operation, or a call. */

    tok = tokpeek(self);

    /* expr '=' expr */

    if (tok->type == TOK_EQ) {
        tok = toknext(self);
        tok = toknext(self);

        /* node_assign:
            - left_expr (value to add to)
            - right_expr (expression to add)
         */

        expr = node_alloc(NULL, NODE_ASSIGN);
        node_add_child(expr, left_expr);
        right_expr = parse_expr_inside(self, expr);
        node_add_child(expr, right_expr);

        return expr;
    }

    /* expr '(' ... ')' */

    if (tok->type == TOK_LPAREN) {
        tok = toknext(self);
        tok = toknext(self);

        /* node_call:
            - some name
            - arg1
            - arg2
            - ...
         */

        expr = node_alloc(NULL, NODE_CALL);
        node_add_child(expr, left_expr);
        parse_call_args(self, expr);

        return expr;
    }

    return left_expr;
}

static void parse_expr(struct parser *self, struct node *parent)
{
    node_add_child(parent, parse_expr_inside(self, parent));
}

static void parse_func_body(struct parser *self, struct node_func_def *func_def)
{
    struct tok *tok;

    while (1) {
        tok = toknext(self);

        switch (tok->type) {
        case TOK_RBRACE:
            return;
        case TOK_KW_LET:
            parse_var_decl(self, (struct node *) func_def);
            break;
        default:
            parse_expr(self, (struct node *) func_def);
            break;
        }

        /* After each statement, the language requires a semicolon. */

        tok = toknext(self);
        debug("> %s", tok_typename(tok->type));
        if (tok->type == TOK_RBRACE)
            break;

        if (tok->type != TOK_SEMICOLON)
            pef(self, tok, ";", "expected a semicolon after the statement");
    }
}

static void parse_func(struct parser *self, struct node *parent)
{
    struct node_func_decl *decl;
    struct node_func_def *def;
    struct tok *tok;

    tok = tokcur(self);
    if (tok->type != TOK_KW_FUNC)
        pef(self, tok, "func", "expected func keyword");

    decl = parse_func_head(self, parent);
    tok = toknext(self);

    /* If we end the function here, it's a declaration. */
    if (tok->type == TOK_SEMICOLON)
        return;

    if (tok->type != TOK_LBRACE)
        pef(self, tok, "{", "expected brace for function body");

    def = node_alloc(parent, NODE_FUNC_DEF);
    def->decl = decl;

    parse_func_body(self, def);
}

void parser_parse(struct parser *self)
{
    struct tok *tok;

    self->tree = node_alloc(NULL, NODE_FILE);
    self->sel_tok = 0;

    type_register_add_builtin(&self->types);

    while (1) {
        tok = tokcur(self);
        if (tok->type == TOK_NULL)
            break;

        switch (tok->type) {
        case TOK_KW_FUNC:
            parse_func(self, self->tree);
            break;
        default:
            peh(self, tok, "expected a top-level declaration, like a function",
                "unexpected token");
        }

        self->sel_tok++;
    }

    for (int i = 0; i < self->types.n_types; i++)
        type_dump(self->types.types[i]);
    node_tree_dump(self->tree);
}
