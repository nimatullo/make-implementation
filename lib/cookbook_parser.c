#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "cookbook.h"
#include "debug.h"

static void unparse_recipe(RECIPE *rp, FILE *out);
static void unparse_task(TASK *tp, FILE *out);
static void unparse_step(STEP *sp, FILE *out);
static void unparse_token(char *tok, FILE *out);

static RECIPE *parse_recipe(FILE *in, int *err);
static RECIPE *parse_recipe_header(FILE *in, int *err);
static TASK *parse_task(FILE *in, int *err);
static STEP *parse_step(FILE *in, int *err);
static char *parse_token(FILE *in, int *err);
static int is_delim(int c);

static int set_dependencies(COOKBOOK *cbp);

static RECIPE *get_recipe(COOKBOOK *cbp, char *name);

static char *peek_token;
static int lineno;

/*
 * Print a cookbook, in a format from which it can be parsed.
 */
void unparse_cookbook(COOKBOOK *cpb, FILE *out) {
    RECIPE *rp;
    for(rp = cpb->recipes; rp != NULL; rp = rp->next) {
	unparse_recipe(rp, out);
    }
    fprintf(out, "\n");
}

/*
 * Print a recipe, in a format from which it can be parsed.
 */
static void unparse_recipe(RECIPE *rp, FILE *out) {
    // Print the recipe header.
    unparse_token(rp->name, out);
    fprintf(out, " : ");
    RECIPE_LINK *link;
    for(link = rp->this_depends_on; link != NULL; link = link->next) {
	unparse_token(link->name, out);
	if(link->next != NULL)
	    fprintf(out, " ");
    }
    fprintf(out, "\n");

    // Print the recipe tasks.
    TASK *task;
    for(task = rp->tasks; task != NULL; task = task->next)
	unparse_task(task, out);

    fprintf(out, "\n");
}

/*
 * Print a task, in a format from which it can be parsed.
 */
static void unparse_task(TASK *tp, FILE *out) {
    STEP *sp;
    fprintf(out, "\t");
    for(sp = tp->steps; sp != NULL; sp = sp->next) {
	unparse_step(sp, out);
	if(sp->next != NULL)
	    fprintf(out, "%s", " | ");
    }
    if(tp->input_file != NULL) {
	fprintf(out, " < ");
	unparse_token(tp->input_file, out); 
    }
    if(tp->output_file != NULL) {
	fprintf(out, " > "); 
	unparse_token(tp->output_file, out); 
    }
    fprintf(out, "\n");
}

/*
 * Print a step, in a format from which it can be parsed.
 */
static void unparse_step(STEP *sp, FILE *out) {
    int i;
    for(i = 0; sp->words[i] != NULL; i++) {
	unparse_token(sp->words[i], out);
	if(sp->words[i+1] != NULL)
	    fprintf(out, " ");
    }
}

/*
 * Print a token, in a format from which it can be parsed.
 */
static void unparse_token(char *tok, FILE *out) {
    char c;
    while((c = *tok) != '\0') {
	if(isspace(c) || is_delim(c) || c == '\\')
	    fputc('\\', out);
	fputc(c, out);
	tok++;
    }
}

/*
 * Parse a cookbook.
 *
 * A non-NULL cookbook is always returned.
 *
 * If parsing was completely successful, then the variable pointed at
 * by errp is set to 0.  If there was any error, then the variable
 * pointed at by errp is set to nonzero.
 * 
 * It is the caller's responsibility to free the data structure returned.
 */
COOKBOOK *parse_cookbook(FILE *in, int *errp) {
    debug("***COOKBOOK");
    COOKBOOK *cbp = calloc(1, sizeof(COOKBOOK));
    *errp = 0;
    lineno = 1;
    
    // A cookbook is a sequence of recipes.
    RECIPE *rp;
    RECIPE **last = &cbp->recipes;
    while((rp = parse_recipe(in, errp)) != NULL) {
	*last = rp;
        last = &rp->next;
    }
    if(ferror(in)) {
	fprintf(stderr, "%d: I/O error reading cookbook\n", lineno);
	(*errp)++;
    }
    if(cbp->recipes == NULL || set_dependencies(cbp))
	(*errp)++;
    return cbp;
}

