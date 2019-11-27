#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define EXIT "exit\n"

void myfprint(FILE *);


int main(int argc, char *argv[]) {

	// batch
	if (argc > 1) {
		FILE *fp; 
		if ((fp = fopen(argv[1], "r")) == NULL) {
			fprintf(stderr, "file %s could not be opened\n", argv[1]);
			exit(1);
		}	
		myfprint(fp);
		fclose(fp);
		exit(0);
	}

	// interactive
	while (true) {
		printf("wish> ");
		myfprint(stdin);	
	}
	return 0;
}


void myfprint(FILE *fp) {
	char *linep = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while (true) {
		linelen = getline(&linep, &linecap, fp);
		if (linelen < 0 || !strcmp(linep, EXIT)) { 
			fclose(fp);
			exit(0);
		}
		printf("%s(%ld chars, line buffer size %zu)\n", 
				linep, linelen, linecap);
	}
}
