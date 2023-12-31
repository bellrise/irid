/* Leaf compiler
   Copyright (c) 2023 bellrise */

#ifndef LC_H
#define LC_H

#include <stdbool.h>
#include <stddef.h>

#define LC_VER_MAJ 0
#define LC_VER_MIN 1

struct allocator;
extern struct allocator *global_ac;

/* -- opt.c -- */

struct options
{
    char *output;
    char *input;
};

void opt_set_defaults(struct options *opts);
void opt_parse(struct options *opts, int argc, char **argv);
void opt_add_missing_opts(struct options *opts);

/* -- file.c -- */

char *file_read(const char *path);

/* -- preproc.c -- */

char *preproc_remove_comments(char *source);
char *preproc_replace_tabs(char *source);

/* -- tok.c -- */

enum tok_type
{
    TOK_NULL,
    TOK_NUM,       /* number */
    TOK_CHAR,      /* character */
    TOK_STR,       /* string */
    TOK_SYM,       /* symbol */
    TOK_KW_TYPE,   /* "type" */
    TOK_KW_DECL,   /* "decl" */
    TOK_KW_FUNC,   /* "func" */
    TOK_KW_LET,    /* "let" */
    TOK_KW_IF,     /* "if" */
    TOK_KW_ELSE,   /* "else" */
    TOK_SEMICOLON, /* ; */
    TOK_COLON,     /* : */
    TOK_LBRACE,    /* { */
    TOK_RBRACE,    /* } */
    TOK_LPAREN,    /* ( */
    TOK_RPAREN,    /* ) */
    TOK_LBRACKET,  /* [ */
    TOK_RBRACKET,  /* ] */
    TOK_ARROW,     /* -> */
    TOK_PLUS,      /* + */
    TOK_MINUS,     /* - */
    TOK_STAR,      /* * */
    TOK_SLASH,     /* / */
    TOK_EQ,        /* = */
    TOK_CMPEQ,     /* == */
    TOK_CMPNEQ,    /* != */
    TOK_NOT,       /* ! */
    TOK_DOT,       /* . */
    TOK_COMMA,     /* , */
    TOK_AMPERSAND, /* & */
    TOK_QUOTE,     /* ' */
};

struct tok
{
    const char *pos;
    int len;
    int type;
};

const char *tok_typename(int type);

struct tokens
{
    const char *source_name;
    const char *source;
    struct tok *tokens;
    int n_tokens;
};

struct token_walker
{
    struct tokens *tokens;
    int sel;
};

struct tokens *tokens_new();
void tokens_tokenize(struct tokens *);

/* -- type.c -- */

enum type_type
{
    TYPE_NULL,
    TYPE_INTEGER,
    TYPE_POINTER,
};

struct type
{
    int type;
    char *name;
};

struct type_integer
{
    struct type head;
    int bit_width;
};

struct type_pointer
{
    struct type head;
    struct type *base_type;
};

struct type_register
{
    struct type **types;
    int n_types;
};

void *type_alloc(struct type_register *, int type_kind);
void type_dump(struct type *);
void type_dump_inline(struct type *);
const char *type_name(int type_kind);

void type_register_add_builtin(struct type_register *);
struct type *type_register_resolve(struct type_register *, const char *name);
struct type_pointer *type_register_add_pointer(struct type_register *,
                                               struct type *base_type);

/* -- alloc.c -- */

struct ac_allocation
{
    size_t size;
    size_t index;
    char area[];
};

struct allocator
{
    struct ac_allocation **allocs;
    size_t size;
    size_t space;
};

void *ac_alloc(struct allocator *, size_t bytes);
void *ac_realloc(struct allocator *, void *addr, size_t bytes);

void allocator_init(struct allocator *);
void allocator_free_all(struct allocator *);

/* -- parser.c -- */

enum node_type
{
    NODE_FILE,
    NODE_FUNC_DECL,
    NODE_FUNC_DEF,
    NODE_TYPE_DECL,
    NODE_VAR_DECL,
    NODE_ASSIGN,
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV,
    NODE_MOD,
    NODE_CMPEQ,
    NODE_CMPNEQ,
    NODE_CALL,
    NODE_VALUE,
    NODE_LITERAL
};

struct node
{
    struct node *parent;
    struct node *child;
    struct node *next;
    int type;
};

struct node_func_decl
{
    struct node head;
    struct type **param_types;
    const char **param_names;
    struct type *return_type;
    char *name;
    int n_params;
};

struct node_func_def
{
    struct node head;
    struct node_func_decl *decl;
};

struct node_var_decl
{
    struct node head;
    struct type *var_type;
    char *name;
};

struct node_value
{
    struct node head;
    char *name;
};

enum literal_type
{
    LITERAL_INTEGER,
    LITERAL_STRING
};

struct node_literal
{
    struct node head;
    int type;
    union
    {
        long long integer_value;
        char *string_value;
    };
};

void *node_alloc(struct node *parent, int type);
void node_add_child(struct node *parent, struct node *child);
const char *node_name(struct node *);
void node_tree_dump(struct node *);

struct parser
{
    struct tokens *tokens;
    struct node *tree;
    int sel_tok;
    struct type_register types;
};

struct parser *parser_new();
void parser_parse(struct parser *);

/* utils */

char *string_copy(const char *src, int len);

/* debug/die */

#if defined DEBUG
# define debug(...) debug_impl(__VA_ARGS__)
#else
# define debug(...)
#endif

void debug_impl(const char *fmt, ...);
void die(const char *fmt, ...);

#endif /* LC_H */
