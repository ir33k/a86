#include <stdio.h>
#define A86_IMPLEMENTATION
#include "a86.h"

int
main(int argc, char **argv)
{
	char *path = argc > 1 ? argv[1] : 0; /* Path to input file */
	FILE *in = stdin;		     /* Input file */
	enum a86_err err = 0;	/* For result error code */
	/* Print path to binary file. */
	fprintf(stdout, "; %s\n\n", path ? path : "STDIN");
	if (path && (in = fopen(path, "rb")) == 0) {
		perror("fopen");
		return 1;
	}
	if ((err = a86_trans(in, stdout))) {
		a86_fperror(stderr, err);
	}
	if (in != stdin && fclose(in) == EOF) {
		perror("fclose");
		return 1;
	}
	return err;
}
