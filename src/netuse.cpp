#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <ctype.h>

static char whitespace[] = " \t";

static char* GetToken (char* &buffer);

int main (int argc, char *argv[])
{
    char buffer[512];

    FILE *results = _popen ("net use", "rt");

    if (results == NULL)
    {   printf ("_popen failed\n");
        return 1;
    }

    while (!feof (results))
    {
        if (fgets (buffer, sizeof(buffer), results) == NULL)
            continue;

        char *ptr = buffer;
        char *token = buffer;

        token = GetToken (ptr);

        if (  (0 != _stricmp (token, "ok"))
           && (0 != _stricmp (token, "disconnected"))
           )
        {
            continue;
        }

        token = GetToken (ptr);

        if (!isalpha(token[0]) || (token[1] != ':'))
            continue;

        char *drive = token;

        token = GetToken (ptr);

        if ((token[0] != '\\') || (token[1] != '\\'))
            continue;

        printf ("%s \"%s\"\n", drive, token);
    }

    _pclose (results);

    return 0;
}


static char* GetToken (char* &buffer)
{
    char *ptr = buffer;

    if (!*ptr)
        return NULL;

    while (*ptr && isspace(*ptr))
        ++ptr;

    char *token = ptr;

    if (!*ptr) return token;

    while (*ptr && !isspace(*ptr))
        ++ptr;

    if (*ptr)
        *ptr++ = 0;

    buffer = ptr;
    return token;
}
