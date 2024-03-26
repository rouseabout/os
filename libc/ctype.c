#include <ctype.h>
#include <string.h>

int isascii(int c)
{
    return c < 128;
}

int isalnum(int c)
{
    return isalpha(c) || isdigit(c);
}

int isalpha(int c)
{
    return isupper(c) || islower(c);
}

int iscntrl(int c)
{
    return c < ' ' || c == 127;
}

int isdigit(int c)
{
    return c >= '0' && c <= '9';
}

int isgraph(int c)
{
    return isprint(c);
}

int islower(int c)
{
    return c >= 'a' && c <= 'z';
}

int isprint(int c)
{
    return c >= ' ' && c < 127;
}

int ispunct(int c)
{
    return c > 0 ? !!strchr("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", c) : 0;
}

int isspace(int c)
{
    return c == ' ' || c == '\t';
}

int isupper(int c)
{
    return c >= 'A' && c <= 'Z';
}

int isxdigit(int c)
{
    return isdigit(c) || ( c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

int toascii(int c)
{
    return c < 128 ? c : 0;
}

static const char lower[] = "abcdefghijklmnopqrstuvwxyz";
int tolower(int c)
{
    return c >= 'A' && c <= 'Z' ? lower[c - 'A'] : c;
}

static const char upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
int toupper(int c)
{
    return c >= 'a' && c <= 'z' ? upper[c - 'a'] : c;
}
