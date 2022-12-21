#include "ppcc.h"

int main(int argc, char *argv[])
{
	if (argc != 2) {
		panic("%s: invalid number of arguments.\n", argv[0]);
		return 1;
	}

	Token *tk = tokenize(argv[1]);
	Function *prog = parse(tk);
	codegen(prog);

	return 0;
}
