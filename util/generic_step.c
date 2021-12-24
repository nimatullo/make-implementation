#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

/*
 * Test program to be used as a "generic step" in a recipe task.
 * When invoked, it announces itself using the name by which it was invoked,
 * then delays for a randomly chosen interval.  If invoked with arguments,
 * and the first argument is '-', then it copies stdin to stdout until EOF
 * is seen, otherwise it does not do this.  Finally, then announces its
 * termination and exits.
 * 
 * -m message:
 *      will print out the message passed in as an argument
 * -d:
 *      will disable the random delay so it is just a delay of 0 seconds
 */

/*
 * Set the following to limit the size of the timestamp that is output
 * and make it more readable.  Previously, this was necessary to make the
 * output fit in a fixed-size buffer, but this has now been fixed, so it
 * should be just a convenience.
 */
#define BASE_SECONDS (0)
int get_ms(struct timespec *);

int main(int argc, char *argv[]) {
    int c;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    srandom((unsigned int)ts.tv_nsec);
    int delay = random() % 10;
    char * message = 0;

    for(int i = 0; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch(argv[i][1]) {
                case 'm':
                    message = argv[i + 1];
                    break;
                case 'd':
                    delay = 0;
            }
        }
    }
    
    char *buffer = NULL;
    size_t size;
    FILE *out = open_memstream(&buffer, &size);
    int ms = get_ms(&ts);
    fprintf(out, "START\t[%ld.%03d,%6d,%2d]",
	    ts.tv_sec-BASE_SECONDS, ms, getpid(), delay);
    for(int i = 0; i < argc; i++)
	fprintf(out, " %s", argv[i]);
    fprintf(out, "\n");
    fclose(out);
    write(STDERR_FILENO, buffer, size);
    free(buffer);
    if (message != NULL) {
        printf("%s\n", message);
    }
    if(argc > 1 && !strcmp(argv[1], "-")) {
        while((c = getchar()) != EOF)
            putchar(c);
    }
    usleep(delay * 100000);
    clock_gettime(CLOCK_REALTIME, &ts);
    ms = get_ms(&ts);
    fprintf(stderr, "END\t[%ld.%03d,%6d,%2d]\n",
	    ts.tv_sec-BASE_SECONDS, ms, getpid(), delay);
}

int get_ms(struct timespec * ts){
    int ms = (int) (ts->tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
    if (ms > 999) {
        ts->tv_sec++;
        ms = 0;
    }
    return ms;
}
