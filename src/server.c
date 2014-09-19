#include "server.h"
#include "helper.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>

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
    fprintf(f, "<h1>%d - %s</h1>", status, title);//naughty HTML
    fprintf(f, "<p>%s\r\n</p>", text);
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

int process_request(FILE *f)
{
    char buf[4096];
    char *method;
    char *path;
    char *protocol;
    struct stat statbuf;
    char pathbuf[4096];
    int len;

    if (!fgets(buf, sizeof(buf), f))
        return -1;
    printf("URL: %s", buf);

    //strtok - tokenizer strtok(NULL, " ") - takes another token
    method = strtok(buf, " ");
    char curr_dir[150];
    char *root;
    getcwd(curr_dir, 150);
    root = strcat(curr_dir, "/html");
    char *relative_path = strtok(NULL, " ");
    path = strcat(root, relative_path);
    protocol = strtok(NULL, "\r");
    
    if (!method || !path || !protocol)
        return -1;

    fseek(f, 0, SEEK_CUR); //change stream direction

    if (strcasecmp(method, "GET") != 0)
    {
        //currently only GET is supported
        send_response(f, 501, "Not supported", NULL, "Method is not supported");
    }else if (stat(path, &statbuf) < 0)
    {
        send_response(f, 404, "Not found", NULL, "File not found");
    }else if (S_ISDIR(statbuf.st_mode))
    {
        len = strlen(path);

        if (len == 0 || path[len -1] != '/')
        {
            snprintf(pathbuf, sizeof(pathbuf), "Location: %s/", path);
            send_response(f, 302, "Found", pathbuf, "Directories must end with a slash.");
        }else
        {
            snprintf(pathbuf, sizeof(pathbuf), "%sindex.html", path);
            if (stat(pathbuf, &statbuf) >= 0)
                send_file(f, pathbuf, &statbuf);
            else
            {
                //print directory listing
                DIR *dir;
                struct dirent *de;

                send_header(f, 200, "OK", NULL, "text/html", -1, statbuf.st_mtime);
                fprintf(f, "<h1>Index of %s</h1>\r\n", relative_path);
                fprintf(f, "<pre>Name\t\t\t\t\tLast Modified\t\t\t\t\tSize\r\n");
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

                    fprintf(f, "<a href=\"%s%s\">", de->d_name, S_ISDIR(statbuf.st_mode) ? "/" : "");
                    fprintf(f, "%s%s</a>", de->d_name, S_ISDIR(statbuf.st_mode) ? "/" : "");
                    if (strlen(de->d_name) < 32) 
                        fprintf(f, "\t\t\t\t\t%zu%s", 32 - strlen(de->d_name), "");
                    if (S_ISDIR(statbuf.st_mode))
                        fprintf(f, "\t\t\t\t\t%s\r\n", timebuf);
                    else
                        fprintf(f, "\t\t\t\t\t%s %10zu\r\n", timebuf, statbuf.st_size);
                }
                closedir(dir);

                fprintf(f, "</pre>\r\n<hr /><address>%s</address>\r\n", SERVER);
            }
        }
    }
    else
        send_file(f, path, &statbuf);

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

    while (1)
    {
        int s;
        FILE *f;
        s = accept(sock, NULL, NULL);
        if (s < 0)
            break;

        f = fdopen(s, "a+");
        process_request(f);
        fclose(f);
    }

    close(sock);
    return 0;
}