#include <wctype.h>
#include <ctype.h>
#include <syslog.h>

int iswalnum(wint_t wc)
{
    return isalnum(wc);
}

int iswalpha(wint_t wc)
{
    return isalpha(wc);
}

int iswctype(wint_t wc, wctype_t charclass)
{
    syslog(LOG_DEBUG, "libc: iswctype %d %d", wc, (int)charclass);
    return 0;
}

int iswprint(wint_t wc)
{
    return isprint(wc);
}

int iswspace(wint_t wc)
{
    return isspace(wc);
}

int iswupper(wint_t wc)
{
    return isupper(wc);
}

wint_t towlower(wint_t wc)
{
    return tolower(wc);
}

wint_t towupper(wint_t wc)
{
    return toupper(wc);
}

wctype_t wctype(const char * property)
{
    syslog(LOG_DEBUG, "libc: wctype: %s", property);
    return 0;
}
