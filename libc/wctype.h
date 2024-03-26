#ifndef WCTYPE_H
#define WCTYPE_H

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

int iswalnum(wint_t);
int iswalpha(wint_t);
int iswctype(wint_t, wctype_t);
int iswprint(wint_t);
int iswspace(wint_t);
int iswupper(wint_t);
wint_t towlower(wint_t);
wint_t towupper(wint_t);
wctype_t wctype(const char *);

#ifdef __cplusplus
}
#endif

#endif /* WCTYPE_H */
