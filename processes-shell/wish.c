#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define EXIT "exit"
#define PATH "path"
#define CD "cd"
#define DEFAULT_PATH "/bin"

// TODO move to header file
char *prepend(const char *restrict, const char *restrict, const char *);
char *redirect(char **);
char **get_next_args(char ***, size_t *);
char **tokenize(const char *, const char *, const char *);
FILE get_output(int *, const char **argv);
int execute(char **, char **);
size_t count_args(const char *, const char *, const char *); 
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
	const char *ops = ">&;";
	// TODO move fp logic to function `get_output`
	FILE *fp = stdin;

	// special case: batch mode
	bool batch = false;
	switch (argc) {
		case 1:
			break;
		case 2:
			batch = true;
			break;
		default:
			err_routine(true); 
	}

	if (batch) {
		fp = fopen(argv[1], "r");
		if (fp == NULL) {
			err_routine(true);
		}
	}

	// initialize path
	char **path = tokenize(DEFAULT_PATH, delims, ops);

	// run commands
	char **tokens = malloc(0);
	tokens[0] = NULL;
	while (true) {
		free_tokens(tokens);
		if (!batch) printf("wish> ");
		if ((linelen = getline(&line, &linecap, fp)) < 0) {
			free_tokens(path);
			exit(0);
		}	

		tokens = tokenize(line, delims, ops);
		char **tokens_cpy = tokens;
		char **args;
		size_t nargs = 0;
		while ((args = get_next_args(&tokens_cpy, &nargs)) != NULL) {
			if (nargs == 0) continue;
			char *cmd = args[0]; 

			// first check for built-ins
			if (strcmp(cmd, EXIT) == 0) {
				cmd_exit(args, line, nargs);	
			} else if (strcmp(cmd, CD) == 0) {
				cmd_cd(args, nargs);
			} else if (strcmp(cmd, PATH) == 0) {
				cmd_path(&path, args, nargs);
			}

			// not a built-in command, call from path
			else {
				int rc = fork();
				if (rc < 0) {
					err_routine(true);
				}
				// child [DEBUG: parent]
				else if (rc == 0) {
					if ((execute(args, path) == -1)) {
						free_tokens(tokens);
						err_routine(true);
					}
				}
			}
		}
		// parent: wait for ALL child processes
		while (wait(NULL) > 0);
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
	// handle redirection
	char *out;
	if ((out = redirect(args)) != NULL) {
		int fd = open(out, O_WRONLY | O_TRUNC | O_CREAT);
		// redirect stdout in to file
		if (fd < 0 || (fd = dup2(fd, STDOUT_FILENO) < 0 )) err_routine(true);
	}

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


char **get_next_args(char ***tokensp, size_t *nargs) {
	if (**tokensp == NULL) {
		return NULL;
	}

	// count args
	char **start = *tokensp;
	*nargs = 0;
	for (char **stop = start; strcmp(*stop, "&") != 0; ++stop) {
		*nargs += 1;
		if (*(stop + 1) == NULL) break;			
	}	
	
	// copy args	
	char **args = malloc(sizeof(char *) * (*nargs + 1));
	for (int i = 0; i < *nargs; ++i) {
		args[i] = strdup(**tokensp);	
		*tokensp += 1;
	}
	args[*nargs] = NULL;
	
	// advance tokens pointer past "&"
	if (**tokensp != NULL) *tokensp += 1;
	
	return args;
}


char *redirect(char **args) {
	// count number of args by locating NULL pointer
	size_t nargs = 0;
	for (char **p = args; *p != NULL; p++) {
		nargs++;
	}

	// ensure '>' only allowed if at least two args, and only second to last
	for (int i = 0; i < nargs; ++i) {
		if (strcmp(args[i], ">") == 0 && (nargs < 3 || i != nargs - 2)) {
			err_routine(true);
		}
	}	
	// check that penultimate arg is '>'
	if (nargs < 3 || strcmp(args[nargs - 2], ">") != 0) {
		return NULL;
	}	

	// remove "> [out]" from args
	char *out = strdup(args[nargs - 1]);
	free(args[nargs - 1]);
	free(args[nargs - 2]);
	args[nargs - 1] = NULL;
	args[nargs - 2] = NULL;

	// output to last arg
	return out;

}


char *prepend(const char *restrict base, const char *restrict prefix,
		const char *sep) {
	char *s = malloc(strlen(prefix) + strlen(sep)+ strlen(base) + 1);
	strcpy(s, prefix);
	strcat(s, sep);
	strcat(s, base);

	return s;
}

char **tokenize(const char *line, const char *delims, const char *operators) {

	size_t nargs = count_args(line, delims, operators);	

	// tokenize args 
	char **tokens = malloc(sizeof(char *) * (nargs + 1));
	int token_index = 0;
	bool in_word = false;
	const char *start = line;
	const char *end = line;
	while (*end != '\0') {
		// delimiter encountered
		if (strchr(delims, *end) != NULL) {
			// previously in word, make new token
			if (in_word) {
				tokens[token_index++] = strndup(start, end - start);
				in_word = false;
				start = end + 1;
			} 
			// not in word, advance start
			else {
				start++;
			}
		} 
		// operator encountered
		else if (strchr(operators, *end) != NULL) {
			// previously in word, make new token
			if (in_word) {
				tokens[token_index++] = strndup(start, end - start);
				in_word = false;
			}
			// add operator itself to tokens 
			start = end;
			tokens[token_index++] = strndup(start, 1);
			start = end + 1;
		}
		// alphanumeric encountered
		else {
			in_word = true;
		}
		end++;
	}
	// append final token
	if (in_word) {
		tokens[token_index++] = strndup(start, end - start);
	}	

	tokens[token_index] = NULL; 
	return tokens;
}


size_t count_args(const char *line, const char *delims, const char *ops) {
	char delims_and_ops[strlen(delims) + strlen(ops) + 1];
	strcpy(delims_and_ops, delims);
	strcat(delims_and_ops, ops);

	// count number of non-operator args 
	size_t nargs = 0;
	const char *p = line;
	// skip any initial whitespace
	p += strspn(p, delims_and_ops);
	while (*p != '\0') {
		p += strcspn(p, delims_and_ops);
		p += strspn(p, delims_and_ops);
		nargs += 1;
	}

	// count number of operators
	p = strpbrk(line, ops);
	while (p != NULL) {
		nargs += 1;
		p = strpbrk(p + 1, ops);
	}

	return nargs;
}


void free_tokens(char **tokens) {
	for (char **p = tokens; *p != NULL; p++) {
		free(*p);
	}
	free(tokens);
}

