#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define EXIT "exit"
#define PATH "path"
#define CD "cd"
#define DEFAULT_PATH "/bin"


char *prepend(const char *restrict, const char *restrict, const char *);
char **tokenize(const char *, const char *, size_t *);
int execute(char **, char **path);
void cmd_exit(char **, char *, size_t);
void cmd_cd(char **, size_t);
void cmd_path(char ***, char **, size_t);
void err_routine(bool);
void free_tokens(char **);


int main(int argc, char *argv[]) {
	char *line = NULL;
	size_t linecap = 0;
	int linelen = 0;
	const char *delims = " \t\n";
	FILE *fp = stdin;
	bool batch = (argc > 1);

	// special case: batch mode
	if (batch) {
		fp = fopen(argv[1], "r");
		if (fp == NULL) {
			err_routine(true);
		}
	}

	size_t nargs;
	char **path = tokenize(DEFAULT_PATH, delims, &nargs);
	char **tokens = malloc(0);
	tokens[0] = NULL;
	while (true) {
		free_tokens(tokens);
		if (!batch) printf("wish> ");
		if ((linelen = getline(&line, &linecap, fp)) < 0) {
			free_tokens(path);
			exit(0);
		}	

		tokens = tokenize(line, delims, &nargs);
		char *cmd = tokens[0]; 

		// first check for built-ins
		if (strcmp(cmd, EXIT) == 0) {
			cmd_exit(tokens, line, nargs);	
		} else if (strcmp(cmd, CD) == 0) {
			cmd_cd(tokens, nargs);
		} else if (strcmp(cmd, PATH) == 0) {
			cmd_path(&path, tokens, nargs);
		}
			
		// not a built-in command, call from path
		else {
			int rc = fork();
			if (rc < 0) {
				err_routine(true);
			}
			// child [DEBUG: parent]
			else if (rc == 0) {
				if ((execute(tokens, path) == -1)) {
					free_tokens(tokens);
					err_routine(true);
				}
			}
			// parent
			else {					
				wait(NULL);
			}
		}
	}
}


void cmd_exit(char **tokens, char *line, size_t nargs) {
	if (nargs > 1) {
		err_routine(false);
	} else {
		free_tokens(tokens);
		free(line);
		exit(0);
	}
}


void cmd_cd(char **tokens, size_t nargs) {
	if (nargs != 2 || chdir(tokens[1]) < 0) {
		err_routine(false);
	}
}


void cmd_path(char ***pathp, char **tokens, size_t nargs) {
	free_tokens(*pathp);
	char **path = malloc(sizeof(char *) * nargs);
	for (int i = 0; i < nargs - 1; ++i) { 
		path[i] = strdup(tokens[i + 1]); 
	}
	path[nargs - 1] = NULL;
	*pathp = path;
}

void err_routine(bool call_exit) {
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message)); 
	if (call_exit) exit(1);
}


int execute(char **args, char **path) {
	for (char **p = path; *p != NULL; ++p) {
		char *prepended = prepend(args[0], *p, "/");
		if (access(prepended, X_OK) == 0) {
			// args must contain *p
			free(args[0]);
			args[0] = prepended;
			return execv(args[0], args);
		}
		free(prepended);
	}

	return -1;
}


char *prepend(const char *restrict base, const char *restrict prefix,
		const char *sep) {
	char *s = malloc(strlen(prefix) + strlen(sep)+ strlen(base) + 1);
	strcpy(s, prefix);
	strcat(s, sep);
	strcat(s, base);

	return s;
}


char **tokenize(const char *line, const char *delims, size_t *nargs) {
	// count number of args
	*nargs = 0;
	const char *p = line;
	while (*p != '\0') {
		p += strcspn(p, delims);
		p += strspn(p, delims);
		*nargs += 1;
	}
	// tokenize args (final arg must be NULL for execv)
	char **tokens = malloc((*nargs + 1) * sizeof(char *));
	char *token, *tofree, *linedup; 
	tofree = linedup = strdup(line);
	int i = 0;
	while ((token = strsep(&linedup, delims)) != NULL) {
		// ensure token is not delimiter
		if (strlen(token) > 0) {
			tokens[i] = strdup(token);
			i++;
		}
	} 
	tokens[i] = NULL;
	free(tofree);

	return tokens;
}


void free_tokens(char **tokens) {
	for (char **p = tokens; *p != NULL; p++) {
		free(*p);
	}
	free(tokens);
}
