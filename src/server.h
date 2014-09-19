#ifndef SERVER_H_
#define SERVER_H_

/** Deduces mime type by file name
 */
char *get_mimetype_by_name(char *name);
/** Deduces mime type by file extension
 */
char *get_mimetype_by_ext(char **ext);

/** Simple structure for mime
  * NOTE: might need expansion on later development
 */
struct mime;

#endif