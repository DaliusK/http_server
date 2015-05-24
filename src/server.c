#include "server.h"
#include "helper.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER "http_server 0.1"
#define PROTOCOL "HTTP/1.1"
#define TIMEFORMAT "%a, %d %b %Y %H:%M:%S GMT" // still viable from 1.0 version

void set_simple_head(FILE *f, char *title, char *description, char *keywords) {
    fprintf(f, "<head>");
    fprintf(f, "<meta charset=\"UTF-8\" />");

    if (description)
        fprintf(f, "<meta name=\"description\" value=\"%s\" />", description);

    if (keywords)
        fprintf(f, "<meta name=\"keywords\" value=\"%s\" />", keywords);

    if (title)
        fprintf(f, "<title>%s</title>", title);

    fprintf(f, "</head>");
}

void send_header(FILE *f, int status, char *title, char *extra, char *mime,
                 int length, time_t date) {
    time_t now;
    char timebuf[128];

    // protocol, status, title and server name
    fprintf(f, "%s %d %s\r\n", PROTOCOL, status, title);
    fprintf(f, "Server: %s\r\n", SERVER);

    // current time
    now = time(NULL);
    strftime(timebuf, sizeof(timebuf), TIMEFORMAT, gmtime(&now));
    fprintf(f, "Date: %s\r\n", timebuf);

    // extra metadata to add
    // NOTE: might want to expand to extra array of metadata
    if (extra)
        fprintf(f, "%s\r\n", extra);

    // if mime type is known
    if (mime)
        fprintf(f, "Content-Type: %s\r\n", mime);

    if (length >= 0)
        fprintf(f, "Content-Length: %d\r\n", length);

    // modification date
    if (date != -1) {
        strftime(timebuf, sizeof(timebuf), TIMEFORMAT, gmtime(&date));
        fprintf(f, "Last-Modified: %s\r\n", timebuf);
    }
    // end it
    fprintf(f, "Connection: close\r\n");
    fprintf(f, "\r\n");
}

void send_response(FILE *f, int status, char *title, char *extra, char *text) {
    send_header(f, status, title, extra, "text/html", -1, -1);
    fprintf(f, "<html>");
    set_simple_head(f, title, text, NULL);
    fprintf(f, "<body>");
    fprintf(f, "<h1>%d - %s</h1>", status, title); // naughty HTML
    fprintf(f, "<p>%s\r\n</p>", text);
    fprintf(f, "</body></html>");
}

void send_file(FILE *f, char *path, struct stat *statbuf) {
    char data[1024];
    int n;

    FILE *file = fopen(path, "r");
    if (!file)
        send_response(f, 403, "Forbidden", NULL, "Access denied.");
    else {
        // get file stats - size
        int length = S_ISREG(statbuf->st_mode) ? statbuf->st_size : -1;
        send_header(f, 200, "OK", NULL, get_mimetype_by_name(path), length,
                    statbuf->st_mtime);

        while ((n = fread(data, 1, sizeof(data), file)) > 0)
            fwrite(data, 1, n, f);
        fclose(file);
    }
}

char *get_root() {
    int bufsize = 1023;
    char *buf = calloc(bufsize, sizeof(char));
    char *root = calloc(bufsize, sizeof(char));
    get_exec_path(buf, bufsize);
    int index = lastIndexOf(buf, "/");
    substr(root, bufsize, buf, index);
    strcat(root, "/../../../html");
    log_info("Setting root: %s", root);
    free(buf);
    return root;
}

