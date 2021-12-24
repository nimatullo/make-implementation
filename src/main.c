#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cookbook.h"
#include "pipeline.h"
#include "recipe.h"
#include "workqueue.h"

int
main(int argc, char* argv[])
{
  int opt;
  char* path = "./rsrc/cookbook.ckb";
  MAX_COOKS = 1;
  while ((opt = getopt(argc, argv, ":f:c:")) != -1) {
    switch (opt) {
      case 'f':
        path = optarg;
        break;
      case 'c':
        MAX_COOKS = atoi(optarg);
        break;
      case ':':
        debug("Option needs value");
        exit(EXIT_FAILURE);
      case '?':
        debug("Unkown option %c. Ignoring...", optopt);
        break;
    }
  }
  debug("Path %s", path);
  debug("Cooks %d", MAX_COOKS);
  COOKBOOK* cbp;
  int err = 0;
  FILE* in;

  if ((in = fopen(path, "r")) == NULL) {
    fprintf(stderr, "Can't open cookbook '%s': %s\n", path, strerror(errno));
    exit(1);
  }
  cbp = parse_cookbook(in, &err);
  main_recipe = cbp->recipes;
  char *custom_main_recipe = argv[optind];
  debug("custom main rec %s", custom_main_recipe);
  if (custom_main_recipe != NULL) {
    set_main_recipe(custom_main_recipe);
    debug("Main recipe provided %s", main_recipe->name);
  }
  if (err) {
    printf("%i\n", err);
    fprintf(stderr, "Error parsing cookbook '%s'\n", path);
    exit(1);
  }

  get_all_leaves(main_recipe->this_depends_on);
  process_queue();

  exit(EXIT_SUCCESS);
}
