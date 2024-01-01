/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

#include "lc.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

struct tokens *tokens_new()
{
    return ac_alloc(global_ac, sizeof(struct tokens));
}

static void tnew(struct tokens *self, int type, const char *s, int len)
{
    struct tok *tok;

    self->tokens = ac_realloc(global_ac, self->tokens,
                              sizeof(struct tok) * (self->n_tokens + 1));
    tok = &self->tokens[self->n_tokens++];

    tok->type = type;
    tok->pos = s;
    tok->len = len;
}

const char *tok_typename(int type)
{
    const char *toknames[] = {
        "NULL",      "NUM",     "CHAR",   "STR",    "SYM",     "KW_TYPE",
        "KW_DECL",   "KW_FUNC", "KW_LET", "KW_IF",  "KW_ELSE", "SEMICOLON",
        "COLON",     "LBRACE",  "RBRACE", "LPAREN", "RPAREN",  "LBRACKET",
        "RBRACKET",  "ARROW",   "PLUS",   "MINUS",  "STAR",    "SLASH",
        "EQ",        "CMPEQ",   "CMPNEQ", "NOT",    "DOT",     "COMMA",
        "AMPERSAND", "QUOTE",   "PERCENT"};

    return toknames[type];
}

static bool isstartsymchar(char c)
{
    return isalpha(c) || c == '_';
}

static bool issymchar(char c)
{
    return isstartsymchar(c) || isdigit(c);
}

static int min(int a, int b)
{
    return a < b ? a : b;
}

static void replace_syms_with_kw(struct tokens *self)
{
    struct
    {
        int type;
        const char *name;
    } strings[] = {
        {TOK_KW_TYPE, "type"}, {TOK_KW_DECL, "decl"}, {TOK_KW_FUNC, "func"},
        {TOK_KW_LET, "let"},   {TOK_KW_IF, "if"},     {TOK_KW_ELSE, "else"},
    };

    const int n_strings = 6;
    int len;

    for (int i = 0; i < self->n_tokens; i++) {
        if (!self->tokens[i].len)
            continue;

        for (int j = 0; j < n_strings; j++) {
            len = min(self->tokens[i].len, strlen(strings[j].name));
            if (!strncmp(self->tokens[i].pos, strings[j].name, len)) {
                self->tokens[i].type = strings[j].type;
                break;
            }
        }
    }
}

void tokens_tokenize(struct tokens *self)
{
    const char *p;
    const char *q;

    p = self->source;
    q = self->source;

    while (*p) {
        /* string */
        if (*p == '"') {
            q = p + 1;
            while (*q != '"')
                q++;

            q++;
            tnew(self, TOK_STR, p, (size_t) q - (size_t) p);
            p = q;
            continue;
        }

        /* symbol */
        if (isstartsymchar(*p)) {
            q = p;
            while (issymchar(*q))
                q++;

            tnew(self, TOK_SYM, p, (size_t) q - (size_t) p);
            p = q;
            continue;
        }

        /* character */
        if (p[0] == '\'' && p[2] == '\'') {
            tnew(self, TOK_CHAR, p + 1, 1);
            p += 3;
            continue;
        }

        /* number */
        if (isdigit(*p)) {
            q = p;
            while (isdigit(*q) || *q == 'x')
                q++;

            tnew(self, TOK_NUM, p, (size_t) q - (size_t) p);
            p = q;
            continue;
        }

        switch (*p) {
        case ';':
            tnew(self, TOK_SEMICOLON, p, 1);
            break;
        case ':':
            tnew(self, TOK_COLON, p, 1);
            break;
        case '{':
            tnew(self, TOK_LBRACE, p, 1);
            break;
        case '}':
            tnew(self, TOK_RBRACE, p, 1);
            break;
        case '(':
            tnew(self, TOK_LPAREN, p, 1);
            break;
        case ')':
            tnew(self, TOK_RPAREN, p, 1);
            break;
        case '[':
            tnew(self, TOK_LBRACKET, p, 1);
            break;
        case ']':
            tnew(self, TOK_RBRACKET, p, 1);
            break;
        case '-':
            if (p[1] == '>') {
                tnew(self, TOK_ARROW, p, 2);
                p += 1;
            } else {
                tnew(self, TOK_MINUS, p, 1);
            }
            break;
        case '+':
            tnew(self, TOK_PLUS, p, 1);
            break;
        case '*':
            tnew(self, TOK_STAR, p, 1);
            break;
        case '/':
            tnew(self, TOK_SLASH, p, 1);
            break;
        case '=':
            if (p[1] == '=') {
                tnew(self, TOK_CMPEQ, p, 2);
                p += 1;
            } else {
                tnew(self, TOK_EQ, p, 1);
            }
            break;
        case '!':
            if (p[1] == '=') {
                tnew(self, TOK_CMPNEQ, p, 2);
                p += 1;
            } else {
                tnew(self, TOK_NOT, p, 1);
            }
            break;
        case '.':
            tnew(self, TOK_DOT, p, 1);
            break;
        case ',':
            tnew(self, TOK_COMMA, p, 1);
            break;
        case '&':
            tnew(self, TOK_AMPERSAND, p, 1);
            break;
        case '\'':
            tnew(self, TOK_QUOTE, p, 1);
            break;
        case '%':
            tnew(self, TOK_PERCENT, p, 1);
            break;
        }

        p++;
    }

    /* End the token array with a null token. */
    tnew(self, TOK_NULL, p, 0);

    replace_syms_with_kw(self);
}
