#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/cygwin.h>

static inline bool testPath(char* s) {
	return strlen(s) >=3 &&
		s[1] == ':' &&
		(s[2] == '/' || s[2] == '\\') &&
		(s[0] >= 'A' && s[0] <= 'Z' || s[0] >= 'a' && s[0] <= 'z');
}

int main(int argc, char** argv) {
	// convert arguments to Unix path
	for (int i = 1; i < argc; i++) {
		char *win32 = argv[i];
		if (testPath(win32)) {
			ssize_t size;
			char *posix;
			size = cygwin_conv_path(CCP_WIN_A_TO_POSIX | CCP_RELATIVE, win32, NULL, 0);
			if (size < 0)
				err(1, "cygwin_conv_path");
			else {
				posix = (char *) malloc(size);
				if (cygwin_conv_path(CCP_WIN_A_TO_POSIX | CCP_RELATIVE, win32, posix, size))
					err(1, "cygwin_conv_path");
			}
			argv[i] = posix;
		}
	}
	execv("/usr/bin/scp", argv);
	return 1;
}