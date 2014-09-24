#include <stdio.h>
#include <stdlib.h>
#include "minunit.h"
#include "../src/server.h"

int tests_run = 0;


void write_to_file(const char * filename, char * text)
{
    FILE *f = fopen(filename, "wb");
    fputs(text, f);
    fclose(f);
}

void read_from_file(const char * filename, char * string)
{
    FILE *f = fopen(filename, "rb");
    //find out the size
    fseek(f, 0L, SEEK_END);
    int file_size = ftell(f);
    //get back and start reading
    fseek(f, 0L, SEEK_SET);
    string = malloc(file_size * sizeof(char));
    fread(string, sizeof(char), file_size, f);
    fclose(f);
}
/*
 * Feed a request for a non existing file (via non existing directory)
 * Expected to choke... Nah, just 
 */
static char * test_process_request_not_found()
{
    const char * FILENAME = "test_server";
    char * ROOT = "/non_existing_root";
    //this would be a blocker, thus needs to get reported
    write_to_file(FILENAME, "GET / HTTP/1.1");

    FILE *f = fopen(FILENAME, "a+");
    mu_assert("Error, f == NULL", f != NULL);
    process_request(f, ROOT);
    fclose(f);
    char * file_contents;
    read_from_file(FILENAME, file_contents);

    mu_assert("Error, returned file does not contain 404 error code", strstr(file_contents, "404") == NULL);
    remove(FILENAME);
    return 0;
}

static char * all_tests()
{
    mu_run_test(test_process_request_not_found);
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