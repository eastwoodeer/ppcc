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

static Node *new_node(NodeKind kind, Token *tk)
{
	Node *n = calloc(1, sizeof(Node));
	n->kind = kind;
	n->tk = tk;

	return n;
}

static Node *new_num(int val, Token *tk)
{
	Node *n = new_node(ND_NUM, tk);
	n->val = val;
	return n;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tk)
{
	Node *n = new_node(kind, tk);
	n->lhs = lhs;
	n->rhs = rhs;

	return n;
}

static Node *new_unary(NodeKind kind, Node *lhs, Token *tk)
{
	Node *n = new_node(kind, tk);
	n->lhs = lhs;

	return n;
}

static Node *new_var_node(Obj *var, Token *tk)
{
	Node *n = new_node(ND_VAR, tk);
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
 *        | "for" "(" expr-stmt ";" expr? ";" expr? ")" stmt
 *        | "while" "(" expr ")" stmt
 *        | "{" compound-stmt
 *        | expr-stmt
 * expr-stmt = expr? ";"
 * compound-stmt = stmt* "}"
 * expr = assign
 * assign = equality ( "=" assign )?
 * equality = relational ( "==" relational | "!=" relational )*
 * relational = add ( "<" add | "<=" add | ">" add | ">=" add )*
 * add = mul ( "+" mul | "-" mul )* 
 * unary = ( "+" | "-" | "&" | "*" ) unary | primary 
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
		Node *n = new_node(ND_RETURN, tk);
		n->lhs = expr(tk->next, &tk);
		*rest = skip(tk, ";");
		return n;
	}

	if (equal(tk, "if")) {
		Node *n = new_node(ND_IF, tk);
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

	if (equal(tk, "for")) {
		Node *n = new_node(ND_FOR, tk);
		tk = skip(tk->next, "(");
		n->init = expr_stmt(tk, &tk);

		if (!equal(tk, ";")) {
			n->cond = expr(tk, &tk);
		}
		tk = skip(tk, ";");

		if (!equal(tk, ")")) {
			n->inc = expr(tk, &tk);
		}
		tk = skip(tk, ")");
		n->then = stmt(tk, rest);
		return n;
	}

	if (equal(tk, "while")) {
		Node *n = new_node(ND_FOR, tk);
		tk = skip(tk->next, "(");
		n->cond = expr(tk, &tk);
		tk = skip(tk, ")");
		n->then = stmt(tk, rest);
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
		return new_node(ND_BLOCK, tk);
	}

	Node *n = new_node(ND_EXPR_STMT, tk);
	n->lhs = expr(tk, &tk);
	*rest = skip(tk, ";");
	return n;
}

static Node *expr(Token *tk, Token **rest)
{
	return assign(tk, rest);
}

Node *compound_stmt(Token *tk, Token **rest)
{
	Node *n = new_node(ND_BLOCK, tk);

	Node head = {};
	Node *cur = &head;

	while (!equal(tk, "}")) {
		cur = cur->next = stmt(tk, &tk);
	}

	n->body = head.next;
	*rest = tk->next;

	return n;
}

static Node *assign(Token *tk, Token **rest)
{
	Node *n = equality(tk, &tk);
	if (equal(tk, "=")) {
		n = new_binary(ND_ASSIGN, n, assign(tk->next, &tk), tk);
	}
	*rest = tk;
	return n;
}

static Node *equality(Token *tk, Token **rest)
{
	Node *n = relational(tk, &tk);
	while (1) {
		Token *t = tk;

		if (equal(tk, "==")) {
			n = new_binary(ND_EQ, n, relational(tk->next, &tk), t);
			continue;
		}

		if (equal(tk, "!=")) {
			n = new_binary(ND_NE, n, relational(tk->next, &tk), t);
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
		Token *t = tk;

		if (equal(tk, "<")) {
			n = new_binary(ND_LT, n, add(tk->next, &tk), t);
			continue;
		}

		if (equal(tk, "<=")) {
			n = new_binary(ND_LE, n, add(tk->next, &tk), t);
			continue;
		}

		if (equal(tk, ">")) {
			n = new_binary(ND_GT, n, add(tk->next, &tk), t);
			continue;
		}

		if (equal(tk, ">=")) {
			n = new_binary(ND_GE, n, add(tk->next, &tk), t);
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
		Token *t = tk;

		if (equal(tk, "+")) {
			n = new_binary(ND_ADD, n, mul(tk->next, &tk), t);
			continue;
		}

		if (equal(tk, "-")) {
			n = new_binary(ND_SUB, n, mul(tk->next, &tk), t);
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
		Token *t = tk;

		if (equal(tk, "*")) {
			n = new_binary(ND_MUL, n, unary(tk->next, &tk), t);
			continue;
		}

		if (equal(tk, "/")) {
			n = new_binary(ND_DIV, n, unary(tk->next, &tk), t);
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
		return new_unary(ND_NEG, unary(tk->next, rest), tk);
	}

	if (equal(tk, "&")) {
		return new_unary(ND_ADDR, unary(tk->next, rest), tk);
	}

	if (equal(tk, "*")) {
		return new_unary(ND_DEREF, unary(tk->next, rest), tk);
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
		return new_var_node(var, tk);
	}

	if (tk->kind == TK_NUM) {
		Node *n = new_num(tk->val, tk);
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
