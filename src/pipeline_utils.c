#include "pipeline_utils.h"

int
redirect_input(int fd)
{
  if (fd > -1) {
    if (dup2(fd, STDIN_FILENO) == -1) {
      error("Error duping!");
      return 1;
    }
  }
  return 0;
}

int
redirect_output(int fd)
{
  if (fd > -1) {
    if (dup2(fd, STDOUT_FILENO) == -1) {
      error("Error duping");
      return 1;
    }
  }
  return 0;
}

int
write_dup(int output_file_fd, int file_descriptor)
{
  if (output_file_fd > -1) {
    if (dup2(output_file_fd, STDOUT_FILENO) == -1) {
      error("Error duping");
      return 1;
    }
  } else {
    if (dup2(file_descriptor, STDOUT_FILENO) == -1) {
      error("Error duping");
      return 1;
    }
  }
  return 0;
}

int
open_for_reading(char* path)
{
  int in = open(path, O_RDONLY);
  if (path == NULL)
    return 0;
  else if (in < 0)
    return -1;
  else
    return in;
}

int
open_for_writing(char* path)
{
  int out = open(path, O_CREAT | O_RDWR, 0666);
  if (path == NULL)
    return 0;
  else if (out < 0)
    return -1;
  else
    return out;
}

int
initialize_pipes(int pipefd[][2], int number_of_steps)
{
  for (int i = 0; i < number_of_steps - 1; i++) {
    if (pipe(pipefd[i]) < 0) {
      error("Error creating pipe!");
      return 1;
    }
  }
  return 0;
}
