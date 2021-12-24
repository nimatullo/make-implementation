#ifndef COOKBOOK_H
#define COOKBOOK_H

#include <stdio.h>

/*
 * A "cookbook" consists of a list of named "recipes".
 */
typedef struct cookbook
{
  struct recipe* recipes; // List of recipes in the cookbook.
  void* state;            // Any additional state info you need to add.
} COOKBOOK;

/*
 * A "recipe" consists of a name, a list of "sub-recipes", and a sequence of
 * "tasks". In order to carry out the recipe, first the sub-recipes must be
 * performed (possibly in parallel), and then the specified tasks must be
 * carried out in sequence.
 *
 * A "recipe_link" links from one recipe to another.  Recipe links are chained
 * together to form a list of sub-recipes on which a given recipe depends, and a
 * list of recipes that have a given recipe as a sub-recipe.
 */
typedef struct recipe_link
{
  char* name;               // Name of the linked recipe.
  struct recipe* recipe;    // The linked recipe.
  struct recipe_link* next; // Next link in the dependency list.
} RECIPE_LINK;

typedef struct recipe
{
  char* name;                   // Name of the recipe.
  RECIPE_LINK* this_depends_on; // List of recipes on which this recipe depends.
  RECIPE_LINK* depend_on_this;  // List of recipes that depend on this recipe.
  struct task* tasks;           // Tasks to perform to complete the recipe.
  struct recipe* next;          // Next recipe in the cookbook.
  void* state;                  // Any additional state info you need to add.
} RECIPE;

/*
 * A "task" consists of a sequence of "steps" to be performed by processes
 * in a pipeline, with the standard output of each command piped as the
 * standard input of the next command in the pipeline.
 * The tasks in a recipe are chained together into a list.
 * Each task may optionally include input or output redirections.
 * An input redirection is the name of a file (which must already exist) to
 * be used as the standard input to the first command in the pipeline.
 * An output redirection is the name of a file (which is created if it does
 * not exist and truncated if it does) to be used as the standard output of
 * the last command in the pipeline.
 */
typedef struct task
{
  struct step* steps; // Steps to perform to complete the task.
  char* input_file;   // Name of file for input redirection, or NULL.
  char* output_file;  // Name of file for output redirection, or NULL.
  struct task* next;  // Next task in the recipe.
} TASK;

/*
 * A "step" consists of a NULL-terminated array of "words".  The first word
 * is interpreted as the name of a program to be run; the subsequent words are
 * its arguments.  The steps in a task are chained into a list.
 */
typedef struct step
{
  char** words;      // NULL-terminated list of words.
  struct step* next; // Next step in the task.
} STEP;

/*
 * Function for parsing a cookbook from an input stream.
 * This always returns a non-NULL cookbook, however, if errors are detected
 * during parsing it might be incomplete and should not be used.  In this
 * case the variable pointed at by errp is set to a nonzero value.
 * If parsing was successful, then this variable is set to zero.
 *
 * The caller is responsible for freeing the object returned by this function
 * and for managing the input stream and closing it (if appropriate).
 */
COOKBOOK*
parse_cookbook(FILE* in, int* errp);

/*
 * Function for outputting a cookbook to an output stream, in a format from
 * which it can be parsed again.
 *
 * The caller is reponsible for managing the output stream, including checking
 * it for errors and closing it (if appropriate).
 */
void
unparse_cookbook(COOKBOOK* cbp, FILE* out);

#endif
