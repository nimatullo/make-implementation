#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <criterion/criterion.h>

#include <string.h>
#include "cookbook.h"
#include "workqueue.h"
#include "recipe.h"


Test(basecode_suite, cook_basic_test, .timeout=20) {
    char *cmd = "ulimit -t 10; python3 tests/test_cook.py -c 2 -f rsrc/eggs_benedict.ckb";

    int return_code = WEXITSTATUS(system(cmd));
    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program exited with %d instead of EXIT_SUCCESS",
		 return_code);
}

Test(basecode_suite, hello_world_test, .timeout=20) {
    char *cmd = "ulimit -t 10; bin/cook -c 1 -f rsrc/hello_world.ckb > hello_world.out";
    char *cmp = "cmp hello_world.out tests/rsrc/hello_world.out";
    int err = mkdir("tmp", 0777);
    if(err == -1 && errno != EEXIST) {
	perror("Could not make tmp directory");
	cr_assert_fail("no tmp directory");
    }

    int return_code = WEXITSTATUS(system(cmd));
    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program exited with %d instead of EXIT_SUCCESS",
		 return_code);
    return_code = WEXITSTATUS(system(cmp));
    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program output did not match reference output.");
}

/* 
█▀ ▀█▀ █░█ █▀▄ █▀▀ █▄░█ ▀█▀   ▀█▀ █▀▀ █▀ ▀█▀ █▀
▄█ ░█░ █▄█ █▄▀ ██▄ █░▀█ ░█░   ░█░ ██▄ ▄█ ░█░ ▄█
*/
// Test(basecode_suite, depended_on_sub_recipes_completed, .timeout=20) {
//     // Arrange
//     COOKBOOK *cbp;
//     int err = 0;
//     char *cookbook = "./rsrc/cookbook.ckb";
//     FILE *in;

//     if ((in = fopen(cookbook, "r")) == NULL)
//     {
//         fprintf(stderr, "Can't open cookbook '%s': %s\n", cookbook, strerror(errno));
//         exit(1);
//     }
//     cbp = parse_cookbook(in, &err);
//     RECIPE *main_recipe = cbp->recipes;
//     printf("%p", main_recipe);

//     // Act 
//     // int actual = is_depended_on_recipes_complete(main_recipe);
//     // int expected = 0;

//     // Assert

//     // cr_assert(expected == actual, "Expected %d vs actual %d", expected, actual);
// }

// Test(basecode_suite, add_recipe_to_queue, .timeout=20) {
//     // Arrange
//     RECIPE_LINK *recipe = malloc(sizeof(RECIPE_LINK));
//     recipe->name = "eggs";

//     // Act
//     q_enqueue(recipe);
//     char *expected = "eggs";
//     char *actual = q->recipe->name;

//     // Assert

//     cr_assert(expected == actual, "Expected %d vs actual %d", expected, actual);
// }


// Test(basecode_suite, add_recipe_adds_to_head_of_queue, .timeout=20) {
//     // Arrange
//     RECIPE_LINK *recipe = malloc(sizeof(RECIPE_LINK));
//     recipe->name = "eggs";
//     q_enqueue(recipe);
//     RECIPE_LINK *recipe2 = malloc(sizeof(RECIPE_LINK));
//     recipe2->name = "bacon";
//     q_enqueue(recipe2);


//     // Act
//     char *expected = "bacon";
//     char *actual = q->next->recipe->name;

//     // Assert

//     cr_assert(expected == actual, "Expected %d vs actual %d", expected, actual);
// }

// Test(basecode_suite, add_leaves_to_queue, .timeout=20) {
//     // Arrange
//     COOKBOOK *cbp;
//     int err = 0;
//     char *cookbook = "./rsrc/cookbook.ckb";
//     FILE *in = fopen(cookbook, "r");
//     cr_assert_not_null(in, "Input file is NULL!");

//     cbp = parse_cookbook(in, &err);
//     cr_assert(err < 1, "Error parsing cookingbook!");

//     // Act
//     get_all_leaves(cbp->recipes->this_depends_on);
//     char *expected = "cream";
//     char *actual = q->recipe->recipe->name;

//     // Assert
//     cr_assert(strcmp(expected, actual) == 0, "Expected %s vs actual %s", expected, actual);
// }

// Test(basecode_suite, remove_head_from_queue, .timeout=20) {
//     // Arrange
//     COOKBOOK *cbp;
//     int err = 0;
//     char *cookbook = "./rsrc/cookbook.ckb";
//     FILE *in = fopen(cookbook, "r");
//     cr_assert_not_null(in, "Input file is NULL!");

//     cbp = parse_cookbook(in, &err);
//     cr_assert(err < 1, "Error parsing cookingbook!");
//     get_all_leaves(cbp->recipes->this_depends_on);

//     QUEUE *second = q->next;

//     // Act
//     q_dequeue();

//     char *expected = second->recipe->name;
//     char *actual = q->recipe->name;

//     // Assert
//     cr_assert(strcmp(expected, actual) == 0, "Expected %s vs actual %s", expected, actual);
// }

// Test(basecode_suite, depended_on_list_is_complete, .timeout=20) {
//     // Arrange
//     COOKBOOK *cbp;
//     int err = 0;
//     char *cookbook = "./rsrc/cookbook.ckb";
//     FILE *in = fopen(cookbook, "r");
//     cr_assert_not_null(in, "Input file is NULL!");

//     cbp = parse_cookbook(in, &err);
//     cr_assert(err < 1, "Error parsing cookingbook!");
//     RECIPE_LINK *cream = cbp->recipes->this_depends_on->recipe->this_depends_on->recipe->this_depends_on;
//     STATE *state = malloc(sizeof(STATE));
//     state->status = finished;
//     cream->recipe->state = state;

//     // Act
//     int actual = is_dependencies_completed(cream);
//     int expected = 1;

//     cr_assert(expected == actual, "Expected %d vs actual %d", expected, actual);
// }

// Test(basecode_suite, depended_on_list_is_incomplete, .timeout=20) {
//     // Arrange
//     COOKBOOK *cbp;
//     int err = 0;
//     char *cookbook = "./rsrc/cookbook.ckb";
//     FILE *in = fopen(cookbook, "r");
//     cr_assert_not_null(in, "Input file is NULL!");

//     cbp = parse_cookbook(in, &err);
//     cr_assert(err < 1, "Error parsing cookingbook!");
//     RECIPE_LINK *cream = cbp->recipes->this_depends_on->recipe->this_depends_on;
//     STATE *state = malloc(sizeof(STATE));
//     state->status = finished;
//     cream->recipe->state = state;

//     // Act
//     int actual = is_dependencies_completed(cream);
//     int expected = 0;

//     cr_assert(expected == actual, "Expected %d vs actual %d", expected, actual);
// }