#include "server.h"
#include "helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER "http_server 0.1"
#define PROTOCOL "HTTP/1.1"
#define TIMEFORMAT "%a, %d %b %Y %H:%M:%S GMT"//still viable from 1.0 version
#define PORT 8080
typedef struct
{
    char * extension;
    char * type;
} mime;

mime mimes[13] =
{
    {".html", "text/html"},
    {".htm",  "text/html"},
    {".css",  "text/css"},
    {".js",   "text/javascript"},
    {".jpg",  "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png",  "image/png"},
    {".gif",  "imag/gif"},
    {".wav",  "audio/wav"},
    {".mp3",  "audio/mpeg"},
    {".avi",  "video/x-msvideo"},
    {".mpg",  "video/mpeg"},
    {".mpeg", "video/mpeg"}
};

char *get_mimetype_by_name(char *name)
{
    char *extension = strrchr(name, '.');

    //check if extension even exists
    if (!extension)
        return NULL;

    return get_mimetype_by_ext(&extension);
}

char *get_mimetype_by_ext(char **ext)
{

    char *extension = *ext;//local copy
    to_lowercase(extension);

    int i;
    for (i = 0; i < sizeof(mimes) / sizeof(mime); i++)
    {
        if (strcmp(mimes[i].extension, extension) == 0)
        {
            return mimes[i].type;
        }
    }
    //nothing found - return NULL
    return NULL;
}

