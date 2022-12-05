#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	TK_PUNCT,					/* punctuations */
	TK_NUM,						/* numbers */
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

static Token* skip(Token *tk, char *s)
{
	if (!equal(tk, s)) {
		panic("skip: expected '%s'", s);
	}

	return tk->next;
}

static int get_num(Token *tk)
{
	if (tk->kind != TK_NUM) {
		panic("get_num: expected a number got %d", tk->kind);
	}

	return tk->val;
}

static Token* new_token(char *start, char *end, TokenKind kind)
{
	Token *tk = calloc(1, sizeof(Token));
	tk->kind = kind;
	tk->loc = start;
	tk->len = end - start;

	return tk;
}

static Token *tokenize(char *s)
{
	Token head = {};
	Token *cur = &head;
	char *p = s;

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

		if (*p == '+' || *p == '-') {
			cur = cur->next = new_token(p, p+1, TK_PUNCT);
			p++;
			continue;
		}

		panic("tokenize: invalid token");
	}

	cur = cur->next = new_token(p, p, TK_EOF);

	return head.next;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
		fprintf(stderr, "%s: invalid number of arguments.\n", argv[0]);
		return 1;
	}

	Token *tk = tokenize(argv[1]);

	printf(".global main\n");
	printf("main:\n");
	printf("    li 3, %d\n", get_num(tk));
	tk = tk->next;

	while (tk->kind != TK_EOF) {
		if (equal(tk, "+")) {
			tk = skip(tk, "+");
			printf("    addi 3, 3, %d\n", get_num(tk));
			tk = tk->next;
			continue;
		}

		if (equal(tk, "-")) {
			tk = skip(tk, "-");
			printf("    subi 3, 3, %d\n", get_num(tk));
			tk = tk->next;
			continue;
		}

		panic("main: unexpected end");
		return 1;
	}

	printf("    blr\n");

    return 0;
}