/*
 * Parse a recipe.
 *
 * Returns the recipe, or NULL if EOF is encountered or an error occurs.
 * In case of error, errno is set.
 */
static RECIPE *parse_recipe(FILE *in, int *errp) {
    debug("***RECIPE");
    // A recipe consists of a header line, followed by a sequence of tasks.
    RECIPE *rp = parse_recipe_header(in, errp);
    if(rp == NULL)
	return NULL;

    // Parse the recipe tasks and link them into the cookbook.
    TASK *task;
    TASK **last = &rp->tasks;
    while((task = parse_task(in, errp)) != NULL) {
	*last = task;
	last = &task->next;
    }

    return rp;
}

/*
 * Parse a recipe header.
 *
 * Returns partially initialized recipe on success, NULL otherwise.
 * In case of error, errno is set.
 */
static RECIPE *parse_recipe_header(FILE *in, int *errp) {
    debug("***RECIPE HEADER");
    // A recipe header consists of a name, followed by a colon as a word by itself,
    // followed by a sequence of sub-recipe names.
    //
    // Lines preceding the header that consist only of whitespace are skipped.
    char *w;

    // Skip any blank lines preceding the recipe.
    while(!feof(in) && (w = parse_token(in, errp)) != NULL && *w == '\0')
	free(w);
    if(feof(in))
	return NULL;

    // At this point, w should contain the recipe name.
    RECIPE *rp = calloc(1, sizeof(RECIPE));
    rp->name = w;

    // Check for the colon that is supposed to follow.
    if((w = parse_token(in, errp)) == NULL || strcmp(w, ":")) {
	fprintf(stderr, "%d: Expected ':' after recipe name '%s' but '%s' was seen.\n",
		lineno, rp->name, w != NULL ? w : "(NULL)");
	if(w != NULL)
	    free(w);
	(*errp)++;
	return rp;
    }
    free(w);

    // The remaining words are the names of sub-recipes.
    // Create links for them.
    RECIPE_LINK **last = &rp->this_depends_on;
    while((w = parse_token(in, errp)) != NULL && *w != '\0') {
	RECIPE_LINK *link = calloc(1, sizeof(RECIPE_LINK));
	link->name = w;
	*last = link;
	last = &link->next;
    }
    if(w != NULL)
	free(w);

    return rp;
}

/*
 * Parse a task.
 *
 * Returns the task, or NULL if a blank line is seen.
 */
static TASK *parse_task(FILE *in, int *errp) {
    debug("***TASK");
    TASK *tp = calloc(1, sizeof(TASK));

    // A task consists of a sequence of steps to be run as a pipeline,
    // optionally followed by input and output redirections.
    STEP *sp;
    STEP **lastp = &tp->steps;
    int ends_with_vbar = 0;
    while(!feof(in) && (sp = parse_step(in, errp)) != NULL) {
	// parse_step() stops when EOF, NL, |, <, or > is seen,
	// and it leaves the delimiter token unread.
	ends_with_vbar = 0;
	*lastp = sp;
	lastp = &sp->next;
	// Examine the delimiter that caused parse_step to stop.
	// Check for redirections and pipelines that end with "|".
	char *w;
	while(!feof(in) && (w = parse_token(in, errp)) != NULL) {
	    debug("(step delimiter: '%s')", w);
	    if(*w == '\0') {
		free(w);
		break;
	    } else if(!strcmp(w, "|")) {
		ends_with_vbar = 1;
		free(w);
		break;  // Parse another step.
	    } else if(!strcmp(w, "<") || !strcmp(w, ">")) {
		// Input or output redirection -- get filename.
		char *n = parse_token(in, errp);
		if(n == NULL) {
		    fprintf(stderr, "%d: Missing filename in input or output redirection\n",
			    lineno);
		    free(w);
		    (*errp)++;
		    return tp;
		}
		debug("(redirect '%s')", n);
		char **np = (*w == '<' ? &tp->input_file : &tp->output_file);
		if(*np != NULL) {
		    fprintf(stderr, "%d: Redundant input or output redirection\n", lineno);
		    free(w);
		    free(n);
		    (*errp)++;
		    continue;
		}
		*np = n;
		free(w);
	    } else {
		// Shouldn't happen.
		fprintf(stderr, "%d: Step terminated by unknown delimiter '%s'", lineno, w);
		free(w);
		(*errp)++;
		break;
	    }
	}
	if(!ends_with_vbar) {
	    debug("(end of step)");
	    break;
	}
    }
    if(ends_with_vbar) {
	fprintf(stderr, "%d: Pipeline terminated by '|' -- another step is required\n", lineno);
	(*errp)++;
    }
    if(tp->steps == NULL) {
	free(tp);
	debug("(empty task -- end of recipe)");
	return NULL;
    }
    debug("(end task)");
    return tp;
}

