#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>


void log_format(const char* tag, const char* message, va_list args)
{
    time_t curr_time;
    char *date = malloc(20 * sizeof(char));

    time(&curr_time);
    strftime(date, 20, "%Y-%m-%d %H:%M:%S", localtime(&curr_time));
    printf("%s [%s] ", date, tag);
    vprintf(message, args);
    if (message[strlen(message) - 1] != '\n')
        printf("\n");
    free(date);
}

void log_error(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    log_format("error", message, args);
    va_end(args);
}

void log_info(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    log_format("info", message, args);
    va_end(args);
}

void log_debug(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    log_format("debug", message, args);
    va_end(args);
}
