#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "minunit.h"
#include "../src/server.h"

int tests_run = 0;

void write_to_file(const char *filename, char *text) {
  FILE *f = fopen(filename, "wb");
  fputs(text, f);
  fclose(f);
}

int is_in_file(const char *filename, char *substr) {
  FILE *f = fopen(filename, "rb");
  // find out the size
  fseek(f, 0L, SEEK_END);
  int file_size = ftell(f);
  // get back and start reading
  fseek(f, 0L, SEEK_SET);
  char *str = malloc(file_size * sizeof(char));
  fread(str, sizeof(char), file_size, f);
  fclose(f);
  if (strstr(str, substr) == NULL) {
    free(str);
    return 0;
  } else {
    free(str);
    return 1;
  }
}

/*
 * Feed a request for a non existing file (via non existing directory)
 * Expected to choke... Nah, just return 404
 */
static char *test_process_request_not_found() {
  printf("Test process request: File not found\n");
  const char *FILENAME = "test_server_not_found";
  char *ROOT = "non_existing_root";
  // this would be a blocker, thus needs to get reported
  write_to_file(FILENAME, "GET / HTTP/1.1");

  FILE *f = fopen(FILENAME, "a+");
  mu_assert("Error, f == NULL", f != NULL);
  process_request(f, ROOT);
  fclose(f);

  mu_assert("Error, returned file does not contain 404 error code",
            is_in_file(FILENAME, "404") == 1);
  remove(FILENAME);
  printf("\n");
  return 0;
}

/*
 * Feed a request with a bad method, which is most likely unsupported
 * Expected to return 501
 */
static char *test_process_request_bad_method() {
  printf("Test process request: Bad method\n");
  const char *FILENAME = "test_server_bad_method";

  char *ROOT = "not_important_dir";

  int root_created = 0;
  struct stat *st = {0};

  if (stat(ROOT, st) == -1) {
    mkdir(ROOT, 0700);
    root_created = 1;
  }

  write_to_file(FILENAME, "BAD_METHOD / HTTP/1.1");
  FILE *f = fopen(FILENAME, "a+");

  mu_assert("Error, f == NULL", f != NULL);

  process_request(f, ROOT);
  fclose(f);

  mu_assert("Error, returned file does not contain 501 error code",
            is_in_file(FILENAME, "501") == 1);

  // sanitize after test
  if (root_created == 1)
    rmdir(ROOT);

  remove(FILENAME);
  printf("\n");
  return 0;
}

/*
 * Tests how request is parsed
 * 1. Empty
 * 2. Only method
 * 3. Method and root
 */
static char *test_process_request_invalid_request() {
  printf("Test process request: invalid request\n");
  const char *FILENAME = "test_server_invalid_request";
  char *ROOT = "invalid_request";

  int root_created = 0;
  struct stat *st = {0};

  if (stat(ROOT, st) == -1) {
    mkdir(ROOT, 0700);
    root_created = 1;
  }

  // simply empty
  write_to_file(FILENAME, "");

  FILE *f = fopen(FILENAME, "a+");
  mu_assert("Error, f == NULL", f != NULL);
  int returncode = process_request(f, ROOT);
  fclose(f);
  remove(FILENAME);
  mu_assert("Error, process_request should've returned -1", returncode == -1);

  // only method
  write_to_file(FILENAME, "GET");

  f = fopen(FILENAME, "a+");
  mu_assert("Error, f == NULL", f != NULL);
  returncode = process_request(f, ROOT);
  fclose(f);
  remove(FILENAME);
  mu_assert("Error, process_request should've returned -1", returncode == -1);

  // method and root
  write_to_file(FILENAME, "GET /");

  f = fopen(FILENAME, "a+");
  mu_assert("Error, f == NULL", f != NULL);
  returncode = process_request(f, ROOT);
  fclose(f);
  mu_assert("Error, process_request should've returned -1", returncode == -1);

  // sanitize after test
  if (root_created == 1)
    rmdir(ROOT);

  remove(FILENAME);
  printf("\n");
  return 0;
}

