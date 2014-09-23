#ifndef HELPER_H_
#define HELPER_H_

#include <string.h>

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

/** Turns a string into lowercase
 */
void to_lowercase(char *string);

/** Needed for defining where there server is being run
 */
typedef enum
{
    WINDOWS,
    LINUX,
    BSD,
    SOLARIS,
    OTHER_UNIX
} OS;

/** Returns the operating system enum
 */
OS get_os();

/*
** Given substr(buffer, sizeof(buffer), "string", len), then the output
** in buffer for different values of len is:
** For positive values of len:
** 0    ""
** 1    "s"
** 2    "st"
** ...
** 6    "string"
** 7    "string"
** ...
** For negative values of len:
** -1   "g"
** -2   "ng"
** ...
** -6   "string"
** -7   "string"
** ...
** Subject to buffer being long enough.
** If buffer is too short, the empty string is set (unless buflen is 0,
** in which case, everything is left untouched).
*/
void substr(char *buffer, size_t buflen, char const *source, int len);

/* int indexOf_shift (char* base, char* str, int startIndex)
 *            Custom function for getting the first index of str in base
 *            after the given startIndex
 */
int indexOf_shift(char* base, char* str, int startIndex);

/** use two index to search in two part to prevent the worst case
 * (assume search 'aaa' in 'aaaaaaaa', you cannot skip three char each time)
 * code attributed to http://ben-bai.blogspot.com/2013/03/c-string-startswith-endswith-indexof.html
 */
int lastIndexOf(char* base, char* str);

/** Returns executable's path
 */
void get_exec_path(char *buf, int bufsize);

/** Decodes URL (all these cryptic hex codes)
 * src string should be given in exactly the url to be decoded
 */
void decode_url(char *src, int srclen, char *dest);

#endif