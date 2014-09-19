#ifndef SERVER_H_
#define SERVER_H_

/** Deduces mime type by file name
 */
char *get_mimetype_by_name(char *name);

/** Deduces mime type by file extension
 */
char *get_mimetype_by_ext(char **ext);

/** Sends header information
 */
void send_header(FILE *f, int status, char *title, char *extra, char *mime, int length, time_t date);

/** Sends response to user if something goes wrong (mostly errors)
 */
void send_response(FILE *f, int status, char *title, char *extra, char *text);

/** Sends a file to the client
 */
void send_file(FILE *f, char *path, struct stat *statbuf);

/** The main function for processing client's needs
 */
int process_request(FILE *f);

/** Simple structure for mime
  * NOTE: might need expansion on later development
 */
struct mime;

#endif