/*
 * Parse a step.
 */
static STEP *parse_step(FILE *in, int *errp) {
    debug("***STEP");
    // A step consists of a sequence of non-delimiter words.
    // Delimiters are "|", "<", and ">" in words by themselves.
    STEP *sp = calloc(1, sizeof(STEP));
    char *w;
    int length = 0;
    int max = 8;
    sp->words = calloc(max, sizeof(char *));
    while((w = parse_token(in, errp)) != NULL && *w != '\0') {
	if(length >= max-1) {
	    max *= 2;
	    sp->words = realloc(sp->words, max * sizeof(char *));
	}
	if(!strcmp(w, "|") || !strcmp(w, "<") || !strcmp(w, ">")) {
	    debug("(push back '%s')", w);
	    peek_token = w;
	    break;
	} else {
	    sp->words[length++] = w;
	}
    }
    if(w != NULL) {
	debug("(push back '%s')", w);
	peek_token = w; 
	if(*w == '\0')
	    lineno--;
    }
    if(length == 0) {
	// No step here
	free(sp->words);
	free(sp);
	return NULL;
    }
    debug("(end step)");
    sp->words[length] = NULL;
    return sp;
}

static int is_delim(int c) {
    return c == '<' || c == '>' || c == '|' || c == ':';
}

/*
 * Parse a token.
 *
 * Initial whitespace (other than newline) is skipped.
 * If a newline is seen, an empty token is returned.
 * If '<', '>', or '|' is seen, a single-character token is returned.
 * If EOF is encountered or an error occurs, NULL is returned.
 * In other cases, characters are read up to the next whitespace character
 * or EOF.  The character delimiting the end of the word is left in
 * the input stream.
 *
 * If '\' (backslash) is seen while reading a token, then any special meaning
 * of the next character is canceled, and that next character (without the
 * backslash) is included in the token.  The backslash character itself is
 * subject to this quoting behavior; thus two backslashes in a row result in
 * a single backslash in the token.
 *
 * The caller is responsible for freeing any non-NULL token returned.
 */
