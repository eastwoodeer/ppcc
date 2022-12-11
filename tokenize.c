#include "ppcc.h"

static char *current_line;

void verror_at(char *loc, char *fmt, va_list ap)
{
	int pos = loc - current_line;
	fprintf(stderr, "%s\n", current_line);
	fprintf(stderr, "%*s%s", pos, "", "^ ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

void panic_at(char *loc, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verror_at(loc, fmt, ap);
}

void panic_tk(Token *tk, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verror_at(tk->loc, fmt, ap);
}

void panic(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

bool equal(Token *tk, char *s)
{
	return (memcmp(tk->loc, s, tk->len) == 0 && s[tk->len] == '\0');
}

Token *skip(Token *tk, char *s)
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

Token *tokenize(char *s)
{
	current_line = s;
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
