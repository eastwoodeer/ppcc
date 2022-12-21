#ifndef __PPCC_H__
#define __PPCC_H__

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
	TK_NUM, /* numbers */
	TK_EOF,
} TokenKind;

typedef struct Token Token;
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
	ND_ASSIGN,
	ND_EXPR_STMT,
	ND_VAR, /* variable */
	ND_NUM, /* Integer */
} NodeKind;

typedef struct Node Node;
struct Node {
	NodeKind kind;
	Node *next;
	Node *lhs;
	Node *rhs;
	char name;
	int val;
};

Node *parse(Token *tk);

void codegen(Node *n);

#endif
