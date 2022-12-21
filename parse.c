#include "ppcc.h"

Obj *locals;

static Obj *find_var(Token *tk)
{
	for (Obj *var = locals; var; var = var->next) {
		if (tk->len == strlen(var->name) &&
		    !strncmp(tk->loc, var->name, tk->len)) {
			return var;
		}
	}
	return NULL;
}

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

static Node *new_var_node(Obj *var)
{
	Node *n = new_node(ND_VAR);
	n->var = var;
	return n;
}

static Obj *new_lval(char *name)
{
	Obj *var = calloc(1, sizeof(Obj));
	var->name = name;
	var->next = locals;
	locals = var;

	return var;
}

/**
 * stmt = "return" stmt ";"
 *        | "if" "(" expr ")" stmt ("else" stmt)?
 *        | "{" compound-stmt
 *        | expr-stmt
 * expr-stmt = expr? ";"
 * compound-stmt = stmt* "}"
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
static Node *compound_stmt(Token *tk, Token **rest);
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
	if (equal(tk, "return")) {
		Node *n = new_unary(ND_RETURN, expr(tk->next, &tk));
		*rest = skip(tk, ";");
		return n;
	}

	if (equal(tk, "if")) {
		Node *n = new_node(ND_IF);
		tk = skip(tk->next, "(");
		n->cond = expr(tk, &tk);
		tk = skip(tk, ")");
		n->then = stmt(tk, &tk);
		if (equal(tk, "else")) {
			n->els = stmt(tk->next, &tk);
		}
		*rest = tk;
		return n;
	}

	if (equal(tk, "{")) {
		return compound_stmt(tk->next, rest);
	}

	return expr_stmt(tk, rest);
}

static Node *expr_stmt(Token *tk, Token **rest)
{
	if (equal(tk, ";")) {
		*rest = tk->next;
		return new_node(ND_BLOCK);
	}

	Node *n = new_unary(ND_EXPR_STMT, expr(tk, &tk));
	*rest = skip(tk, ";");
	return n;
}

static Node *expr(Token *tk, Token **rest)
{
	return assign(tk, rest);
}

Node *compound_stmt(Token *tk, Token **rest)
{
	Node head = {};
	Node *cur = &head;

	while (!equal(tk, "}")) {
		cur = cur->next = stmt(tk, &tk);
	}

	Node *n = new_node(ND_BLOCK);
	n->body = head.next;
	*rest = tk->next;

	return n;
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
		Obj *var = find_var(tk);
		if (!var) {
			var = new_lval(strndup(tk->loc, tk->len));
		}
		*rest = tk->next;
		return new_var_node(var);
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

Function *parse(Token *tk)
{
	tk = skip(tk, "{");

	Function *prog = calloc(1, sizeof(Function));
	prog->body = compound_stmt(tk, &tk);
	prog->locals = locals;

	return prog;
}
