#include <ctype.h>
#include <dirent.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "histedit.h"

#if defined(__GNUC__)
#define __demo_unused __attribute__((__unused__))
#else
#define __demo_unused
#endif

static int cont = 0;
static volatile sig_atomic_t sig = 0;

static char* prompt(EditLine *el __demo_unused)
{
	static char a[] = "> ";
	static char b[] = "| ";
	return (cont ? b : a);
}

static void sig_handler(int i)
{
	sig = i;
}

int main(int argc __demo_unused, char **argv) {
	EditLine *el = NULL;
	const char *buf;
	Tokenizer *tok;
	History *hist;
	HistEvent ev;
	int num, ncont;
	int ac, cc, co;
	const char **av;
	const LineInfo *li;

	setlocale(LC_CTYPE, "");

	signal(SIGINT, sig_handler);
	signal(SIGQUIT, sig_handler);
	signal(SIGHUP, sig_handler);
	signal(SIGTERM, sig_handler);

	hist = history_init();
	history(hist, &ev, H_SETSIZE, 100);

	tok  = tok_init(NULL);
	el = el_init(*argv, stdin, stdout, stderr);

	el_set(el, EL_EDITOR, "emacs");
	el_set(el, EL_SIGNAL, 1);
	el_set(el, EL_PROMPT_ESC, prompt, '\1');
	el_set(el, EL_HIST, history, hist);

	el_source(el, NULL);

	for (;;)
	{
		buf = el_gets(el, &num);
		li = el_line(el);

		if (sig) {
			fprintf(stderr, "signal %d\n", (int)sig);
			sig = 0;
			continue;
		}

		if (buf == NULL) {
			//fprintf(stderr, "\n");
			continue;
		}

		if (!cont && num == 1) {
			continue;
		}

		ac = cc = co = 0;
		ncont = tok_line(tok, li, &ac, &av, &cc, &co);
		if (ncont < 0) {
			fprintf(stderr, "internal error\n");
			cont = 0;
			continue;
		}

		history(hist, &ev, cont ? H_APPEND : H_ENTER, buf);

		cont = ncont;
		ncont = 0;
		if (cont) {
			continue;
		}

		if (strcmp(av[0], "exit") == 0) {
			switch (ac) {
			case 1:
				_Exit(0);
			default:
				fprintf(stderr, "exit: invalid arguments\n");
				break;
			}
		} else if (strcmp(av[0], "history") == 0) {
			switch (ac) {
			case 1:
				for (int rv = history(hist, &ev, H_LAST); rv != -1;
					rv = history(hist, &ev, H_PREV)) {
						fprintf(stdout, "%4d %s", ev.num, ev.str);
					}
				break;
			case 2:
				if (strcmp(av[1], "clear") == 0) {
					history(hist, &ev, H_CLEAR);
				} else {
					fprintf(stderr, "history: invalid arguments\n");                    
				}
				break;
			case 3:
				if (strcmp(av[1], "load") == 0) {
					history(hist, &ev, H_LOAD, av[2]);
				} else if (strcmp(av[1], "save") == 0) {
					history(hist, &ev, H_SAVE, av[2]);
				} else {
					fprintf(stderr, "history: invalid arguments\n");                    
				}
				break;
			default:
				fprintf(stderr, "history: invalid arguments\n");                    
				break;
			}
		} else if (el_parse(el, ac, av) == -1) {
			for (int i = 0; i < ac; i++) {
				fprintf(stdout, "arg[%d]=\"%s\"\n", i, av[i]);
			}
		}

		tok_reset(tok);
	}

	el_end(el);
	tok_end(tok);
	history_end(hist);

	return (0);
}
