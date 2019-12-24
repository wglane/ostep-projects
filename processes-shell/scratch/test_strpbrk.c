#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>


int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: ./test_pstrpbrk string1 [string2 ...]\n");
		exit(1);
	}

	const char *ops = ">&;";
	for (int i = 1; i < argc; ++i) {
		const char *s = argv[i];
		size_t nops = 0;
		const char *p = strpbrk(s, ops);
		while (p != NULL) {
			nops += 1;
			p = strpbrk(p + 1, ops);
		}
		printf("Command %s contains %zu ops.\n", s, nops);

		const char *delims = "\n\t ";
		bool in_word = false;
		char *word;
		const char *start = s;
		const char *end = s;
		while (*end != '\0') {
			if (strchr(delims, *end) != NULL) {
				if (in_word) {
					word = strndup(start, end - start);
					printf("Token: %s\n", word);
					free(word);
					start = end + 1;	
					in_word = false;
				} else {
					start++;
				}
			} 
			else if (strchr(ops, *end) != NULL) {
				if (in_word) {
					word = strndup(start, end - start);
					printf("Token: %s\n", word);
					free(word);
					in_word = false; 
				} 
				start = end; 
				word = strndup(start, 1);
				printf("Token: %s\n", word);
				free(word);
				start = end + 1;	
			} 
			else {
				in_word = true;
			}
			end++;
		}
		if (in_word) {
			word = strndup(start, end - start);
			printf("Token: %s\n", word);
			free(word);
			start = end + 1;	
			in_word = false;
		}
	}
}
