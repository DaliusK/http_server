#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include "minunit.h"
#include "../src/logger.h"

int tests_run = 0;

int is_in_file(const char * filename, char * substr)
{
    FILE *f = fopen(filename, "rb");
    //find out the size
    fseek(f, 0L, SEEK_END);
    int file_size = ftell(f);
    //get back and start reading
    fseek(f, 0L, SEEK_SET);
    char *str = malloc(file_size * sizeof(char));
    fread(str, sizeof(char), file_size, f);
    fclose(f);
    if (strstr(str, substr) == NULL)
    {
        free (str);
        return 0;
    }
    else
    {
        free (str);
        return 1;
    }
}

/*
 * Test simple output
 */
static char * test_logging_output()
{
    printf("Test logging: output\n");
    /* intercept stdout */
    /* Solution by: http://stackoverflow.com/a/17071777/552214 */
    FILE *fp;
    char * tmp_filename;
    int stdout_bak;//stdout fd backup

    tmp_filename = tmpnam(NULL);
    stdout_bak = dup(fileno(stdout));
    fp = fopen(tmp_filename, "w");

    dup2(fileno(fp), fileno(stdout));
    log_info("Testing info");
    log_error("Testing error");
    log_debug("Testing debug");
    fflush(stdout);
    fclose(fp);

    dup2(stdout_bak, fileno(stdout));

    mu_assert("Info tag not shown in log stdout", is_in_file(tmp_filename, "info"));
    mu_assert("Error tag not shown in log stdout", is_in_file(tmp_filename, "error"));
    mu_assert("Debug tag not shown in log stdout", is_in_file(tmp_filename, "debug"));
    mu_assert("Info log message not in stdout", is_in_file(tmp_filename, "Testing info"));
    mu_assert("Error log message not in stdout", is_in_file(tmp_filename, "Testing error"));
    mu_assert("Debug log message not in stdout", is_in_file(tmp_filename, "Testing debug"));

    return 0;
}

static char * all_tests()
{
    printf("===== TEST LOGGER =====\n");
    mu_run_test(test_logging_output);
    return 0;
}

int main(int argc, char **argv)
{
    char *result = all_tests();
    if (result != 0)
    {
        printf("%s\n", result);
    }
    else
    {
        printf("ALL TESTS PASSED\n");
    }

    printf("Tests run: %d\n", tests_run);

    return result != 0;
}