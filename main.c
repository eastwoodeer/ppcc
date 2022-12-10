#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
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

static char *current_line;

static void verror_at(char *loc, char *fmt, va_list ap)
{
	int pos = loc - current_line;
	fprintf(stderr, "%s\n", current_line);
	fprintf(stderr, "%*s%s", pos, "", "^ ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

static void panic_at(char *loc, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verror_at(loc, fmt, ap);
}

static void panic_tk(Token *tk, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verror_at(tk->loc, fmt, ap);
}

static void panic(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

static bool equal(Token *tk, char *s)
{
	return (memcmp(tk->loc, s, tk->len) == 0 && s[tk->len] == '\0');
}

static Token *skip(Token *tk, char *s)
{
	if (!equal(tk, s)) {
		panic_tk(tk, "skip: expected '%s'", s);
	}

	return tk->next;
}

static int get_num(Token *tk)
{
	if (tk->kind != TK_NUM) {
		panic_tk(tk, "get_num: expected a number");
	}

	return tk->val;
}

static Token *new_token(char *start, char *end, TokenKind kind)
{
	Token *tk = calloc(1, sizeof(Token));
	tk->kind = kind;
	tk->loc = start;
	tk->len = end - start;

	return tk;
}

static bool startswith(char *s, char *p)
{
	return strncmp(s, p, strlen(p)) == 0;
}

static int read_punct(char *s)
{
	if (startswith(s, "==") || startswith(s, "!=") || startswith(s, ">=") ||
	    startswith(s, "<=")) {
		return 2;
	}

	return ispunct(*s) ? 1 : 0;
}

static Token *tokenize()
{
	Token head = {};
	Token *cur = &head;
	char *p = current_line;

	while (*p) {
		if (isspace(*p)) {
			p++;
			continue;
		}

		if (isdigit(*p)) {
			cur = cur->next = new_token(p, p, TK_NUM);
			char *q = p;
			cur->val = strtoul(p, &p, 10);
			cur->len = p - q;
			continue;
		}

		/* Punctuators */
		int len = read_punct(p);
		if (len) {
			cur = cur->next = new_token(p, p + len, TK_PUNCT);
			p += cur->len;
			continue;
		}

		panic_at(p, "tokenize: invalid token");
	}

	cur = cur->next = new_token(p, p, TK_EOF);

	return head.next;
}

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
	ND_NUM, /* Integer */
} NodeKind;

typedef struct Node Node;
struct Node {
	NodeKind kind;
	Node *lhs;
	Node *rhs;
	int val;
};

static Node *new_node(NodeKind kind)
{
	Node *n = calloc(1, sizeof(Node));
	n->kind = kind;

	return n;
}

static Node *new_num(int val)
{
	Node *n = new_node(ND_NUM);
	n->val = val;
	return n;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs)
{
	Node *n = new_node(kind);
	n->lhs = lhs;
	n->rhs = rhs;

	return n;
}

static Node *new_unary(NodeKind kind, Node *lhs)
{
	Node *n = new_node(kind);
	n->lhs = lhs;

	return n;
}

/**
 * expr = equality
 * equality = relational ( "==" relational | "!=" relational )*
 * relational = add ( "<" add | "<=" add | ">" add | ">=" add )*
 * add = mul ( "+" mul | "-" mul )* 
 * unary = ( "+" | "-" ) unary | primary 
 * mul = unary ( "*" unary | "/" unary )*  
 * primary = "(" expr ")" | num
 **/
static Node *expr(Token *tk, Token **rest);
static Node *equality(Token *tk, Token **rest);
static Node *relational(Token *tk, Token **rest);
static Node *add(Token *tk, Token **rest);
static Node *mul(Token *tk, Token **rest);
static Node *unary(Token *tk, Token **rest);
static Node *primary(Token *tk, Token **rest);

static Node *expr(Token *tk, Token **rest)
{
	return equality(tk, rest);
}

static Node *equality(Token *tk, Token **rest)
{
	Node *n = relational(tk, &tk);
	while (1) {
		if (equal(tk, "==")) {
			n = new_binary(ND_EQ, n, relational(tk->next, &tk));
			continue;
		}

		if (equal(tk, "!=")) {
			n = new_binary(ND_NE, n, relational(tk->next, &tk));
			continue;
		}

		*rest = tk;
		return n;
	}
}

static Node *relational(Token *tk, Token **rest)
{
	Node *n = add(tk, &tk);
	while (1) {
		if (equal(tk, "<")) {
			n = new_binary(ND_LT, n, add(tk->next, &tk));
			continue;
		}

		if (equal(tk, "<=")) {
			n = new_binary(ND_LE, n, add(tk->next, &tk));
			continue;
		}

		if (equal(tk, ">")) {
			n = new_binary(ND_GT, n, add(tk->next, &tk));
			continue;
		}

		if (equal(tk, ">=")) {
			n = new_binary(ND_GE, n, add(tk->next, &tk));
			continue;
		}

		*rest = tk;
		return n;
	}
}

static Node *add(Token *tk, Token **rest)
{
	Node *n = mul(tk, &tk);
	while (1) {
		if (equal(tk, "+")) {
			n = new_binary(ND_ADD, n, mul(tk->next, &tk));
			continue;
		}

		if (equal(tk, "-")) {
			n = new_binary(ND_SUB, n, mul(tk->next, &tk));
			continue;
		}

		*rest = tk;
		return n;
	}
}

static Node *mul(Token *tk, Token **rest)
{
	Node *n = unary(tk, &tk);
	while (1) {
		if (equal(tk, "*")) {
			n = new_binary(ND_MUL, n, unary(tk->next, &tk));
			continue;
		}

		if (equal(tk, "/")) {
			n = new_binary(ND_DIV, n, unary(tk->next, &tk));
			continue;
		}

		*rest = tk;
		return n;
	}
}

static Node *unary(Token *tk, Token **rest)
{
	if (equal(tk, "+")) {
		return unary(tk->next, rest);
	}

	if (equal(tk, "-")) {
		return new_unary(ND_NEG, unary(tk->next, rest));
	}

	return primary(tk, rest);
}

static Node *primary(Token *tk, Token **rest)
{
	if (equal(tk, "(")) {
		Node *n = expr(tk->next, &tk);
		*rest = skip(tk, ")");
		return n;
	}

	if (tk->kind == TK_NUM) {
		Node *n = new_num(tk->val);
		*rest = tk->next;
		return n;
	}

	panic_tk(tk, "expecetd a expression");

	/* never reaches here. */
	return NULL;
}

/* Code generator */
static int stack_depth = 0;

static void push()
{
	printf("    addi 1, 1, -4\n");
	printf("    stw  3, 0(1)\n");
	stack_depth++;
}

static void pop(char *arg)
{
	printf("    lwz %s, 0(1)\n", arg);
	printf("    addi 1, 1, 4\n");
	stack_depth--;
}

static void gen_expr(Node *n)
{
	switch (n->kind) {
	case ND_NUM:
		printf("    li 3, %d\n", n->val);
		return;
	case ND_NEG:
		gen_expr(n->lhs);
		printf("    neg 3, 3\n");
		return;
	}

	gen_expr(n->rhs);
	push();
	gen_expr(n->lhs);
	pop("4");

	switch (n->kind) {
	case ND_ADD:
		printf("    add 3, 3, 4\n");
		return;
	case ND_SUB:
		printf("    sub 3, 3, 4\n");
		return;
	case ND_MUL:
		printf("    mullw 3, 3, 4\n");
		return;
	case ND_DIV:
		printf("    divw 3, 3, 4\n");
		return;
	case ND_NE:
		printf("    xor 4, 3, 4\n");
		printf("    addic 3, 4, -1\n");
		printf("    subfe 3, 3, 4\n");
		return;
	case ND_EQ:
		printf("    xor 3, 3, 4\n");
		printf("    cntlzw 3, 3\n");
		printf("    srwi 3, 3, 5\n");
		return;
	case ND_GT:
		printf("    srwi 9, 3, 31\n");
		printf("    srawi 10, 4, 31\n");
		printf("    subfc 3, 3, 4\n");
		printf("    adde 3, 9, 10\n");
		printf("    xori 3, 3, 0x1\n");
		return;
	case ND_GE:
		printf("    srwi 10, 4, 31\n");
		printf("    srawi 9, 3, 31\n");
		printf("    subfc 4, 4, 3\n");
		printf("    adde 3, 10, 9\n");
		return;
	case ND_LT:
		printf("    srwi 9, 4, 31\n");
		printf("    srawi 10, 3, 31\n");
		printf("    subfc 4, 4, 3\n");
		printf("    adde 3, 9, 10\n");
		printf("    xori 3, 3, 0x1\n");
		return;
	case ND_LE:
		printf("    srwi 10, 3, 31\n");
		printf("    srawi 9, 4, 31\n");
		printf("    subfc 4, 3, 4\n");
		printf("    adde 3, 10, 9\n");
		return;
	}

	panic("invalid expression.");
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "%s: invalid number of arguments.\n", argv[0]);
		return 1;
	}

	current_line = argv[1];
	Token *tk = tokenize();
	Node *n = expr(tk, &tk);

	if (tk->kind != TK_EOF) {
		panic_tk(tk, "extra token.");
	}

	printf(".global main\n");
	printf("main:\n");

	gen_expr(n);

	assert(stack_depth == 0);

	printf("    blr\n");

	return 0;
}
