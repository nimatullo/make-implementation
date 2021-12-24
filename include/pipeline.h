#ifndef PIPELINE_H
#define PIPELINE_H

#include "debug.h"
#include "pipeline_utils.h"
#include "recipe.h"
#include "workqueue.h"

#include <string.h>

#define READ_END 0
#define WRITE_END 1

#define CLOSE_W_END(fd) close(fd[WRITE_END])
#define CLOSE_R_END(fd) close(fd[READ_END])
#define CLOSE_BOTH_ENDS(fd)                                                    \
  CLOSE_W_END(fd);                                                             \
  CLOSE_R_END(fd);

/**
 * @brief "main processing loop" where all the queued up recipes
 * are handled.
 *
 * It takes the head of the queue and performs all the necessary tasks
 * for that recipe. After all the tasks for that recipe are completed, it will
 * dequeue that recipe.
 *
 * It moves on to the next head and continues until there are no more remaining
 * recipes left in the queue.
 *
 */
void
process_queue();

/**
 * @brief Establishes a pipe between the processes and executes
 * the tasks associates with a step in parallel.
 *
 * Parent writes the start step into the pipe and the children will
 * read from that pipe, perform the steps, and write the next step back
 * into the pipe.
 *
 */
int
process_steps(TASK* task);

/**
 * @brief First tries to run the command using execvp in the /util/ directory.
 * If it fails, it will try running it in the system directory.
 * If that fails as well, the program exits with 1.
 *
 * @param util_path Path of the /util/ directory
 * @param steps The actual command that needs to run
 * @return int
 */
void
execute_command(char* util_path, char** steps);

void
completed_recipe_handler(int signo);

#endif