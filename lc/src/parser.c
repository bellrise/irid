/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

#include "lc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void pe_base(struct parser *self, struct tok *here, int offset,
                    const char *fix_str, const char *help_str, const char *fmt,
                    va_list args)
{
    source_error(self->tokens, here->pos, here->len, offset, fix_str, help_str,
                 "error", "\033[31m", fmt, args);
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

static struct parsed_type *parse_type(struct parser *self)
{
    struct parsed_type *type;
    struct tok *place;
    struct tok *tok;
    bool is_pointer;
    char *type_name;

    tok = tokcur(self);
    if (tok->type != TOK_SYM)
        pef(self, tok, "int", "expected type name");

    type_name = string_copy(tok->pos, tok->len);
    is_pointer = false;
    place = tok;

    tok = tokpeek(self);
    if (tok->type == TOK_AMPERSAND) {
        is_pointer = true;
        toknext(self);
    }

    type = parsed_type_register_new(self->types, type_name, is_pointer);
    type->place = place;

    return type;
}

static void func_decl_add_param(struct node_func_decl *decl,
                                struct parsed_type *type, const char *name)
{
    decl->param_types =
        ac_realloc(global_ac, decl->param_types,
                   sizeof(struct parsed_type *) * (decl->n_params + 1));
    decl->param_names = ac_realloc(global_ac, decl->param_names,
                                   sizeof(const char *) * (decl->n_params + 1));

    decl->param_types[decl->n_params] = type;
    decl->param_names[decl->n_params++] = name;
}

static void func_decl_collect_params(struct parser *self,
                                     struct node_func_decl *decl)
{
    struct parsed_type *param_type;
    struct tok *tok;
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
    decl->head.place = funcname;
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
    decl->head.place = tokpeek(self);
    decl->var_type = parse_type(self);

    tok = toknext(self);
    if (tok->type != TOK_SYM)
        pef(self, tok, "var", "expected variable name");

    decl->name = string_copy(tok->pos, tok->len);
}

static struct node *parse_expr_inside(struct parser *self, struct node *parent);

static void parse_call_args(struct parser *self, struct node *call)
{
    struct node *arg;
    struct tok *tok;
    struct tok *start_tok;

    tok = tokcur(self);

    while (1) {
        if (tok->type == TOK_RPAREN)
            break;

        start_tok = tok;
        node_add_child(call, parse_expr_inside(self, call));
        tok = tokcur(self);

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

    struct node_label *label_node;
    struct node_literal *literal_node;

    struct node *expr;
    struct node *left_expr;
    struct node *right_expr;
    struct tok *tok;
    struct tok *tmptok;
    int node_type;
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
        label_node = node_alloc(NULL, NODE_LABEL);
        label_node->head.place = tok;
        label_node->name = string_copy(tok->pos, tok->len);
        left_expr = (struct node *) label_node;
    }

    /* literal */

    else if (tok->type == TOK_NUM) {
        literal_node = node_alloc(NULL, NODE_LITERAL);
        literal_node->head.place = tok;
        literal_node->type = LITERAL_INTEGER;

        tmpstr = string_copy(tok->pos, tok->len);
        literal_node->integer_value = strtol(tmpstr, NULL, 0);
        left_expr = (struct node *) literal_node;
    }

    else if (tok->type == TOK_STR) {
        literal_node = node_alloc(NULL, NODE_LITERAL);
        literal_node->head.place = tok;
        literal_node->type = LITERAL_STRING;

        literal_node->string_value = string_copy(tok->pos + 1, tok->len - 2);
        left_expr = (struct node *) literal_node;
    }

    /* Now, the complicated part starts. We have to check for any operators
       after the first value. This means that the expression at hand is a
       complex expression which is either a two-hand operation, or a call. */

    tok = tokpeek(self);
    tmptok = tok;

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
        expr->place = tmptok;
        node_add_child(expr, left_expr);
        parse_call_args(self, expr);

        ((struct node_call *) expr)->call_end_place = tokcur(self);

        left_expr = expr;
    }

    tok = tokpeek(self);
    tmptok = tok;

    /* expr '=' expr */

    if (tok->type == TOK_EQ) {
        tok = toknext(self);
        tok = toknext(self);

        /* node_assign:
            - left_expr (value to add to)
            - right_expr (expression to add)
         */

        expr = node_alloc(NULL, NODE_ASSIGN);
        expr->place = tmptok;

        right_expr = parse_expr_inside(self, expr);

        node_add_child(expr, left_expr);
        node_add_child(expr, right_expr);

        return expr;
    }

    /* expr ( '+' | '-' | '*' | '/' | '%' | '==' | '!=' ) expr */

    node_type = NODE_NULL;

    if (tok->type == TOK_PLUS)
        node_type = NODE_ADD;
    if (tok->type == TOK_MINUS)
        node_type = NODE_SUB;
    if (tok->type == TOK_STAR)
        node_type = NODE_MUL;
    if (tok->type == TOK_SLASH)
        node_type = NODE_DIV;
    if (tok->type == TOK_PERCENT)
        node_type = NODE_MOD;
    if (tok->type == TOK_CMPEQ)
        node_type = NODE_CMPEQ;
    if (tok->type == TOK_CMPNEQ)
        node_type = NODE_CMPNEQ;

    if (node_type != NODE_NULL) {
        tok = toknext(self);
        tok = toknext(self);

        /* node_add/sub/mul/div/mod:
            - left_expr
            - right_expr
         */

        expr = node_alloc(NULL, node_type);
        expr->place = tmptok;

        right_expr = parse_expr_inside(self, expr);

        node_add_child(expr, left_expr);
        node_add_child(expr, right_expr);

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
        if (tok->type == TOK_RBRACE)
            break;

        if (tok->type != TOK_SEMICOLON)
            pef(self, tok, ";", "expected a semicolon after the statement");
    }
}

static int attr_flag(const char *attr)
{
    struct
    {
        const char *name;
        int flag;
    } names[2] = {{"naked", ATTR_NAKED}, {"local", ATTR_LOCAL}};

    for (int i = 0; i < 2; i++) {
        if (!strcmp(names[i].name, attr))
            return names[i].flag;
    }

    return 0;
}

static void parse_attributes(struct parser *self, struct attributes *attrs)
{
    struct tok *tok;
    int flag;

    tok = tokcur(self);
    if (tok->type != TOK_LBRACKET)
        pef(self, tok, "[", "expected opening bracket for attribute list");

    while (1) {
        tok = toknext(self);
        if (tok->type == TOK_RBRACKET)
            break;

        if (tok->type != TOK_SYM)
            pe(self, tok, "expected attribute name");

        flag = attr_flag(string_copy(tok->pos, tok->len));
        if (!flag)
            pe(self, tok, "unknown attribute");

        attrs->flags |= flag;

        tok = toknext(self);
        if (tok->type == TOK_RBRACKET)
            break;
        if (tok->type != TOK_COMMA)
            pef(self, tok, ",", "expected comma seperating attributes");
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

    if (tok->type == TOK_LBRACKET) {
        parse_attributes(self, &decl->attrs);
        tok = toknext(self);
    }

    if (tok->type != TOK_LBRACE)
        pef(self, tok, "{", "expected brace for function body");

    def = node_alloc(parent, NODE_FUNC_DEF);
    def->head.place = tok;
    def->decl = decl;

    parse_func_body(self, def);
}

void parser_parse(struct parser *self)
{
    struct tok *tok;

    self->tree = node_alloc(NULL, NODE_FILE);
    self->sel_tok = 0;
    self->types = ac_alloc(global_ac, sizeof(struct type_register));

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

    node_tree_dump(self->tree);
}
