#include "helper.h"
#include <ctype.h>

void to_lowercase(char *string)
{
    int i;
    for (i = 0; string[i]; i++)
        string[i] = tolower(string[i]);
}