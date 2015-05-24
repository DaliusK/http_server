#include "helper.h"
#include "logger.h"
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

typedef struct {
    char *extension;
    char *type;
} mime;

mime mimes[] = {{".html", "text/html"},
                {".htm", "text/html"},
                {".css", "text/css"},
                {".js", "text/javascript"},
                {".jpg", "image/jpeg"},
                {".jpeg", "image/jpeg"},
                {".png", "image/png"},
                {".gif", "imag/gif"},
                {".wav", "audio/wav"},
                {".mp3", "audio/mpeg"},
                {".avi", "video/x-msvideo"},
                {".mpg", "video/mpeg"},
                {".mpeg", "video/mpeg"}};

void to_lowercase(char *string) {
    int i;
    for (i = 0; string[i]; i++)
        string[i] = tolower(string[i]);
}

OS get_os() {
// help from
// http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system#Howtodetecttheoperatingsystemtype
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
    return BSD;
#elif defined(__linux__)
    return LINUX;
#elif defined(__sun) && defined(__SVR4)
    return SOLARIS;
#elif defined(__WIN64) || defined(__WIN32)
    return WINDOWS;
#endif
    return OTHER_UNIX;
}

void get_exec_path(char *buf, int bufsize) {
    // guided by http://stackoverflow.com/a/933996/552214

    readlink("/proc/self/exe", buf, bufsize);
    log_debug("Exec path: %s", buf);
}

void substr(char *buffer, size_t buflen, char const *source, int len) {
    size_t srclen = strlen(source);
    size_t nbytes = 0;
    size_t offset = 0;
    size_t sublen;

    if (buflen == 0) /* Can't write anything anywhere */
        return;

    if (len > 0) {
        sublen = len;
        /* if substr index is bigger than source last index */
        nbytes = (sublen > srclen) ? srclen : sublen;
        offset = 0;
    } else if (len < 0) {
        sublen = -len;
        nbytes = (sublen > srclen) ? srclen : sublen;
        offset = srclen - nbytes;
    }

    if (nbytes >= buflen)
        nbytes = 0;

    if (nbytes > 0)
        memmove(buffer, source + offset, nbytes);
    buffer[nbytes] = '\0';
}

int indexOf_shift(char *base, char *str, int startIndex) {
    int result;
    int baselen = strlen(base);
    // str should not longer than base
    if (strlen(str) > baselen || startIndex > baselen)
        result = -1;
    else {
        if (startIndex < 0)
            startIndex = 0;

        char *pos = strstr(base + startIndex, str);
        if (pos == NULL)
            result = -1;
        else
            result = pos - base;
    }
    return result;
}

int lastIndexOf(char *base, char *str) {
    int result;
    // str should not longer than base
    if (strlen(str) > strlen(base))
        result = -1;
    else {
        int start = 0;
        int endinit = strlen(base) - strlen(str);
        int end = endinit;
        int endtmp = endinit;
        while (start != end) {
            start = indexOf_shift(base, str, start);
            end = indexOf_shift(base, str, end);

            // not found from start
            if (start == -1)
                end = -1; // then break;
            else if (end == -1) {
                // found from start
                // but not found from end
                // move end to middle
                if (endtmp == (start + 1))
                    end = start; // then break;
                else {
                    end = endtmp - (endtmp - start) / 2;
                    if (end <= start)
                        end = start + 1;
                    endtmp = end;
                }
            } else {
                // found from both start and end
                // move start to end and
                // move end to base - strlen(str)
                start = end;
                end = endinit;
            }
        }
        result = start;
    }
    return result;
}

char *get_mimetype_by_name(char *name) {
    char *extension = strrchr(name, '.');

    // check if extension even exists
    if (!extension)
        return NULL;

    return get_mimetype_by_ext(&extension);
}

char *get_mimetype_by_ext(char **ext) {

    char *extension = *ext; // local copy
    to_lowercase(extension);

    int i;
    for (i = 0; i < sizeof(mimes) / sizeof(mime); i++) {
        if (strcmp(mimes[i].extension, extension) == 0) {
            return mimes[i].type;
        }
    }
    // nothing found - return NULL
    return NULL;
}

void decode_url(char *src, int srclen, char *dest) {
    int i;
    char *str = src;
    for (i = 0; i < srclen; i++, str++, dest++) {
        if (*str == '+') {
            *dest = ' ';
        } else if (*str == '%') {
            int code;
            // match two numbers into code. There should be one match
            if (sscanf(str + 1, "%2x", &code) != 1) {
                // don't know what the hell this is - use ?
                code = '?';
            } // else - normal code is retrieved
            *dest = code;
            str += 2; // skip forward by 2 hex numbers
        } else {
            *dest = *str;
        }
    }
    *dest = '\n';
    *++dest = '\0';
}