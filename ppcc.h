#ifndef __PPCC_H__
#define __PPCC_H__

#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	TK_IDENT,
	TK_PUNCT, /* punctuations */
	TK_KEYWORD, /* keywords */
	TK_NUM, /* numbers */
	TK_EOF,
} TokenKind;

typedef struct Token Token;
typedef struct Type Type;

struct Token {
	TokenKind kind;
	Token *next;
	int val;
	char *loc;
	int len;
};

Token *tokenize(char *s);
bool equal(Token *tk, char *s);
Token *skip(Token *tk, char *s);

void verror_at(char *loc, char *fmt, va_list ap);
void panic_at(char *loc, char *fmt, ...);
void panic_tk(Token *tk, char *fmt, ...);
void panic(char *fmt, ...);

typedef struct Node Node;

/* Local variable */
typedef struct Obj Obj;
struct Obj {
	Obj *next;
	char *name;
	int offset;
};

/* Function */
typedef struct Function Function;
struct Function {
	Node *body;
	Obj *locals;
	int stack_size;
};

/* Parser */
typedef enum {
	ND_ADD, /* + */
	ND_SUB, /* - */
	ND_MUL, /* * */
	ND_DIV, /* / */
	ND_NEG, /* unary */
	ND_EQ, /* == */
	ND_NE, /* != */
	ND_LT, /* < */
	ND_LE, /* <= */
	ND_GT, /* > */
	ND_GE, /* >= */
	ND_ASSIGN, /* = */
	ND_ADDR, /* & */
	ND_DEREF, /* * */
	ND_RETURN, /* "return" */
	ND_IF, /* "if" */
	ND_FOR, /* "for"/"while" */
	ND_BLOCK, /* {...} */
	ND_EXPR_STMT,
	ND_VAR, /* variable */
	ND_NUM, /* Integer */
} NodeKind;

struct Node {
	NodeKind kind;
	Node *next;
	Type *type;
	Token *tk;

	Node *lhs;
	Node *rhs;

	/* if/for statement */
	Node *cond;
	Node *then;
	Node *els;
	Node *init;
	Node *inc;

	Node *body; /* Block */
	Obj *var; /* if kind == ND_VAR */
	int val; /* if kind == ND_NUM */
};

typedef enum {
	TP_INT,
	TP_PTR,
} TypeKind;

struct Type {
	TypeKind kind;
	Type *base;
};

extern Type *type_int;

bool is_integer(Type *t);
void add_type(Node *n);

Function *parse(Token *tk);

void codegen(Function *prog);

#endif
