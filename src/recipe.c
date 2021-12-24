#include "recipe.h"
#include "debug.h"
#include "workqueue.h"

void
get_all_leaves(RECIPE_LINK* depend_list)
{
  while (depend_list != NULL) {
    if (depend_list->recipe->this_depends_on == NULL) {
      q_enqueue(depend_list);
    }
    get_all_leaves(depend_list->recipe->this_depends_on);
    depend_list = depend_list->next;
  }
}

int
is_dependencies_completed(RECIPE_LINK* recipe)
{
  RECIPE_LINK* dependencies = recipe->recipe->this_depends_on;
  while (dependencies != NULL) {
    STATE* state = dependencies->recipe->state;
    if (state == NULL || state->status != finished)
      return 0;
    dependencies = dependencies->next;
  }
  return 1;
}

void
queue_recipes_from_depend_on_this_list(RECIPE_LINK* recipe)
{
  RECIPE_LINK* dependencies = recipe->recipe->depend_on_this;
  while (dependencies != NULL) {
    if (is_dependencies_completed(dependencies)) {
      q_enqueue(dependencies);
    }
    dependencies = dependencies->next;
  }
}

int
count_number_of_steps(STEP* steps)
{
  int count = 0;
  while (steps != NULL) {
    count++;
    steps = steps->next;
  }
  return count;
}

RECIPE*
find_by_pid(pid_t pid)
{
  RECIPE* rec = main_recipe;

  while (rec != NULL) {
    if (rec->state != NULL && ((STATE*)rec->state)->worker_pid == pid) {
      return rec;
    }
    rec = rec->next;
  }
  return NULL;
}

void
set_main_recipe(char* recipe_name)
{
  RECIPE* current_recipe = main_recipe;
  while (current_recipe != NULL) {
    if (strcmp(recipe_name, current_recipe->name) == 0) {
      main_recipe = current_recipe;
      return;
    }
    current_recipe = current_recipe->next;
  }
  error("Recipe %s not found in the .ckb file!", recipe_name);
  exit(EXIT_FAILURE);
}