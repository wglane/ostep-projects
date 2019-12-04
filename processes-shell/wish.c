#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define EXIT "exit"
#define PATH "path"
#define CD "cd"
#define BIN "/bin/"
#define USRBIN "/usr/bin/"


void err_routine();
void prepend_path(char **);

int main(int argc, char *argv[]) {
	char *line = NULL;
	size_t linecap = 0;
	int linelen = 0;
	FILE *fp = stdin;
	bool batch = (argc > 1);

	// special case: batch mode
	if (batch) {
		fp = fopen(argv[1], "r");
		if (fp == NULL) {
			err_routine();
		}
	}

	while (true) {
		if (!batch) printf("wish> ");
		if ((linelen = getline(&line, &linecap, fp)) < 0) {
			exit(0);	
		}
		// duplicate line to avoid mutating with strsep
		char *linedup = strdup(line);
		char *token = strsep(&linedup, "\n");

		// first check for built-ins
		// TODO call err_routine if args are passed to EXIT
		if (strcmp(token, EXIT) == 0) {
			free(line);
			exit(0);
		} else if (strcmp(token, CD) == 0) {
			// TODO
		} else if (strcmp(token, PATH) == 0) {
			// TODO
		}

		// not a built-in command, call from path
		else {
			int rc = fork();
			if (rc < 0) {
				err_routine();
			}
			// child
			else if (rc == 0) { 	
				char *args[2];
				args[0] = (token == '\0') ? NULL : token; 
				args[1] = NULL;
				prepend_path(&token);
				if ((execv(args[0], args)) == -1) {
					err_routine();
				}
			}
			// parent
			else {					
				wait(NULL);
			}
		}
	}
	free(line);
	exit(0);
}

void err_routine() {
	fprintf(stderr, "An error has occurred\n");
	exit(1);
}

void prepend_path(char **tokenp) {
	char *token = *tokenp;
	if (token == NULL) {
		err_routine();
	}
	int n = strlen(token);
	char *prepended = malloc(strlen(BIN) + n + 1);		
	strcpy(prepended, BIN);
	strcat(prepended, token);
	if (access(prepended, X_OK)) {
		free(token);
		*tokenp = prepended;
	}
}




