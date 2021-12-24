#ifndef WORKQUEUE_H
#define WORKQUEUE_H
#include "cookbook.h"
#include "debug.h"
#include <signal.h>
#include <stdlib.h>

/**
 * @brief Number of max cooks. Default is 1.
 *
 */
int MAX_COOKS;

volatile sig_atomic_t ACTIVE_COOKS;

/**
 * @brief Work queue. Queues the recipe to be completed
 * at the head.
 *
 */
typedef struct workqueue
{
  RECIPE_LINK* recipe;
  struct workqueue* next;
} QUEUE;

/**
 * @brief Head queue node
 *
 */
QUEUE* q;

/**
 * @brief Add a recipe to the head of the queue.
 *
 * @param recipe
 */
void
q_enqueue(RECIPE_LINK* recipe);

/**
 * @brief Sets the recipe at the head of the queue to "finished"
 * and then tries to add all the recipes that depend on the recipe
 * at the head of the queue to the queue. It then removes the recipe
 * from the queue.
 *
 */
void
q_dequeue();

void
print_queue();

#endif