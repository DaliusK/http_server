#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#ifndef SERVER_H_
#define SERVER_H_


/** Sends header information
 */
void send_header(FILE *f, int status, char *title, char *extra, char *mime, int length, time_t date);

/** Sends response to user if something goes wrong (mostly errors)
 */
void send_response(FILE *f, int status, char *title, char *extra, char *text);

/** Sends a file to the client
 */
void send_file(FILE *f, char *path, struct stat *statbuf);

/** Send directory listing
 */
void send_directory_listing(FILE *f, struct stat statbuf, char* relative_path, char* path);
/** Sets a simple <head> metadata with title
 */
void set_simple_head(FILE *f, char* title, char* description, char* keywords);
/** The main function for processing client's needs
 */
int process_request(FILE *f, char *root);

/** Gets the root for server to work on
 */
char* get_root();

/** a function for looping to accept requests from users and process them
 */
int loop(int sock, char* root);


#endif