void send_directory_listing(FILE *f, struct stat statbuf, char *relative_path,
                            char *path) {
    DIR *dir;
    struct dirent *de;

    send_header(f, 200, "OK", NULL, "text/html", -1, statbuf.st_mtime);

    fprintf(f, "<html>");
    set_simple_head(f, relative_path, relative_path, NULL);
    fprintf(f, "<body>");
    fprintf(f, "<h1>Index of %s</h1>\r\n", relative_path);
    fprintf(f, "<pre><table border=\"0\" width=\"100%%\">");
    fprintf(f, "<tr><th align=\"left\">Name</th><th align=\"left\">Last "
               "modified</th><th align=\"left\">Size</th></tr>");
    fprintf(f, "<hr />\r\n");

    dir = opendir(path);
    while ((de = readdir(dir)) != NULL) {
        char timebuf[32];
        struct tm *tm;

        char *pathbuf = calloc(strlen(path) + 128, sizeof(char));
        memcpy(pathbuf, path, strlen(path));
        pathbuf[strlen(pathbuf)] = '\0';
        strcat(pathbuf, de->d_name);

        stat(pathbuf, &statbuf);
        tm = gmtime(&statbuf.st_mtime);
        strftime(timebuf, sizeof(timebuf), "%Y-%b-%d %H:%M:%S", tm);

        fprintf(f, "<tr>");
        if (strcmp(de->d_name, ".") == 0)
            continue;

        // regressive and unecessary to see what's below root
        if (strcmp(relative_path, "/") == 0 && strcmp(de->d_name, "..") == 0)
            continue;

        fprintf(f, "<td><a href=\"%s%s\">", de->d_name,
                S_ISDIR(statbuf.st_mode) ? "/" : "");
        fprintf(f, "%s%s</a></td>", de->d_name,
                S_ISDIR(statbuf.st_mode) ? "/" : "");

        if (S_ISDIR(statbuf.st_mode))
            fprintf(f, "<td>%s</td>", timebuf);
        else
            fprintf(f, "<td>%s</td> <td>%10llu</td>", timebuf,
                    (unsigned long long)statbuf.st_size);
        fprintf(f, "</tr>");
        free(pathbuf);
    }
    closedir(dir);
    fprintf(f, "</table></pre>\r\n<hr /><address>%s</address>\r\n", SERVER);
    fprintf(f, "</body></html>");
}

int process_get_request(FILE *f, char *path, char *relative_path) {
    struct stat statbuf;
    char pathbuf[4096];
    int len;

    /* Check if path exists */
    if (stat(path, &statbuf) >= 0) {
        /* Check if it is a directory*/
        if (S_ISDIR(statbuf.st_mode)) {
            len = strlen(path);
            /* Check for directory compliance */

            if (len == 0 || path[len - 1] != '/') {
                /* HTTP 302 requires a Location in headers to be sent back */
                snprintf(pathbuf, sizeof(pathbuf), "Location: %s/",
                         relative_path);
                log_debug("HTTP 302, %s", pathbuf);
                send_response(f, 302, "Found", pathbuf,
                              "Directories must end with a slash.");
            } else {
                /* Show index.html or directory listing on directory request*/
                snprintf(pathbuf, sizeof(pathbuf), "%sindex.html", path);

                /* If it's a file, send it, else list the contents of dir */
                if (stat(pathbuf, &statbuf) >= 0) {
                    send_file(f, pathbuf, &statbuf);
                } else {
                    send_directory_listing(f, statbuf, relative_path, path);
                }
            }
        } else {
            send_file(f, path, &statbuf);
        }
    } else {
        send_response(f, 404, "Not found", NULL, "File not found");
    }
    return 0;
}

int process_request(FILE *f, char *root) {
    // TODO: divide into subfunctions
    char buf[1024];
    char *method;

    char *relative_path;
    char *protocol;

    if (!fgets(buf, sizeof(buf), f))
        return -1;

    log_info("%s", buf);

    // strtok - tokenizer strtok(NULL, " ") - takes another token
    method = strtok(buf, " ");
    relative_path = strtok(NULL, " ");
    protocol = strtok(NULL, "\r");

    if (!method || !relative_path || !protocol)
        return -1;

    // moved after checking the relative path
    char *path = calloc(strlen(root) + strlen(relative_path) + 1, sizeof(char));

    memmove(path, root, strlen(root));
    path[strlen(root) + 1] = '\0';
    strcat(path, relative_path);

    fseek(f, 0, SEEK_CUR); // change stream direction

    if (strcasecmp(method, "GET") == 0) {
        process_get_request(f, path, relative_path);
    } else {
        send_response(f, 501, "Not supported", NULL, "Method is not supported");
    }

    free(path);
    return 0;
}

int loop(int sock, char *root) {
    int s;
    FILE *f;
    struct sockaddr_in client;
    client.sin_family = AF_INET;
    socklen_t client_len = sizeof(client);

    s = accept(sock, (struct sockaddr *)&client, &client_len);
    if (s < 0)
        return 1;

    f = fdopen(s, "a+");
    log_info(inet_ntoa(client.sin_addr));
    process_request(f, root);
    fclose(f);
    return 0;
}
