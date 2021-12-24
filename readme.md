# Makefile implementation

Custom implementation of the `make` unix command. 

Spawns multiple processes for each command so the entire program runs concurrently.

## Usage

Create a `.ckb` file with a yaml like structure. The first entry is the main program and everything after the colon is that command's dependencies. All the dependencies of a command need to be completed before the steps of that command start. There are examples of `.ckb` files in the `/rsrc` folder.

The program accepts a command line as follows:
```bash
cook [-f cookbook] [-c max_cooks] [main_recipe_name]
```