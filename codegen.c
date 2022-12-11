#include "ppcc.h"

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

void codegen(Node *n)
{
	printf(".global main\n");
	printf("main:\n");

	gen_expr(n);

	printf("    blr\n");

	assert(stack_depth == 0);
}
