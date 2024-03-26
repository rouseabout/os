#include <time.h>
#include <string.h>
#include <bsd/string.h>
#include <os/syscall.h>

char * asctime(const struct tm * timeptr)
{
    static char result[64];
    strftime(result, sizeof(result), "%a %b %e %H:%M:%S %Y", timeptr);
    return result;
}

char * ctime(const time_t * clock)
{
    return asctime(localtime(clock));
}

double difftime(time_t time1, time_t time0)
{
    return time1 - time0;
}

struct tm *gmtime(const time_t * timer)
{
    static struct tm result;
    return gmtime_r(timer, &result);
}

static const int dim[12] = {31,29,31,30,31,30,31,31,30,31,30,31};

static int is_common_year(int y)
{
    return (y % 4) || (!(y % 100) && (y % 400));
}

struct tm * gmtime_r(const time_t * timer, struct tm * result)
{
    memset(result, 0, sizeof(struct tm));

    time_t t = *timer;
    result->tm_sec = t % 60; t /= 60;
    result->tm_min = t % 60; t /= 60;
    result->tm_hour = t % 24; t /= 24;

    result->tm_year = 70;
    result->tm_wday = 4;
    result->tm_mday = 1;

    for (time_t i = 0; i < t; i++) {
        result->tm_wday++;
        result->tm_mday++;
        result->tm_yday++;

        if (result->tm_wday == 7)
            result->tm_wday = 0;

        if (result->tm_mday > dim[result->tm_mon] || (result->tm_mon==1 && result->tm_mday > 28 && is_common_year(result->tm_year + 1900))) {
            result->tm_mday = 1;
            result->tm_mon++;
        }

        if (result->tm_mon == 12) {
            result->tm_year++;
            result->tm_mon = 0;
            result->tm_yday = 0;
            result->tm_mday = 1;
        }
    }
    return result;
}

struct tm *localtime(const time_t * timer)
{
    static struct tm result;
    return localtime_r(timer, &result);
}

struct tm * localtime_r(const time_t * timer, struct tm * result)
{
    return gmtime_r(timer, result);
}

static int calc_yday(const struct tm * timeptr)
{
    int yday = timeptr->tm_mday - 1;

    for (int i = 0; i < timeptr->tm_mon; i++)
        if (i == 1)
            yday += is_common_year(1900 + timeptr->tm_year) ? 28 : 29;
        else
            yday += dim[i];

    return yday;
}

time_t mktime(struct tm * timeptr)
{
    timeptr->tm_yday = calc_yday(timeptr);
    return timeptr->tm_sec + timeptr->tm_min*60 + timeptr->tm_hour*3600 + timeptr->tm_yday*86400 +
        (timeptr->tm_year - 70)*31536000 + ((timeptr->tm_year - 69)/4)*86400 -
        ((timeptr->tm_year - 1)/100)*86400 + ((timeptr->tm_year + 299)/400)*86400;
}

#include <stdio.h>
#include <string.h>
#include <langinfo.h>
size_t strftime(char * s, size_t maxsize, const char * format, const struct tm * timeptr)
{
    const char * s_start = s;
    int state = 0;
    int remain = maxsize;
    for (; *format && remain; format++) {
        if (state) {
            switch(*format) {
            case 'a': snprintf(s, remain, "%s", nl_langinfo(ABDAY_1 + timeptr->tm_wday)); break;
            case 'b': snprintf(s, remain, "%s", nl_langinfo(ABMON_1 + timeptr->tm_mon)); break;
            case 'Y': snprintf(s, remain, "%04d", 1900 + timeptr->tm_year); break;
            case 'm': snprintf(s, remain, "%02d", timeptr->tm_mon + 1); break;
            case 'd': snprintf(s, remain, "%02d", timeptr->tm_mday); break;
            case 'e': snprintf(s, remain, "%2d", timeptr->tm_mday); break;
            case 'H': snprintf(s, remain, "%02d", timeptr->tm_hour); break;
            case 'M': snprintf(s, remain, "%02d", timeptr->tm_min); break;
            case 'S': snprintf(s, remain, "%02d", timeptr->tm_sec); break;
            default: snprintf(s, remain, "%%%c", *format); break;
            }
            int sz = strlen(s);
            s += sz;
            remain -= sz;
            state = 0;
        } else {
            if (*format == '%') {
                state = 1;
            } else {
                *s++ = *format;
                remain--;
            }
        }
    }
    *s = 0;
    return strlen(s_start);
}
