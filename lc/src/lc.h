/* Leaf compiler
   Copyright (c) 2023-2024 bellrise */

#ifndef LC_H
#define LC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define LC_VER_MAJ 0
#define LC_VER_MIN 6

struct allocator;
extern struct allocator *global_ac;

#define MAX_ARG 4

/* -- opt.c -- */

struct options
{
    char *output;
    char *input;
    bool set_origin;
    int origin;

    bool w_unused_var;

    bool f_comment_asm;
    bool f_cmp_literal;
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
    TOK_KW_RETURN, /* "return" */
    TOK_KW_FOR,    /* "for" */
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
    TOK_PERCENT,   /* % */
    TOK_THREEDOT,  /* ... */
};

struct tok
{
    const char *pos;
    int len;
    int type;
};

const char *tok_typename(int type);

/* Stores a token array. */

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
    TYPE_STRUCT,
};

/* Information about a type, like its bitwidth. */

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

struct type_struct
{
    struct type head;
    struct type **field_types;
    char **field_names;
    int n_fields;
};

struct type_register
{
    struct type **types;
    int n_types;
};

void *type_alloc(struct type_register *, int type_kind);
void type_dump(struct type *);
const char *type_kind_name(int type_kind);
const char *type_repr(struct type *);
bool type_is_null(struct type *);
bool type_cmp(struct type *, struct type *);
int type_size(struct type *);
void type_struct_add_field(struct type_struct *, struct type *type,
                           const char *name);
struct type *type_struct_find_field(struct type_struct *, const char *name);
int type_struct_field_offset(struct type_struct *, const char *name);

struct type *type_register_resolve(struct type_register *, const char *name);
struct type_pointer *type_register_add_pointer(struct type_register *,
                                               struct type *base_type);
struct type_struct *type_register_alloc_struct(struct type_register *,
                                               const char *name);

/* Parsed types are the result of the parser, and do not check for any validity,
   which is done in the compiler. */

struct parsed_type
{
    char *name;
    bool is_pointer;
    struct tok *place;
};

struct parsed_type_register
{
    struct parsed_type **types;
    int n_types;
};

struct parsed_type *parsed_type_register_new(struct parsed_type_register *,
                                             const char *name, bool is_pointer);
void parsed_type_dump_inline(struct parsed_type *);

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
void allocator_dump(struct allocator *);

/* -- parser.c -- */

enum node_type
{
    NODE_NULL,
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
    NODE_LABEL,
    NODE_LITERAL,
    NODE_RETURN,
    NODE_FIELD,
    NODE_IF,
    NODE_ADDR,
    NODE_LOOP,
    NODE_INDEX,
};

/* The result of parsing is a tree of nodes. */

struct node
{
    struct node *parent;
    struct node *child;
    struct node *next;
    struct tok *place;
    struct parsed_type *cast_into;
    int type;
};

#define ATTR_NAKED 1
#define ATTR_LOCAL 2

struct attributes
{
    int flags;
};

struct node_func_decl
{
    struct node head;
    struct parsed_type **param_types;
    const char **param_names;
    struct tok **param_places;
    struct parsed_type *return_type;
    struct attributes attrs;
    bool is_variadic;
    char *name;
    int n_params;
};

struct node_func_def
{
    struct node head;
    struct node_func_decl *decl;
};

struct type_field
{
    struct parsed_type *parsed_type;
    char *name;
};

struct node_type_decl
{
    struct node head;
    char *typename;
    struct type_field **fields;
    int n_fields;
};

struct node_var_decl
{
    struct node head;
    struct parsed_type *var_type;
    char *name;
};

struct node_label
{
    struct node head;
    char *name;
};

struct node_call
{
    struct node head;
    struct tok *call_end_place;
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
        int integer_value;
        char *string_value;
    };
};

struct node_field
{
    struct node head;
    struct tok *field_tok;
};

struct node_addr
{
    struct node head;
    char *name;
};

struct node_loop
{
    struct node head;
    char *index_name;
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
    struct parsed_type_register *types;
};

struct parser *parser_new();
void parser_parse(struct parser *);

/* -- block.c -- */

enum block_type
{
    BLOCK_NULL,
    BLOCK_FILE,
    BLOCK_FUNC,
    BLOCK_PREAMBLE,
    BLOCK_EPILOGUE,
    BLOCK_LOCAL,
    BLOCK_GLOBAL,
    BLOCK_STORE,
    BLOCK_STORE_RETURN,
    BLOCK_STORE_RESULT,
    BLOCK_STORE_ARG,
    BLOCK_LOAD,
    BLOCK_STRING,
    BLOCK_CALL,
    BLOCK_ASM,
    BLOCK_JMP,
    BLOCK_LABEL,
    BLOCK_CMP,
};

