/*
 *
 * Copyright 1986, 1987, 1988
 * by MIT Student Information Processing Board.
 *
 * For copyright info, see "mit-sipb-copyright.h".
 *
 */

#include "mit-sipb-copyright.h"

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

static const char copyright[] =
    "Copyright 1987,1988 by MIT Student Information Processing Board";

static const char rcsid[] = "$Id: compile_et.c,v 1.10 2002-09-10 16:05:02 ghudson Exp $";

enum lang {
    lang_C,			/* ANSI C (default) */
    lang_KRC,			/* C: ANSI + K&R */
    lang_CPP			/* C++ */
};

/* From gensym.y. */
extern char *gensym(const char *x);
extern int yyparse(void);
extern char *current_token;
extern int table_number, current;

/* From et_lex.lex.l. */
extern FILE *yyin;
extern int num_lines;

FILE *hfile;
FILE *cfile;
char *table_name = NULL;

static int debug;			/* dump debugging info? */
static char *filename;			/* error table source */
static enum lang language;
static const char *whoami;

static char *xmalloc(unsigned int size)
{
    char * p = malloc (size);

    if (!p) {
	perror (whoami);
	exit (1);
    }
    return p;
}

static int check_arg(const char *const *str_list, const char *arg)
{
    while (*str_list)
	if (!strcmp(arg, *str_list++))
	    return 1;
    return 0;
}

static const char *const debug_args[] = {
    "d",
    "debug",
    0,
};

static const char *const lang_args[] = {
    "lang",
    "language",
    0,
};

static const char *const language_names[] = {
    "C",
    "K&R C",
    "C++",
    0,
};

static const char * const c_src_prolog[] = {
    "static const char * const text[] = {\n",
    0,
};

static const char * const krc_src_prolog[] = {
    "#ifdef __STDC__\n",
    "#define NOARGS void\n",
    "#else\n",
    "#define NOARGS\n",
    "#define const\n",
    "#endif\n\n",
    "static const char * const text[] = {\n",
    0,
};

static const char *const struct_def[] = {
    "struct error_table {\n",
    "    char const * const * msgs;\n",
    "    long base;\n",
    "    int n_msgs;\n",
    "};\n",
    "struct et_list {\n",
    "    struct et_list *next;\n",
    "    const struct error_table * table;\n",
    "};\n",
    "extern struct et_list *_et_list;\n",
    "\n", 0,
};

static const char warning[] =
    "/*\n * %s:\n * This file is automatically generated; please do not edit it.\n */\n";

/* pathnames */
char c_file[MAXPATHLEN];	/* output file */
char h_file[MAXPATHLEN];	/* output */

static void usage(void) {
    fprintf (stderr, "%s: usage: %s ERROR_TABLE\n",
	     whoami, whoami);
    exit (1);
}

static void dup_err(const char *type, const char *one, const char *two)
{
    fprintf (stderr, "%s: multiple %s specified: `%s' and `%s'\n",
	     whoami, type, one, two);
    usage ();
}

