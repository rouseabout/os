#include <locale.h>
#include <stddef.h>
#include <errno.h>
#include <syslog.h>

void freelocale(locale_t locobj)
{
}

struct lconv * localeconv(void)
{
    static const struct lconv conv = {
        .decimal_point = ".",
        .grouping = "",
        .thousands_sep = ","
    };

    return (struct lconv *)&conv;
}

locale_t newlocale(int category_mask, const char * locale, locale_t base)
{
    errno = ENOSYS;
    return (locale_t)0;
}

char * setlocale(int category, const char * locale)
{
    syslog(LOG_DEBUG, "libc: setlocale: category=%d, locale=%s", category, locale);
    return "foobar";
}