/* The result of compilation is a (almost) flat tree of blocks. */

struct block
{
    struct block *next;
    struct block *child;
    struct block *parent;
    int type;
};

struct local
{
    struct type *type;
    struct node_var_decl *decl;
    char *name;
    bool is_global;
    bool used;
    union
    {
        struct block_local *local_block;
        struct block_global *global_block;
    };
};

struct block_func
{
    struct block head;
    bool exported;
    char *label;
    struct local **locals;
    int n_locals;
    int emit_locals_size;
    struct type *return_type;
    int var_index;
    int label_index;
};

struct block_local
{
    struct block head;
    struct local *local;
    int emit_offset;
};

struct block_global
{
    struct block head;
    struct local *local;
};

enum imm_width_type
{
    IMM_8,
    IMM_16
};

/* An immediate is a literal integer value which can be either 8 or 16 bits
   wide. */

struct imm
{
    int width;
    int value;
};

enum op_type
{
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_CMPEQ,
    OP_CMPNEQ
};

struct op_value
{
    int type;
    struct value *left;
    struct value *right;
};

enum value_type
{
    VALUE_NULL,
    VALUE_IMMEDIATE,
    VALUE_LOCAL,
    VALUE_GLOBAL,
    VALUE_STRING,
    VALUE_OP,
    VALUE_ADDR,
    VALUE_INDEX,
};

/* A value can either be an immediate integer value, reference to another
   variable or an address. */

struct value
{
    int value_type;
    struct tok *place;
    int local_offset;
    struct type *field_type;
    bool deref;
    struct value *index_offset;
    int index_elem_size;
    struct type *cast_type;
    union
    {
        struct imm imm_value;
        struct local *local_value;
        struct value *index_var;
        char *label_value;
        int string_id_value;
        struct op_value op_value;
    };
};

struct block_store
{
    struct block head;
    struct local *var;
    struct value value;
    int store_offset;
};

struct block_string
{
    struct block head;
    int string_id;
    char *value;
};

struct block_call
{
    struct block head;
    char *call_name;
    struct value *args;
    int n_args;
};

struct block_asm
{
    struct block head;
    char *source;
};

enum jmp_type
{
    JMP_ALWAYS,
    JMP_EQ,
};

struct block_jmp
{
    struct block head;
    char *dest;
    int type;
};

struct block_label
{
    struct block head;
    char *label;
};

struct block_store_arg
{
    struct block head;
    struct local *local;
    int arg;
};

enum cmp_type
{
    CMP_EQ,
    CMP_NEQ,
};

struct block_cmp
{
    struct block head;
    struct value left;
    struct value right;
    int type;
};

void *block_alloc(struct block *parent, int type);
void block_add_child(struct block *parent, struct block *child);
void block_insert_first_child(struct block *parent, struct block *child);
void block_add_next(struct block *any_block, struct block *next);
void block_insert_next(struct block *any_block, struct block *next);
const char *block_name(struct block *);
void block_tree_dump(struct block *);

/* -- compiler.c -- */

struct func_sig
{
    char *name;
    struct type **param_types;
    int n_params;
    struct type *return_type;
    struct node_func_decl *decl;
};

struct compiler
{
    struct node *tree;
    struct block *file_block;
    struct type_register *types;
    struct tokens *tokens;
    struct block_string **strings;
    struct options *opts;
    struct func_sig **funcs;
    struct local **globals;
    int n_globals;
    int n_funcs;
    int n_strings;
};

void compiler_compile(struct compiler *);

/* -- emit.c -- */

struct emitter
{
    FILE *out;
    struct block *file_block;
    struct options *opts;
};

void emitter_emit(struct emitter *, struct options *opts);

/* -- utils.c -- */

char *string_copy(const char *src, int len);
char *string_copy_z(const char *src);

/* -- error.c -- */

void source_error(struct tokens *tokens, const char *here, int here_len,
                  int offset, const char *fix_str, const char *help_str,
                  const char *title, const char *color, const char *fmt,
                  va_list args);

void source_error_nv(struct tokens *tokens, const char *here, int here_len,
                     int offset, const char *fix_str, const char *help_str,
                     const char *title, const char *color, const char *fmt,
                     ...);

/* debug/die */

#if defined DEBUG
# define debug(...) debug_impl(__VA_ARGS__)
#else
# define debug(...)
#endif

void debug_impl(const char *fmt, ...);
void die(const char *fmt, ...);

#endif /* LC_H */