int main(int argc, char **argv)
{
    char *p, *ename;
    int len;
    char const * const *cpp;
    int got_language = 0;

    /* argument parsing */
    debug = 0;
    filename = 0;
    whoami = argv[0];
    p = strrchr (whoami, '/');
    if (p)
	whoami = p+1;
    while (argv++, --argc) {
	char *arg = *argv;
	if (arg[0] != '-') {
	    if (filename)
		dup_err ("filenames", filename, arg);
	    filename = arg;
	}
	else {
	    arg++;
	    if (check_arg (debug_args, arg))
		debug++;
	    else if (check_arg (lang_args, arg)) {
		got_language++;
		arg = *++argv, argc--;
		if (!arg)
		    usage ();
		if (language)
		    dup_err ("languanges", language_names[(int)language], arg);
#define check_lang(x,v) else if (!strcasecmp(arg,x)) language = v
		check_lang ("c", lang_C);
		check_lang ("ansi_c", lang_C);
		check_lang ("ansi-c", lang_C);
		check_lang ("krc", lang_KRC);
		check_lang ("kr_c", lang_KRC);
		check_lang ("kr-c", lang_KRC);
		check_lang ("k&r-c", lang_KRC);
		check_lang ("k&r_c", lang_KRC);
		check_lang ("c++", lang_CPP);
		check_lang ("cplusplus", lang_CPP);
		check_lang ("c-plus-plus", lang_CPP);
#undef check_lang
		else {
		    fprintf (stderr, "%s: unknown language name `%s'\n",
			     whoami, arg);
		    fprintf (stderr, "\tpick one of: C K&R-C\n");
		    exit (1);
		}
	    }
	    else {
		fprintf (stderr, "%s: unknown control argument -`%s'\n",
			 whoami, arg);
		usage ();
	    }
	}
    }
    if (!filename)
	usage ();
    if (!got_language)
	language = lang_C;
    else if (language == lang_CPP) {
	fprintf (stderr, "%s: Sorry, C++ support is not yet finished.\n",
		 whoami);
	exit (1);
    }

    p = xmalloc (strlen (filename) + 5);
    strcpy (p, filename);
    filename = p;
    p = strrchr(filename, '/');
    if (p == NULL)
	p = filename;
    else
	p++;
    ename = p;
    len = strlen (ename);
    p += len - 3;
    if (strcmp (p, ".et"))
	p += 3;
    *p++ = '.';
    /* now p points to where "et" suffix should start */
    /* generate new filenames */
    strcpy (p, "c");
    strcpy (c_file, ename);
    *p = 'h';
    strcpy (h_file, ename);
    strcpy (p, "et");

    yyin = fopen(filename, "r");
    if (!yyin) {
	perror(filename);
	exit(1);
    }

    hfile = fopen(h_file, "w");
    if (hfile == NULL) {
	perror(h_file);
	exit(1);
    }
    fprintf (hfile, warning, h_file);

    cfile = fopen(c_file, "w");
    if (cfile == NULL) {
	perror(c_file);
	exit(1);
    }
    fprintf (cfile, warning, c_file);

    /* prologue */
    if (language == lang_C)
	cpp = c_src_prolog;
    else if (language == lang_KRC)
	cpp = krc_src_prolog;
    else
	abort ();
    while (*cpp)
	fputs (*cpp++, cfile);

    /* parse it */
    yyparse();
    fclose(yyin);		/* bye bye input file */

    fputs ("    0\n};\n\n", cfile);
    for (cpp = struct_def; *cpp; cpp++)
	fputs (*cpp, cfile);
    fprintf(cfile,
	    "const struct error_table et_%s_error_table = { text, %ldL, %d };",
	    table_name, (long) table_number, current);
    fputs("\n\nstatic struct et_list link = { 0, 0 };\n\n",
	  cfile);
    fprintf(cfile, "void initialize_%s_error_table (%s);\n\n",
	    table_name, (language == lang_C) ? "void" : "");
    fprintf(cfile, "void initialize_%s_error_table (%s) {\n",
	    table_name, (language == lang_C) ? "void" : "NOARGS");
    fputs("    if (!link.table) {\n", cfile);
    fputs("        link.next = _et_list;\n", cfile);
    fprintf(cfile, "        link.table = &et_%s_error_table;\n", table_name);
    fputs("        _et_list = &link;\n", cfile);
    fputs("    }\n", cfile);
    fputs("}\n", cfile);
    fclose(cfile);

    if (language == lang_KRC) {
	fprintf (hfile, "extern void initialize_%s_error_table ();\n",
		 table_name);
    } else {
	fprintf (hfile, "void initialize_%s_error_table (void);\n",
		 table_name);
    }
    fprintf (hfile, "#define ERROR_TABLE_BASE_%s (%ldL)\n",
	     table_name, (long) table_number);
    fprintf (hfile, "struct error_table;\n");
    fprintf (hfile, "extern const struct error_table et_%s_error_table;\n",
	     table_name);
    /* compatibility... */
    fprintf (hfile, "\n/* for compatibility with older versions... */\n");
    fprintf (hfile, "#define init_%s_err_tbl initialize_%s_error_table\n",
	     table_name, table_name);
    fprintf (hfile, "#define %s_err_base ERROR_TABLE_BASE_%s\n", table_name,
	     table_name);
    fclose(hfile);		/* bye bye include file */

    return 0;
}

int yyerror(char *s)
{
    fputs(s, stderr);
    fprintf(stderr, "\nLine number %d; last token was '%s'\n",
	    num_lines, current_token);
    return 0;
}
