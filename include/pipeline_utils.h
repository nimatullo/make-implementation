#ifndef PIPELINEUTILS_H
#define PIPELINEUTILS_H

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "debug.h"

/**
 * @brief Duplicates the fd to STDIN_FILENO.
 * Will exit with 1 if dup2 fails.
 *
 * @param fd
 */
int
redirect_input(int fd);

/**
 * @brief Duplicates the fd to STDOUT_FILENO.
 * Will exit with 1 if dup2 fails.
 *
 * @param fd
 */
int
redirect_output(int fd);

/**
 * @brief Opens a file with read only access.
 *
 * If the file doesn't exists or opening fails,
 * the program exits with 1.
 *
 * @param path
 * @return int
 */
int
open_for_reading(char* path);

/**
 * @brief Opens a file with read and write access.
 * If the file doesn't exist, one that will be created.
 * Sets the chmod permissions to 0666 which sets permissions so that:
 * (U)ser / owner can read, can write and can't execute.
 * (G)roup can read, can write and can't execute.
 * (O)thers can read, can write and can't execute.

 *
 * If the path contains a directory that doesn't exists
 * or opening the file fails, the program exits with 1.
 *
 * @param path
 * @return int
 */
int
open_for_writing(char* path);

/**
 * @brief Initializes the 2D pipes using the pipe command.
 *
 * If any of the pipes fail to initialize, the program exits with 1.
 *
 * @param pipefd
 * @param number_of_steps
 */
int
initialize_pipes(int pipefd[][2], int number_of_steps);

#endif