void set_simple_head(FILE *f, char* title, char* description, char* keywords)
{
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

void send_header(FILE *f, int status, char *title, char *extra, char *mime, int length, time_t date)
{
    time_t now;
    char timebuf[128];

    //protocol, status, title and server name
    fprintf(f, "%s %d %s\r\n", PROTOCOL, status, title);
    fprintf(f, "Server: %s\r\n", SERVER);

    //current time
    now = time(NULL);
    strftime(timebuf, sizeof(timebuf), TIMEFORMAT, gmtime(&now));
    fprintf(f, "Date: %s\r\n", timebuf);

    //extra metadata to add
    //NOTE: might want to expand to extra array of metadata
    if (extra)
        fprintf(f, "%s\r\n", extra);

    //if mime type is known
    if (mime)
        fprintf(f, "Content-Type: %s\r\n", mime);

    if (length >= 0)
        fprintf(f, "Content-Length: %d\r\n", length);

    //modification date
    if (date != -1)
    {
        strftime(timebuf, sizeof(timebuf), TIMEFORMAT, gmtime(&date));
        fprintf(f, "Last-Modified: %s\r\n", timebuf);
    }
    //end it
    fprintf(f, "Connection: close\r\n");
    fprintf(f, "\r\n");
}

void send_response(FILE *f, int status, char *title, char *extra, char *text)
{
    send_header(f, status, title, extra, "text/html", -1, -1);
    fprintf(f, "<html>");
    set_simple_head(f, title, text, NULL);
    fprintf(f, "<body>");
    fprintf(f, "<h1>%d - %s</h1>", status, title);//naughty HTML
    fprintf(f, "<p>%s\r\n</p>", text);
    fprintf(f, "</body></html>");
}

void send_file(FILE *f, char *path, struct stat *statbuf)
{
    char data[4096];
    int n;

    FILE *file = fopen(path, "r");
    if (!file)
        send_response(f, 403, "Forbidden", NULL, "Access denied.");
    else
    {
        //get file stats - size
        int length = S_ISREG(statbuf->st_mode) ? statbuf->st_size : -1;
        send_header(f, 200, "OK", NULL, get_mimetype_by_name(path), length, statbuf->st_mtime);

        while((n = fread(data, 1, sizeof(data), file)) > 0)
            fwrite(data, 1, n, f);
        fclose(file);
    }
}

char* get_root()
{
    int bufsize = 128;
    char *buf = malloc(bufsize * sizeof(char));
    char *subbuf = malloc(bufsize * sizeof(char));
    get_exec_path(buf, bufsize);
    int index = lastIndexOf(buf, "/");
    substr(subbuf, bufsize, buf, index);
    char *root = strcat(subbuf, "/html");
    printf("Setting root: %s\n", root);
    return root;
}

void send_directory_listing(FILE *f, struct stat statbuf, char* relative_path, char* path)
{
    DIR *dir;
    struct dirent *de;
    char pathbuf[4096];

    send_header(f, 200, "OK", NULL, "text/html", -1, statbuf.st_mtime);

    fprintf(f, "<html>");
    set_simple_head(f, relative_path, relative_path, NULL);
    fprintf(f, "<body>");
    fprintf(f, "<h1>Index of %s</h1>\r\n", relative_path);
    fprintf(f, "<pre><table border=\"0\" width=\"100%%\">");
    fprintf(f, "<tr><th align=\"left\">Name</th><th align=\"left\">Last modified</th><th align=\"left\">Size</th></tr>");
    fprintf(f, "<hr />\r\n"); 

    dir = opendir(path);
    while((de = readdir(dir)) != NULL)
    {
        char timebuf[32];
        struct tm *tm;

        strcpy(pathbuf, path);
        strcat(pathbuf, de->d_name);

        stat(pathbuf, &statbuf);
        tm = gmtime(&statbuf.st_mtime);
        strftime(timebuf, sizeof(timebuf), "%Y-%b-%d %H-%M-%S", tm);

        fprintf(f, "<tr>");
        if (strcmp(de->d_name, ".") == 0)
            continue;

        //regressive and unecessary to see what's below root
        if (strcmp(relative_path, "/") == 0 && strcmp(de->d_name, "..") == 0)
            continue;

        fprintf(f, "<td><a href=\"%s%s\">", de->d_name, S_ISDIR(statbuf.st_mode) ? "/" : "");
        fprintf(f, "%s%s</a></td>", de->d_name, S_ISDIR(statbuf.st_mode) ? "/" : "");
            
        if (S_ISDIR(statbuf.st_mode))
            fprintf(f, "<td>%s</td>", timebuf);
        else
            fprintf(f, "<td>%s</td> <td>%10zu</td>", timebuf, statbuf.st_size);
        fprintf(f, "</tr>");
    }
    closedir(dir);

    fprintf(f, "</table></pre>\r\n<hr /><address>%s</address>\r\n", SERVER);
    fprintf(f, "</body></html>");
}

int process_request(FILE *f, char *root)
{
    //TODO: divide into subfunctions
    char buf[4096];
    char *method;
    char *path = malloc(4096 * sizeof(char));
    char *relative_path;
    char *protocol;
    struct stat statbuf;
    char pathbuf[4096];
    int len;


    if (!fgets(buf, sizeof(buf), f))
        return -1;
    printf("noriu %s", buf);

    //strtok - tokenizer strtok(NULL, " ") - takes another token
    method = strtok(buf, " ");
    relative_path = strtok(NULL, " ");
    strncpy(path, root, 4096);
    strcat(path, relative_path);
    protocol = strtok(NULL, "\r");
    
    if (!method || !path || !protocol)
        return -1;

    fseek(f, 0, SEEK_CUR); //change stream direction


    if (strcasecmp(method, "GET") == 0)
    {
        /* Check if path exists */
        if (stat(path, &statbuf) >= 0)
        {
            /* Check if it is a directory*/
            if (S_ISDIR(statbuf.st_mode))
            {
                len = strlen(path);

                /* Check for directory compliance */
                if (len == 0 || path[len - 1] != '/')
                {
                    //HTTP 302 requires a Location in headers to be sent back
                    snprintf(pathbuf, sizeof(pathbuf), "Location: %s/", relative_path);
                    printf("HTTP 302, %s\n", pathbuf);
                    send_response(f, 302, "Found", pathbuf, "Directories must end with a slash.");
                }
                else
                {
                    /* Show index.html or directory listing on directory request*/
                    snprintf(pathbuf, sizeof(pathbuf), "%sindex.html", path);
                    
                    if (stat(pathbuf, &statbuf) >= 0)
                    {
                        send_file(f, pathbuf, &statbuf);
                    }
                    else
                    {
                        send_directory_listing(f, statbuf, relative_path, path);
                    }
                }
            }
            else
            {
                send_file(f, path, &statbuf);
            }
        }
        else
        {
            send_response(f, 404, "Not found", NULL, "File not found");
        }
    }
    else
    {
        send_response(f, 501, "Not supported", NULL, "Method is not supported");
    }
    return 0;
}

int loop(int sock, char* root)
{
    int s;
    FILE *f;
    struct sockaddr_in client;
    client.sin_family = AF_INET;
    socklen_t client_len = sizeof(client);

    s = accept(sock, (struct sockaddr*)&client, &client_len);
    if (s < 0)
        return 1;

    printf("%s: ", inet_ntoa(client.sin_addr));
    f = fdopen(s, "a+");
    process_request(f, root);
    fclose(f);

    return 0;
}

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in sin;
    sock = socket(AF_INET, SOCK_STREAM, 0);

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr * ) &sin, sizeof(sin)) != 0)
    {
        printf("Failed to bind to socket\n");
        return 1;
    }


    listen(sock, 5);
    printf("HTTP server listening on port %d\n", PORT);
    char *root = get_root();
    int loop_result = loop(sock, root);
    while (loop_result != 1)
    {
        loop_result = loop(sock, root);
    }

    close(sock);
    return loop_result;
}