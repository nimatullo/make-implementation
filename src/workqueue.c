#include "workqueue.h"
#include "recipe.h"
#include <string.h>

void
q_enqueue(RECIPE_LINK* recipe)
{
  STATE* state = malloc(sizeof(STATE));
  state->status = enqueue;
  recipe->recipe->state = state;

  QUEUE* cur = q;
  if (cur == NULL) {
    cur = malloc(sizeof(QUEUE)); // MAKE SURE TO FREE THIS
    cur->recipe = recipe;
    q = cur;
  } else {
    QUEUE* node = calloc(1, sizeof(QUEUE));
    node->recipe = recipe;
    while (cur != NULL) {
      if (strcmp(cur->recipe->name, recipe->name) == 0) {
        free(node);
        return; // If the recipe exists already, don't add again
      }
      if (cur->next == NULL)
        break;
      cur = cur->next;
    }
    cur->next = node;
  }
}

void
q_dequeue()
{
  QUEUE* head = q;
  STATE* state = head->recipe->recipe->state;
  state->status = finished;

  // queue_recipes_from_depend_on_this_list(head->recipe);

  q = head->next;
}

void
print_queue()
{
  QUEUE* head = q;
  while (head != NULL) {
    debug("%s", head->recipe->name);
    head = head->next;
  }
  debug("--");
}