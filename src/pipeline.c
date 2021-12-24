#include "pipeline.h"

RECIPE* rec_link;
RECIPE_LINK* link_to_add;
volatile sig_atomic_t flag;

void
completed_recipe_handler(int signo)
{
  int status;
  pid_t child_pid;

  while ((child_pid = waitpid(-1, &status, WNOHANG)) != 0) {
    if (child_pid == -1) {
      break;
    }
    ACTIVE_COOKS--;
    rec_link = find_by_pid(child_pid);
    link_to_add->recipe = rec_link;
    if (WIFEXITED(status) && (WEXITSTATUS(status) == EXIT_FAILURE)) {
      error("Recipe failed!");
      ((STATE*)rec_link->state)->status = failed;
      while (wait(NULL) > 0)
        ;
      _exit(EXIT_FAILURE);
    } else {
      debug("Recipe %s success!", rec_link->name);
      ((STATE*)rec_link->state)->status = finished;
      queue_recipes_from_depend_on_this_list(link_to_add);
    }
  }
}

void
process_queue()
{
  link_to_add = malloc(sizeof(RECIPE_LINK));
  pid_t pid;
  ACTIVE_COOKS = 0;
  sigset_t sigchild_blocked_mask, inverse_sigchild_blocked_mask;
  TASK* recipe_tasks;

  sigemptyset(&sigchild_blocked_mask);
  sigaddset(&sigchild_blocked_mask, SIGCHLD);

  sigfillset(&inverse_sigchild_blocked_mask);
  sigdelset(&inverse_sigchild_blocked_mask, SIGCHLD);

  struct sigaction act;
  act.sa_handler = &completed_recipe_handler;
  act.sa_flags = SA_RESTART;
  sigemptyset(&act.sa_mask);
  sigaddset(&act.sa_mask, SIGCHLD);
  sigaction(SIGCHLD, &act, NULL);

  sigprocmask(SIG_BLOCK, &sigchild_blocked_mask, NULL);
  while ((q != NULL || ACTIVE_COOKS > 0)) {
    if ((ACTIVE_COOKS == MAX_COOKS) || q == NULL) {
      sigsuspend(&inverse_sigchild_blocked_mask);
    } else {
      if ((pid = fork()) == -1) {
        error("Error forking child.");
        _exit(1);
      } else if (pid == 0) {
        // CHILD PROCESS
        recipe_tasks = q->recipe->recipe->tasks;
        while (recipe_tasks != NULL) {
          if (process_steps(recipe_tasks))
            _exit(EXIT_FAILURE);
          recipe_tasks = recipe_tasks->next;
        }
        _exit(EXIT_SUCCESS);
      }
      ACTIVE_COOKS++;
      // PARENT PROCESS
      ((STATE*)q->recipe->recipe->state)->status = started;
      ((STATE*)q->recipe->recipe->state)->worker_pid = pid;
      QUEUE *free_this = q;
      q = q->next;
      free(free_this);
    }
  }
  free(link_to_add);
}

int
process_steps(TASK* task)
{
  STEP* main_step = task->steps;
  /*
   * fd[0] - used to read from pipe
   * fd[1] - used to write to pipe
   */
  int in = -1, out = -1;
  int NUMBER_OF_STEPS = count_number_of_steps(main_step);
  int LAST_CHILD_PROCESS = NUMBER_OF_STEPS - 1;
  int pipefd[NUMBER_OF_STEPS - 1][2];
  pid_t pid;
  char* util_directory = malloc(8);
  strcpy(util_directory, "./util/");

  if (initialize_pipes(pipefd, NUMBER_OF_STEPS)) {
    free(util_directory);
    return 1;
  }

  if ((in = open_for_reading(task->input_file)) == -1) {
    free(util_directory);
    return 1;
  }
  else if (in == 0)
    in = -1;

  if ((out = open_for_writing(task->output_file)) == -1) {
    free(util_directory);
    return 1;
  }
  else if (out == 0)
    out = -1;

  for (int i = 0; i < NUMBER_OF_STEPS; i++) {
    switch ((pid = fork())) {
      case -1:
        error("Forking failed!");
        free(util_directory);
        return 1;
      case 0:               // Child Process
        if (i == 0 && in) { // If first child and input file is provided
          if (redirect_input(in)) {
            free(util_directory);
            return 1;
          }
        } else {
          if (redirect_input(pipefd[i - 1][READ_END])) {
            free(util_directory);
            return 1;
          }
        }
        if (i != LAST_CHILD_PROCESS && NUMBER_OF_STEPS > 1) {
          if (redirect_output(pipefd[i][WRITE_END])) {
            free(util_directory);
            return 1;
          }
        } else if (i == LAST_CHILD_PROCESS && out) {
          if (redirect_output(out)) {
            free(util_directory);
            return 0;
          }
        }
        for (int j = 0; j < NUMBER_OF_STEPS - 1; j++) {
          CLOSE_BOTH_ENDS(pipefd[j]);
        }
        execute_command(util_directory, main_step->words);
    }
    main_step = main_step->next;
  }
  for (int i = 0; i < NUMBER_OF_STEPS; i++) {
    if (i < NUMBER_OF_STEPS - 1)
      CLOSE_BOTH_ENDS(pipefd[i]);
    wait(NULL);
  }
  free(util_directory);
  close(in);
  close(out);
  return 0;
}

void
execute_command(char* util_path, char** steps)
{
  char* command = strcat(util_path, *steps);
  execvp(command, steps);
  error("Error executing %s", command);
  debug("Attempting to exectute %s on system path", *steps);
  execvp(*steps, steps);
  error("Error executing %s", *steps);
  _exit(1);
}
