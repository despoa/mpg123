/*
 *   getlopt.c
 *
 *   Oliver Fromme  <oliver.fromme@heim3.tu-clausthal.de>
 *   Tue Apr  8 07:15:13 MET DST 1997
 *
 *   Clarification on int vs. long variable types (and the debug stuff):
 *   Thomas Orgis  <thomas@orgis.org>
 *   Fri Dec 23, 2005
 */

#include "config.h"
#include "debug.h"
#include "getlopt.h"

int loptind = 1;	/* index in argv[] */
int loptchr = 0;	/* index in argv[loptind] */
char *loptarg;		/* points to argument if present, else to option */

#if defined(ultrix) || defined(ULTRIX)
char *strdup (char *src)
{
	char *dest;

	if (!(dest = (char *) malloc(strlen(src)+1)))
		return (NULL);
	return (strcpy(dest, src));
}
#endif

topt *findopt (int islong, char *opt, topt *opts)
{
	if (!opts)
		return (0);
	while (opts->lname) {
		if (islong) {
			if (!strcmp(opts->lname, opt))
				return (opts);
		}
		else
			if (opts->sname == *opt)
				return (opts);
		opts++;
	}
	return (0);
}

int performoption (int argc, char *argv[], topt *opt)
{
	int result = GLO_CONTINUE;

	if (!(opt->flags & GLO_ARG)) { /* doesn't take argument */
		if (opt->var) {
			if (opt->flags & GLO_CHAR) /* var is *char */
			{
				debug1("char at %p", opt->var);
				*((char *) opt->var) = (char) opt->value;\
			}
			else if(opt->flags & GLO_LONG)
			{
				debug1("long at %p", opt->var);
				*( (long *) opt->var ) = opt->value;
			}
			else if(opt->flags & GLO_INT)
			{
				debug1("int at %p", opt->var);
				*( (int *) opt->var ) = (int) opt->value;
			}
			else debug("Option without type flag! This is a programming error!");
								
			debug("casting assignment done");
		}
		else
			result = opt->value ? opt->value : opt->sname;
	}
	else { /* requires argument */
		if (loptind >= argc)
			return (GLO_NOARG);
		loptarg = argv[loptind++]+loptchr;
		loptchr = 0;
		if (opt->var) {
			if (opt->flags & GLO_CHAR) /* var is *char */
				*((char **) opt->var) = strdup(loptarg);
			else if(opt->flags & GLO_LONG)
				*((long *) opt->var) = atol(loptarg);
			else if(opt->flags & GLO_INT)
				*((int *) opt->var) = atoi(loptarg);
			else debug("Option without type flag! This is a programming error!");
		}
		else
			result = opt->value ? opt->value : opt->sname;
	}
	if (opt->func)
		opt->func(loptarg);
	return (result);
}

int getsingleopt (int argc, char *argv[], topt *opts)
{
	char *thisopt;
	topt *opt;
	static char shortopt[2] = {0, 0};

	if (loptind >= argc)
		return (GLO_END);
	thisopt = argv[loptind];
	debug1("getsingleopt: %s", thisopt);
	if (!loptchr) { /* start new option string */
		if (thisopt[0] != '-' || !thisopt[1]) /* no more options */
			return (GLO_END);
		if (thisopt[1] == '-') { /* "--" */
			if (thisopt[2]) { /* long option */
				loptarg = thisopt+2;
				loptind++;
				if (!(opt = findopt(1, thisopt+2, opts)))
					return (GLO_UNKNOWN);
				else
					return (performoption(argc, argv, opt));
			}
			else { /* "--" == end of options */
				loptind++;
				return (GLO_END);
			}
		}
		else /* start short option(s) */
			loptchr = 1;
	}
	shortopt[0] = thisopt[loptchr];
	loptarg = shortopt;
	opt = findopt(0, thisopt+(loptchr++), opts);
	if (!thisopt[loptchr]) {
		loptind++;
		loptchr = 0;
	}
	if (!opt)
		return (GLO_UNKNOWN);
	else
		return (performoption(argc, argv, opt));
}

int getlopt (int argc, char *argv[], topt *opts)
{
	
	int result;
	
	while ((result = getsingleopt(argc, argv, opts)) == GLO_CONTINUE);
	return (result);
}

/* EOF */