static char *parse_token(FILE *in, int *errp) {
    int c;
    int bs = 0;  // Whether a backslash was just read.

    // Check for a previously read token that was pushed back.
    if(peek_token != NULL) {
	char *w = peek_token;
	peek_token = NULL;
	if(*w == '\0')
	    lineno++;
	return w;
    }

    // Skip initial whitespace, stopping if a newline is encountered.
    while((c = fgetc(in)) != EOF && isspace(c) && c != '\n')
	;
    if(c == EOF) {
	debug("(EOF)");
	return NULL;
    }
    if(c == '\n') {
	debug("(NL)");
	lineno++;
	return strdup("");
    }

    // A word is a sequence of non-whitespace, non-special characters.
    int length = 0;
    int max = 8;
    char *word = calloc(max, sizeof(char));
    do {
	// Ensure space for another character.
	if(length >= max-1) {
	    max *= 2;
	    word = realloc(word, max * sizeof(char));
	}
	// Check for EOF.
	if(c == EOF) {
	    debug("(EOF)");
	    if(bs) {
		word[length++] = '\\';
		word[length] = '\0';
		debug("WORD: %s", word);
		return word;
	    }
	}
	// Check for newline.
	if(c == '\n') {
	    ungetc(c, in);
	    if(bs) 
		word[length++] = '\\';
	    word[length] = '\0';
	    debug("WORD: %s", word);
	    return word;
	}
	// Check for backslash character.
	if(c == '\\') {
	    if(bs) {
		debug("(BS:=0)");
		bs = 0;
	    } else {
		debug("(BS:=1)");
		bs = 1;
		c = fgetc(in);
		continue;
	    }
	}
	// Check for delimiter characters.
	// At the beginning of a token, these result in
	// a single-character token.
	// Elsewhere, they terminate the current token and become
	// the first character of the next token.
	if(!bs && is_delim(c)) {
	    if(length == 0) {
		char *delim = NULL;
		if(c == '<')
		    delim = strdup("<");
		if(c == '>')
		    delim = strdup(">");
		if(c == '|')
		    delim = strdup("|");
		if(c == ':')
		    delim = strdup(":");
		debug("DELIM: '%s'", delim);
		return delim;
	    } else {
		ungetc(c, in);  // Leave it for next time.
		word[length] = '\0';
		debug("WORD: %s", word);
		return word;
	    }
	}
	// Check for space characters.
	// These terminate the current token, but unless there is a
	// preceding backslash they are never added to the token.
	if(isspace(c)) {
	    if(bs) {
		debug("(BS:=0)");
		bs = 0;
		word[length++] = c;
		c = fgetc(in);
	    } else {
		break;
	    }
	}
	// Default case: add character to token.
	word[length++] = c;
	c = fgetc(in);
    } while(c != EOF);
    if(c != EOF && !isspace(c))
	ungetc(c, in);  // Leave it for next time.

    // Finish up.
    word[length] = '\0';
    if(length == 0) {
	// Only whitespace was read.
	debug("(EMPTY)");
	free(word);
	return strdup("");
    }
    debug("WORD: %s", word);
    return word;
}

/*
 * Get the recipe with a given name from a cookbook.
 */

static RECIPE *get_recipe(COOKBOOK *cbp, char *name) {
    /*
     * We are just using inefficient linear search.
     * If we want to do better, we could first create an index for
     * the cookbook, in the form a of a hash table or sorted list of names
     * that could be accessed using binary search.
     */
    for(RECIPE *rp = cbp->recipes; rp != NULL; rp = rp->next) {
	if(!strcmp(rp->name, name))
	    return rp;
    }
    return NULL;
}

/*
 * Traverse the cookbook and fill in the dependency links from recipes
 * to the sub-recipes on which they depend.  For each dependency of a
 * recipe R on a sub-recipe S, also add an inverse dependency from S to R.
 * The inverse dependencies are used to trigger sup-recipes upon completion
 * of the sub-recipes on which they depend.
 */

static int set_dependencies(COOKBOOK *cbp) {
    RECIPE *rp, *sp;
    for(rp = cbp->recipes; rp != NULL; rp = rp->next) {
	debug("set_dependencies: %s", rp->name);
	RECIPE_LINK *rlp;
	for(rlp = rp->this_depends_on; rlp != NULL; rlp = rlp->next) {
	    debug("depends on: %s", rlp->name);
	    sp = get_recipe(cbp, rlp->name);
	    if(sp == NULL) {
		fprintf(stderr, "Recipe %s depends on non-existent sub-recipe %s\n",
			rp->name, rlp->name);
		return 1;
	    }
	    debug("Set dependency: %s -> %s", rp->name, sp->name);
	    rlp->recipe = sp;
	    RECIPE_LINK *rlp1 = calloc(1, sizeof(RECIPE_LINK));
	    rlp1->name = rp->name;
	    rlp1->recipe = rp;
	    rlp1->next = sp->depend_on_this;
	    sp->depend_on_this = rlp1;
	}
    }
    return 0;
}