static char *test_process_request_send_file() {
  printf("Test process request: Send file\n");
  const char *FILENAME = "test_server_send_file";
  char *ROOT = "test_root_dir";
  const char *TESTFILENAME = "test_root_dir/test_file";

  int root_created = 0;
  struct stat *st = {0};

  if (stat(ROOT, st) == -1) {
    mkdir(ROOT, 0700);
    root_created = 1;
  }
  write_to_file(TESTFILENAME, "send file test");
  write_to_file(FILENAME, "GET /test_file HTTP/1.1");
  FILE *f = fopen(FILENAME, "a+");

  mu_assert("Error, f == NULL", f != NULL);

  process_request(f, ROOT);
  fclose(f);

  mu_assert("Error, process_request should've returned file",
            is_in_file(FILENAME, "send file test") == 1);

  // sanitize after test
  remove(TESTFILENAME);

  if (root_created == 1)
    rmdir(ROOT);

  remove(FILENAME);
  printf("\n");
  return 0;
}

static char *test_process_request_list_dir() {
  printf("Test process request: List directory\n");
  char *ROOT = "test_root_dir";
  const char *FILENAME = "test_server_list_dir";
  int root_created = 0;
  struct stat *st = {0};

  if (stat(ROOT, st) == -1) {
    mkdir(ROOT, 0700);
    root_created = 1;
  }

  write_to_file(FILENAME, "GET / HTTP/1.1");
  FILE *f = fopen(FILENAME, "a+");

  mu_assert("Error, f == NULL", f != NULL);

  process_request(f, ROOT);
  fclose(f);

  mu_assert("Error, should've printed directory listing",
            is_in_file(FILENAME, "Index of /") == 1);

  // sanitize after test
  if (root_created == 1)
    rmdir(ROOT);

  remove(FILENAME);
  printf("\n");
  return 0;
}

static char *test_process_request_no_dir_slash() {
  printf("Test process request: List directory\n");
  char *ROOT = "test_root_dir";
  char *SUBDIR = "test_root_dir/subdir";
  const char *FILENAME = "test_server_no_dir_slash";
  int root_created = 0;
  struct stat *st = {0};

  if (stat(ROOT, st) == -1) {
    mkdir(ROOT, 0700);
    mkdir(SUBDIR, 0700);
    root_created = 1;
  }

  write_to_file(FILENAME, "GET /subdir HTTP/1.1");
  FILE *f = fopen(FILENAME, "a+");

  mu_assert("Error, f == NULL", f != NULL);

  process_request(f, ROOT);
  fclose(f);

  mu_assert("Error, should've printed directory listing",
            is_in_file(FILENAME, "302") == 1);

  // sanitize after test
  if (root_created == 1) {
    rmdir(SUBDIR);
    rmdir(ROOT);
  }

  remove(FILENAME);
  printf("\n");
  return 0;
}

static char *test_process_request_index_found() {
  printf("Test process request: List directory\n");
  char *ROOT = "test_root_dir";
  const char *FILENAME = "test_server_index_found";
  const char *INDEXFILE = "test_root_dir/index.html";
  int root_created = 0;
  struct stat *st = {0};

  if (stat(ROOT, st) == -1) {
    mkdir(ROOT, 0700);
    root_created = 1;
  }

  write_to_file(INDEXFILE, "test index file");
  write_to_file(FILENAME, "GET / HTTP/1.1");
  FILE *f = fopen(FILENAME, "a+");

  mu_assert("Error, f == NULL", f != NULL);

  process_request(f, ROOT);
  fclose(f);

  mu_assert("Error, should've printed directory listing",
            is_in_file(FILENAME, "test index file") == 1);

  // sanitize after test
  remove(INDEXFILE);

  if (root_created == 1)
    rmdir(ROOT);

  remove(FILENAME);
  printf("\n");
  return 0;
}

static char *all_tests() {
  printf("===== TEST SERVER =====\n");
  mu_run_test(test_process_request_not_found);
  mu_run_test(test_process_request_bad_method);
  mu_run_test(test_process_request_invalid_request);
  mu_run_test(test_process_request_send_file);
  mu_run_test(test_process_request_list_dir);
  mu_run_test(test_process_request_no_dir_slash);
  mu_run_test(test_process_request_index_found);
  return 0;
}

int main(int argc, char **argv) {
  char *result = all_tests();
  if (result != 0) {
    printf("%s\n", result);
  } else {
    printf("ALL TESTS PASSED\n");
  }

  printf("Tests run: %d\n", tests_run);

  return result != 0;
}