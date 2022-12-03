#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
		fprintf(stderr, "%s: invalid number of arguments.\n", argv[0]);
		return 1;
	}

	char *p = argv[1];

	printf(".global main\n");
	printf("main:\n");
	printf("    li 3, %ld\n", strtol(p, &p, 10));

	while (*p) {
		if (*p == '+') {
			p++;
			printf("    addi 3, 3, %ld\n", strtol(p, &p, 10));
			continue;
		}

		if (*p == '-') {
			p++;
			printf("    subi 3, 3, %ld\n", strtol(p, &p, 10));
			continue;
		}

		fprintf(stderr, "unexpected character: '%c'\n", *p);
		return 1;
	}

	printf("    blr\n");

    return 0;
}
