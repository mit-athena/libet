%{
#include <stdio.h>
#include <stdlib.h>

char *gensym(const char *x);
static char *ds(const char *string);
static void add_ec(const char *name, const char *description);
static void add_ec_val(const char *name, const char *val,
		       const char *description);
static void put_ecs(void);
static int char_to_num(char c);
static void set_table_num(char *string);

static long gensym_n = 0;

char *current_token = NULL;

extern int yyerror(char *s);
extern char *table_name;
%}
%union {
	char *dynstr;
}

%token ERROR_TABLE ERROR_CODE_ENTRY END
%token <dynstr> STRING QUOTED_STRING
%type <dynstr> ec_name description table_id
%{
%}
%start error_table
%%

error_table	:	ERROR_TABLE table_id error_codes END
			{ table_name = ds($2);
			  current_token = table_name;
			  put_ecs(); }
		;

table_id	:	STRING
			{ current_token = $1;
			  set_table_num($1);
			  $$ = $1; }
		;

error_codes	:	error_codes ec_entry
		|	ec_entry
		;

ec_entry	:	ERROR_CODE_ENTRY ec_name ',' description
			{ add_ec($2, $4);
			  free($2);
			  free($4); }
		|	ERROR_CODE_ENTRY ec_name '=' STRING ',' description
			{ add_ec_val($2, $4, $6);
			  free($2);
			  free($4);
			  free($6);
			}
		;

ec_name		:	STRING
			{ $$ = ds($1);
			  current_token = $$; }
		;

description	:	QUOTED_STRING
			{ $$ = ds($1);
			  current_token = $$; }
		;

%%
/*
 *
 * Copyright 1986, 1987 by the MIT Student Information Processing Board
 *
 * For copyright info, see mit-sipb-copyright.h.
 */

#include "mit-sipb-copyright.h"

#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include "error_table.h"

static const char rcsid[] = "$Id: error_table.y,v 1.6 1999-11-23 20:27:11 danw Exp $";

#include "et_lex.lex.c"

extern FILE *hfile, *cfile;

char *gensym(const char *x)
{
	char *symbol;

	if (!gensym_n) {
		struct timeval tv;
		struct timezone tzp;
		gettimeofday(&tv, &tzp);
		gensym_n = (tv.tv_sec%10000)*100 + tv.tv_usec/10000;
	}
	symbol = malloc(32 * sizeof(char));
	gensym_n++;
	sprintf(symbol, "et%ld", gensym_n);
	return(symbol);
}

static char *ds(const char *string)
{
	char *rv;

	rv = malloc(strlen(string)+1);
	strcpy(rv, string);
	return(rv);
}

long table_number;
int current = 0;
char **error_codes = NULL;

static void add_ec(const char *name, const char *description)
{
	fprintf(cfile, "\t\"%s\",\n", description);
	if (error_codes == NULL) {
		error_codes = malloc(sizeof(char *));
		*error_codes = NULL;
	}
	error_codes = realloc(error_codes, (current + 2) * sizeof(char *));
	error_codes[current++] = ds(name);
	error_codes[current] = NULL;
}

static void add_ec_val(const char *name, const char *val,
		       const char *description)
{
	const int ncurrent = atoi(val);

	if (ncurrent < current) {
		printf("Error code %s (%d) out of order", name,
		       current);
		return;
	}
      
	while (ncurrent > current)
	     fputs("\tNULL,\n", cfile), current++;
	
	fprintf(cfile, "\t\"%s\",\n", description);
	if (error_codes == NULL) {
		error_codes = malloc(sizeof(char *));
		*error_codes = NULL;
	}
	error_codes = realloc(error_codes, (current + 2) * sizeof(char *));
	error_codes[current++] = ds(name);
	error_codes[current] = NULL;
} 

static void put_ecs(void)
{
	int i;
	for (i = 0; i < current; i++) {
	     if (error_codes[i] != NULL)
		  fprintf(hfile, "#define %-40s (%ldL)\n",
			  error_codes[i], table_number + i);
	}
}

/*
 * char_to_num -- maps letters and numbers into a small numbering space
 * 	uppercase ->  1-26
 *	lowercase -> 27-52
 *	digits    -> 53-62
 *	underscore-> 63
 */

static const char char_set[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

static int char_to_num(char c)
{
	const char *where;
	int diff;

	where = strchr (char_set, c);
	if (where) {
		diff = where - char_set + 1;
		assert (diff < (1 << ERRCODE_RANGE));
		return diff;
	}
	else if (isprint ((unsigned char)c))
		fprintf (stderr,
			 "Illegal character `%c' in error table name\n",
			 c);
	else
		fprintf (stderr,
			 "Illegal character %03o in error table name\n",
			 c);
	exit (1);
}

static void set_table_num(char *string)
{
	if (char_to_num (string[0]) > char_to_num ('z')) {
		fprintf (stderr, "%s%s%s%s",
			 "First character of error table name must be ",
			 "a letter; name ``",
			 string, "'' rejected\n");
		exit (1);
	}
	if (strlen(string) > 4) {
		fprintf(stderr, "Table name %s too long, truncated ",
			string);
		string[4] = '\0';
		fprintf(stderr, "to %s\n", string);
	}
	while (*string != '\0') {
		table_number = (table_number << BITS_PER_CHAR)
			+ char_to_num(*string);
		string++;
	}
	table_number = table_number << ERRCODE_RANGE;
}
