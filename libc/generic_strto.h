//FIXME: base==0 case, parse leading '0', '0x'
#define MK_STRTOL(type, name) \
type name(const char * str, char ** endptr, int base) \
{ \
    type v = 0; \
    int negative = 0; \
    while (*str) { \
        char c = *str; \
        if (c == '-') { \
            negative = 1; \
        } else if (isdigit(c)) { \
            v *= base; \
            v += c - '0'; \
        } else if (isalpha(c) && tolower(c) <= 'f') { \
            v *= base; \
            v += 10 + tolower(c) - 'a'; \
        } else \
            break; \
        str++; \
    } \
    if (endptr) \
        *endptr = (char *)str; \
    return negative ? -v : v; \
}
