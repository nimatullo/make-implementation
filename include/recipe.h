#ifndef RECIPE_H
#define RECIPE_H

#include "cookbook.h"
#include <string.h>
#include <sys/types.h>

RECIPE* main_recipe;

/**
 * @brief status of a recipe in a queue
 *
 */
typedef enum
{
  enqueue,
  started,
  finished,
  failed
} STATUS;

/**
 * @brief The state of a recipe.
 * Currently just contains the status.
 *
 */
typedef struct
{
  STATUS status;
  pid_t worker_pid;
} STATE;

/**
 * @brief Gets all the nodes without any dependencies (leaf nodes)
 *
 * @param start Root recipe
 */
void
get_all_leaves(RECIPE_LINK* start);

/**
 * @brief Goes through every recipe in the RECIPE_LINK's depends on list
 * and checks if they are completed.
 *
 * @param recipe
 * @return int
 */
int
is_dependencies_completed(RECIPE_LINK* recipe);

/**
 * @brief Goes through all of the recipes in the
 * `recipe->recipe->depend_on_this` list and queues them to the work queue if
 * all their dependencies have been completed.
 *
 * @param recipe
 */
void
queue_recipes_from_depend_on_this_list(RECIPE_LINK* recipe);

/**
 * @brief Counts the number of steps
 *
 * @param steps
 * @return int
 */
int
count_number_of_steps(STEP* steps);

RECIPE*
find_by_pid(pid_t pid);

void
set_main_recipe(char* recipe_name);

#endif