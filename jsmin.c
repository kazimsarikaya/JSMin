/* jsmin.c
   2019-10-30

   Copyright (C) 2002 Douglas Crockford  (www.crockford.com)

   Permission is hereby granted, free of charge, to any person obtaining a copy of
   this software and associated documentation files (the "Software"), to deal in
   the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is furnished to do
   so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   The Software shall be used for Good, not Evil.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

typedef struct _state {
	int the_a;
	int the_b;
	int look_ahead;
	int the_x;
	int the_y;
} state_t;

void error(char* string) {
	fputs("JSMIN Error: ", stderr);
	fputs(string, stderr);
	fputc('\n', stderr);
	exit(1);
}

/* is_alphanum -- return true if the character is a letter, digit, underscore,
        dollar sign, or non-ASCII character.
 */

int is_alphanum(int codeunit) {
	return (
		(codeunit >= 'a' && codeunit <= 'z')
		|| (codeunit >= '0' && codeunit <= '9')
		|| (codeunit >= 'A' && codeunit <= 'Z')
		|| codeunit == '_'
		|| codeunit == '$'
		|| codeunit == '\\'
		|| codeunit > 126
		);
}


/* get -- return the next character from stdin. Watch out for lookahead. If
        the character is a control character, translate it to a space or
        linefeed.
 */

int get(state_t * state) {
	int codeunit = state->look_ahead;
	state->look_ahead = EOF;
	if (codeunit == EOF) {
		codeunit = getc(stdin);
	}
	if (codeunit >= ' ' || codeunit == '\n' || codeunit == EOF) {
		return codeunit;
	}
	if (codeunit == '\r') {
		return '\n';
	}
	return ' ';
}


/* peek -- get the next character without advancing.
 */

int peek(state_t *state) {
	state->look_ahead = get(state);
	return state->look_ahead;
}


/* next -- get the next character, excluding comments. peek() is used to see
        if a '/' is followed by a '/' or '*'.
 */

int next(state_t *state) {
	int codeunit = get(state);
	if  (codeunit == '/') {
		switch (peek(state)) {
		case '/':
			for (;;) {
				codeunit = get(state);
				if (codeunit <= '\n') {
					break;
				}
			}
			break;
		case '*':
			get(state);
			while (codeunit != ' ') {
				switch (get(state)) {
				case '*':
					if (peek(state) == '/') {
						get(state);
						codeunit = ' ';
					}
					break;
				case EOF:
					error("Unterminated comment.");
				}
			}
			break;
		}
	}
	state->the_y = state->the_x;
	state->the_x = codeunit;
	return codeunit;
}


/* action -- do something! What you do is determined by the argument:
        1   Output A. Copy B to A. Get the next B.
        2   Copy B to A. Get the next B. (Delete A).
        3   Get the next B. (Delete B).
   action treats a string as a single character.
   action recognizes a regular expression if it is preceded by the likes of
   '(' or ',' or '='.
 */

void action(int determined, state_t *state) {
	switch (determined) {
	case 1:
		putc(state->the_a, stdout);
		if (
			(state->the_y == '\n' || state->the_y == ' ')
			&& (state->the_a == '+' || state->the_a == '-' || state->the_a == '*' || state->the_a == '/')
			&& (state->the_b == '+' || state->the_b == '-' || state->the_b == '*' || state->the_b == '/')
			) {
			putc(state->the_y, stdout);
		}
	case 2:
		state->the_a = state->the_b;
		if (state->the_a == '\'' || state->the_a == '"' || state->the_a == '`') {
			for (;;) {
				putc(state->the_a, stdout);
				state->the_a = get(state);
				if (state->the_a == state->the_b) {
					break;
				}
				if (state->the_a == '\\') {
					putc(state->the_a, stdout);
					state->the_a = get(state);
				}
				if (state->the_a == EOF) {
					error("Unterminated string literal.");
				}
			}
		}
	case 3:
		state->the_b = next(state);
		if (state->the_b == '/' && (
					state->the_a == '(' || state->the_a == ',' || state->the_a == '=' || state->the_a == ':'
					|| state->the_a == '[' || state->the_a == '!' || state->the_a == '&' || state->the_a == '|'
					|| state->the_a == '?' || state->the_a == '+' || state->the_a == '-' || state->the_a == '~'
					|| state->the_a == '*' || state->the_a == '/' || state->the_a == '{' || state->the_a == '}'
					|| state->the_a == ';'
					)) {
			putc(state->the_a, stdout);
			if (state->the_a == '/' || state->the_a == '*') {
				putc(' ', stdout);
			}
			putc(state->the_b, stdout);
			for (;;) {
				state->the_a = get(state);
				if (state->the_a == '[') {
					for (;;) {
						putc(state->the_a, stdout);
						state->the_a = get(state);
						if (state->the_a == ']') {
							break;
						}
						if (state->the_a == '\\') {
							putc(state->the_a, stdout);
							state->the_a = get(state);
						}
						if (state->the_a == EOF) {
							error(
								"Unterminated set in Regular Expression literal."
								);
						}
					}
				} else if (state->the_a == '/') {
					switch (peek(state)) {
					case '/':
					case '*':
						error(
							"Unterminated set in Regular Expression literal."
							);
					}
					break;
				} else if (state->the_a =='\\') {
					putc(state->the_a, stdout);
					state->the_a = get(state);
				}
				if (state->the_a == EOF) {
					error("Unterminated Regular Expression literal.");
				}
				putc(state->the_a, stdout);
			}
			state->the_b = next(state);
		}
	}
}


/* jsmin -- Copy the input to the output, deleting the characters which are
        insignificant to JavaScript. Comments will be removed. Tabs will be
        replaced with spaces. Carriage returns will be replaced with linefeeds.
        Most spaces and linefeeds will be removed.
 */

void jsmin(state_t *state) {
	if (peek(state) == 0xEF) {
		get(state);
		get(state);
		get(state);
	}
	state->the_a = '\n';
	action(3,state);
	while (state->the_a != EOF) {
		switch (state->the_a) {
		case ' ':
			action(
				is_alphanum(state->the_b)
				        ? 1
				        : 2,
				state
				);
			break;
		case '\n':
			switch (state->the_b) {
			case '{':
			case '[':
			case '(':
			case '+':
			case '-':
			case '!':
			case '~':
				action(1,state);
				break;
			case ' ':
				action(3,state);
				break;
			default:
				action(
					is_alphanum(state->the_b)
					          ? 1
					          : 2,
					state
					);
			}
			break;
		default:
			switch (state->the_b) {
			case ' ':
				action(
					is_alphanum(state->the_a)
					          ? 1
					          : 3,
					state
					);
				break;
			case '\n':
				switch (state->the_a) {
				case '}':
				case ']':
				case ')':
				case '+':
				case '-':
				case '"':
				case '\'':
				case '`':
					action(1,state);
					break;
				default:
					action(
						is_alphanum(state->the_a)
						            ? 1
						            : 3,
						state
						);
				}
				break;
			default:
				action(1,state);
				break;
			}
		}
	}
}


/* main -- Output any command line arguments as comments
        and then minify the input.
 */

extern int main(int argc, char* argv[]) {
	int i;
	for (i = 1; i < argc; i += 1) {
		fprintf(stdout, "// %s\n", argv[i]);
	}
	state_t *state=(state_t*)malloc(sizeof(state_t));
	state->look_ahead=EOF;
	state->the_x=EOF;
	state->the_y=EOF;
	jsmin(state);
	free(state);
	return 0;
}
