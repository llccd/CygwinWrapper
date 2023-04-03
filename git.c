#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/cygwin.h>
#include <sys/wait.h>

static inline bool testPath(char* s) {
	return strlen(s) >=3 &&
		s[1] == ':' &&
		(s[2] == '/' || s[2] == '\\') &&
		(s[0] >= 'A' && s[0] <= 'Z' || s[0] >= 'a' && s[0] <= 'z');
}

int main(int argc, char** argv) {
	// git rev-parse
	if (argc >= 3 && !strcmp(argv[1], "rev-parse") && strcmp(argv[2], "--help")) {
		int pipefd[2];
		pipe(pipefd);
		int pid = fork();
		if (pid == -1)
			err(1, "fork");
		if (pid == 0) {
			close(pipefd[0]);
			dup2(pipefd[1], 1);
			close(pipefd[1]);
			// fix "@{}" patten
			for (int i = 2; i < argc; i++) {
				char *s = argv[i];
				int len = strlen(s);
				if (len >= 2 && s[0] == '@' && s[1] != '{') {
					char *s2 = (char *) malloc(len + 3);
					s2[0] = '@';
					s2[1] = '{';
					memcpy(s2 + 2, s + 1, len - 1);
					s2[len + 1] = '}';
					s2[len + 2] = 0;
					argv[i] = s2;
				}
			}
			execv("/usr/bin/git", argv);
			return 1;
		}
		close(pipefd[1]);
		FILE* fd = fdopen(pipefd[0], "r");
		char * line = NULL;
		size_t len = 0;
		// convert results to Windows path
		while (getline(&line, &len, fd) != -1) {
			if (line[0] == '/') {
				ssize_t size;
				char *s;
				size = cygwin_conv_path(CCP_POSIX_TO_WIN_A | CCP_RELATIVE, line, NULL, 0);
				if (size < 0)
					err(1, "cygwin_conv_path");
				else {
					s = (char *) malloc(size);
					if (cygwin_conv_path(CCP_POSIX_TO_WIN_A | CCP_RELATIVE, line, s, size))
						err(1, "cygwin_conv_path");
				}
				printf("%s", s);
			}
			else
				printf("%s", line);
		}
		close(pipefd[0]);
		if (line)
			free(line);

		int status;
		if ( waitpid(pid, &status, 0) == -1 )
			err(1, "waitpid");

		if (WIFEXITED(status))
			return WEXITSTATUS(status);

		return 1;
	}

	// convert arguments after "--" to Unix path
	bool found = false;
	for (int i = 2; i < argc; i++) {
		char *s = argv[i];
		if (!found) {
			if (!strcmp(s, "--"))
				found = true;
			continue;
		}
		if (testPath(s)) {
			ssize_t size;
			char *posix;
			size = cygwin_conv_path(CCP_WIN_A_TO_POSIX | CCP_RELATIVE, s, NULL, 0);
			if (size < 0)
				err(1, "cygwin_conv_path");
			else {
				posix = (char *) malloc(size);
				if (cygwin_conv_path(CCP_WIN_A_TO_POSIX | CCP_RELATIVE, s, posix, size))
					err(1, "cygwin_conv_path");
			}
			argv[i] = posix;
		}
	}

	execv("/usr/bin/git", argv);
	return 1;
}