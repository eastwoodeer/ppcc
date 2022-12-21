#include "ppcc.h"

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

static Node *new_var_node(char name)
{
	Node *n = new_node(ND_VAR);
	n->name = name;
	return n;
}

/**
 * stmt = expr-stmt
 * expr-stmt = expr ";"
 * expr = assign
 * assign = equality ( "=" assign )?
 * equality = relational ( "==" relational | "!=" relational )*
 * relational = add ( "<" add | "<=" add | ">" add | ">=" add )*
 * add = mul ( "+" mul | "-" mul )* 
 * unary = ( "+" | "-" ) unary | primary 
 * mul = unary ( "*" unary | "/" unary )*  
 * primary = "(" expr ")" | ident | num
 **/
static Node *expr(Token *tk, Token **rest);
static Node *expr_stmt(Token *tk, Token **rest);
static Node *assign(Token *tk, Token **rest);
static Node *equality(Token *tk, Token **rest);
static Node *relational(Token *tk, Token **rest);
static Node *add(Token *tk, Token **rest);
static Node *mul(Token *tk, Token **rest);
static Node *unary(Token *tk, Token **rest);
static Node *primary(Token *tk, Token **rest);

static Node *stmt(Token *tk, Token **rest)
{
	return expr_stmt(tk, rest);
}

static Node *expr_stmt(Token *tk, Token **rest)
{
	Node *n = new_unary(ND_EXPR_STMT, expr(tk, &tk));
	*rest = skip(tk, ";");
	return n;
}

static Node *expr(Token *tk, Token **rest)
{
	return assign(tk, rest);
}

static Node *assign(Token *tk, Token **rest)
{
	Node *n = equality(tk, &tk);
	if (equal(tk, "=")) {
		n = new_binary(ND_ASSIGN, n, assign(tk->next, &tk));
	}
	*rest = tk;
	return n;
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

	if (tk->kind == TK_IDENT) {
		Node *n = new_var_node(*tk->loc);
		*rest = tk->next;
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

Node *parse(Token *tk)
{
	Node head = {};
	Node *current = &head;
	while (tk->kind != TK_EOF) {
		current = current->next = stmt(tk, &tk);
	}
	return head.next;
}
