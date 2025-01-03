#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <setjmp.h>

static int test_dirname(const char * path, const char * expect)
{
    char * tmp = strdup(path);
    char * n = dirname(tmp);
    int ret = strcmp(n, expect);
    free(tmp);
    return ret;
}

static int test_basename(const char * path, const char * expect)
{
    char * tmp = strdup(path);
    char * n = basename(tmp);
    int ret = strcmp(n, expect);
    free(tmp);
    return ret;
}

typedef struct  {
    char name[16];
} ref_entry;

static int cmp(const void * key_, const void * ent_)
{
    const char * key = key_;
    const ref_entry * ent = ent_;
    return strcmp(key, ent->name);
}

static void * thread_func(void * v)
{
    return (void *)0xc0de;
}

static int signal_fired = 0;
static void alarm_handler(int sig)
{
    signal_fired = 1;
}

static int trap_fired = 0;
static void trap_handler(int sig)
{
    trap_fired = 1;
}

#define TESTCASE(x) do { if (!(x)) printf("FAILED %d\n", __LINE__); } while(0)

int main(int argc, char **argv, char ** envp)
{
    TESTCASE(test_dirname("/usr/lib", "/usr") == 0); TESTCASE(test_basename("/usr/lib", "lib") == 0);
    TESTCASE(test_dirname("/usr/", "/") == 0);       TESTCASE(test_basename("/usr/", "usr") == 0);
    TESTCASE(test_dirname("usr", ".") == 0);         TESTCASE(test_basename("usr", "usr") == 0);
    TESTCASE(test_dirname("/", "/") == 0);           TESTCASE(test_basename("/", "/") == 0);
    TESTCASE(test_dirname(".", ".") == 0);           TESTCASE(test_basename(".", ".") == 0);
    TESTCASE(test_dirname("..", ".") == 0);          TESTCASE(test_basename("..", "..") == 0);

    const char abcde[] = "abcde";
    const char abcdx[] = "abcdx";

{
    const char xxxxx[] = "xxxxx";
    TESTCASE( memcmp( abcde, abcdx, 5 ) < 0 );
    TESTCASE( memcmp( abcde, abcdx, 4 ) == 0 );
    TESTCASE( memcmp( abcde, xxxxx, 0 ) == 0 );
    TESTCASE( memcmp( xxxxx, abcde, 1 ) > 0 );
}

{
    char s[] = "xxxxxxxxxxx";
    TESTCASE( memcpy( s, abcde, 6 ) == s );
    TESTCASE( s[4] == 'e' );
    TESTCASE( s[5] == '\0' );
    TESTCASE( memcpy( s + 5, abcde, 5 ) == s + 5 );
    TESTCASE( s[9] == 'e' );
    TESTCASE( s[10] == 'x' );
}

{
    char s[] = "xxxxabcde";
    TESTCASE( memmove( s, s + 4, 5 ) == s );
    TESTCASE( s[0] == 'a' );
    TESTCASE( s[4] == 'e' );
    TESTCASE( s[5] == 'b' );
    TESTCASE( memmove( s + 4, s, 5 ) == s + 4 );
    TESTCASE( s[4] == 'a' );
}

{
    char s[] = "xxxxxxxxx";
    TESTCASE( memset( s, 'o', 10 ) == s );
    TESTCASE( s[9] == 'o' );
    TESTCASE( memset( s, '_', ( 0 ) ) == s );
    TESTCASE( s[0] == 'o' );
    TESTCASE( memset( s, '_', 1 ) == s );
    TESTCASE( s[0] == '_' );
    TESTCASE( s[1] == 'o' );
}

{
    char s[] = "xx\0xxxxxx";
    TESTCASE( strcat( s, abcde ) == s );
    TESTCASE( s[2] == 'a' );
    TESTCASE( s[6] == 'e' );
    TESTCASE( s[7] == '\0' );
    TESTCASE( s[8] == 'x' );
    s[0] = '\0';
    TESTCASE( strcat( s, abcdx ) == s );
    TESTCASE( s[4] == 'x' );
    TESTCASE( s[5] == '\0' );
    TESTCASE( strcat( s, "\0" ) == s );
    TESTCASE( s[5] == '\0' );
    TESTCASE( s[6] == 'e' );
}

{
    char abccd[] = "abccd";
    TESTCASE( strchr( abccd, 'x' ) == NULL );
    TESTCASE( strchr( abccd, 'a' ) == &abccd[0] );
    TESTCASE( strchr( abccd, 'd' ) == &abccd[4] );
    TESTCASE( strchr( abccd, '\0' ) == &abccd[5] );
    TESTCASE( strchr( abccd, 'c' ) == &abccd[2] );
}

{
    char cmpabcde[] = "abcde";
    char cmpabcd_[] = "abcd\xfc";
    char empty[] = "";
    TESTCASE( strcmp( abcde, cmpabcde ) == 0 );
    TESTCASE( strcmp( abcde, abcdx ) < 0 );
    TESTCASE( strcmp( abcdx, abcde ) > 0 );
    TESTCASE( strcmp( empty, abcde ) < 0 );
    TESTCASE( strcmp( abcde, empty ) > 0 );
    TESTCASE( strcmp( abcde, cmpabcd_ ) < 0 );
}

{
    char cmpabcde[] = "abcde";
    char empty[] = "";
    TESTCASE( strcmp( abcde, cmpabcde ) == 0 );
    TESTCASE( strcmp( abcde, abcdx ) < 0 );
    TESTCASE( strcmp( abcdx, abcde ) > 0 );
    TESTCASE( strcmp( empty, abcde ) < 0 );
    TESTCASE( strcmp( abcde, empty ) > 0 );
}

{
    char s[] = "xxxxx";
    TESTCASE( strcpy( s, "" ) == s );
    TESTCASE( s[0] == '\0' );
    TESTCASE( s[1] == 'x' );
    TESTCASE( strcpy( s, abcde ) == s );
    TESTCASE( s[0] == 'a' );
    TESTCASE( s[4] == 'e' );
    TESTCASE( s[5] == '\0' );
}

{
    TESTCASE( strcspn( abcde, "x" ) == 5 );
    TESTCASE( strcspn( abcde, "xyz" ) == 5 );
    TESTCASE( strcspn( abcde, "zyx" ) == 5 );
    TESTCASE( strcspn( abcdx, "x" ) == 4 );
    TESTCASE( strcspn( abcdx, "xyz" ) == 4 );
    TESTCASE( strcspn( abcdx, "zyx" ) == 4 );
    TESTCASE( strcspn( abcde, "a" ) == 0 );
    TESTCASE( strcspn( abcde, "abc" ) == 0 );
    TESTCASE( strcspn( abcde, "cba" ) == 0 );
}

{
    TESTCASE( strerror( ERANGE ) != strerror( EDOM ) );
}

{
    TESTCASE( strlen( abcde ) == 5 );
    TESTCASE( strlen( "" ) == 0 );
}

{
    char s[] = "xx\0xxxxxx";
    TESTCASE( strncat( s, abcde, 9 ) == s );
    TESTCASE( s[2] == 'a' );
    TESTCASE( s[6] == 'e' );
    TESTCASE( s[7] == '\0' );
    TESTCASE( s[8] == 'x' );
    s[0] = '\0';
    TESTCASE( strncat( s, abcdx, 9 ) == s );
    TESTCASE( s[4] == 'x' );
    TESTCASE( s[5] == '\0' );
    TESTCASE( strncat( s, "\0", 9 ) == s );
    TESTCASE( s[5] == '\0' );
    TESTCASE( s[6] == 'e' );
    TESTCASE( strncat( s, abcde, 0 ) == s );
    TESTCASE( s[5] == '\0' );
    TESTCASE( s[6] == 'e' );
    TESTCASE( strncat( s, abcde, 3 ) == s );
    TESTCASE( s[5] == 'a' );
    TESTCASE( s[7] == 'c' );
    TESTCASE( s[8] == '\0' );
}

{
    char cmpabcde[] = "abcde\0f";
    char cmpabcd_[] = "abcde\xfc";
    char empty[] = "";
    char x[] = "x";
    TESTCASE( strncmp( abcde, cmpabcde, 5 ) == 0 );
    TESTCASE( strncmp( abcde, cmpabcde, 6 ) == 0 );
    TESTCASE( strncmp( abcde, abcdx, 5 ) < 0 );
    TESTCASE( strncmp( abcdx, abcde, 5 ) > 0 );
    TESTCASE( strncmp( empty, abcde, 5 ) < 0 );
    TESTCASE( strncmp( abcde, empty, 5 ) > 0 );
    TESTCASE( strncmp( abcde, abcdx, 4 ) == 0 );
    TESTCASE( strncmp( abcde, x, 0 ) == 0 );
    TESTCASE( strncmp( abcde, x, 1 ) < 0 );
    TESTCASE( strncmp( abcde, cmpabcd_, 6 ) < 0 );
}

{
    char s[] = "xxxxxxx";
    TESTCASE( strncpy( s, "", 1 ) == s );
    TESTCASE( s[0] == '\0' );
    TESTCASE( s[1] == 'x' );
    TESTCASE( strncpy( s, abcde, 6 ) == s );
    TESTCASE( s[0] == 'a' );
    TESTCASE( s[4] == 'e' );
    TESTCASE( s[5] == '\0' );
    TESTCASE( s[6] == 'x' );
    TESTCASE( strncpy( s, abcde, 7 ) == s );
    TESTCASE( s[6] == '\0' );
    TESTCASE( strncpy( s, "xxxx", 3 ) == s );
    TESTCASE( s[0] == 'x' );
    TESTCASE( s[2] == 'x' );
    TESTCASE( s[3] == 'd' );
}

{
    TESTCASE( strpbrk( abcde, "x" ) == NULL );
    TESTCASE( strpbrk( abcde, "xyz" ) == NULL );
    TESTCASE( strpbrk( abcdx, "x" ) == &abcdx[4] );
    TESTCASE( strpbrk( abcdx, "xyz" ) == &abcdx[4] );
    TESTCASE( strpbrk( abcdx, "zyx" ) == &abcdx[4] );
    TESTCASE( strpbrk( abcde, "a" ) == &abcde[0] );
    TESTCASE( strpbrk( abcde, "abc" ) == &abcde[0] );
    TESTCASE( strpbrk( abcde, "cba" ) == &abcde[0] );
}

{
    char abccd[] = "abccd";
    TESTCASE( strrchr( abcde, '\0' ) == &abcde[5] );
    TESTCASE( strrchr( abcde, 'e' ) == &abcde[4] );
    TESTCASE( strrchr( abcde, 'a' ) == &abcde[0] );
    TESTCASE( strrchr( abccd, 'c' ) == &abccd[3] );
}

{
    TESTCASE( strspn( abcde, "abc" ) == 3 );
    TESTCASE( strspn( abcde, "b" ) == 0 );
    TESTCASE( strspn( abcde, abcde ) == 5 );
}

{
    char s[] = "abcabcabcdabcde";
    TESTCASE( strstr( s, "x" ) == NULL );
    TESTCASE( strstr( s, "xyz" ) == NULL );
    TESTCASE( strstr( s, "a" ) == &s[0] );
    TESTCASE( strstr( s, "abc" ) == &s[0] );
    TESTCASE( strstr( s, "abcd" ) == &s[6] );
    TESTCASE( strstr( s, "abcde" ) == &s[10] );
}

{
    char s[] = "_a_bc__d_";
    TESTCASE( strtok( s, "_" ) == &s[1] );
    TESTCASE( s[1] == 'a' );
    TESTCASE( s[2] == '\0' );
    TESTCASE( strtok( NULL, "_" ) == &s[3] );
    TESTCASE( s[3] == 'b' );
    TESTCASE( s[4] == 'c' );
    TESTCASE( s[5] == '\0' );
    TESTCASE( strtok( NULL, "_" ) == &s[7] );
    //TESTCASE( s[6] == '_' ); //FIXME: empty tokens
    TESTCASE( s[7] == 'd' );
    TESTCASE( s[8] == '\0' );
    TESTCASE( strtok( NULL, "_" ) == NULL );
    strcpy( s, "ab_cd" );
    TESTCASE( strtok( s, "_" ) == &s[0] );
    TESTCASE( s[0] == 'a' );
    TESTCASE( s[1] == 'b' );
    TESTCASE( s[2] == '\0' );
    TESTCASE( strtok( NULL, "_" ) == &s[3] );
    TESTCASE( s[3] == 'c' );
    TESTCASE( s[4] == 'd' );
    TESTCASE( s[5] == '\0' );
    TESTCASE( strtok( NULL, "_" ) == NULL );
    strcpy( s, "" );
    TESTCASE( !strtok( s, "\n" ) );
}

{
    char s[] = "xxxxxxx";
    TESTCASE( stpncpy( s, "", 1 ) == s );
    TESTCASE( s[0] == '\0' );
    TESTCASE( s[1] == 'x' );
    TESTCASE( stpncpy( s, abcde, 6 ) == &s[5] );
    TESTCASE( s[0] == 'a' );
    TESTCASE( s[4] == 'e' );
    TESTCASE( s[5] == '\0' );
    TESTCASE( s[6] == 'x' );
    TESTCASE( stpncpy( s, abcde, 7 ) == &s[5] );
    TESTCASE( s[6] == '\0' );
    TESTCASE( stpncpy( s, "xxxx", 3 ) == &s[3] );
    TESTCASE( s[0] == 'x' );
    TESTCASE( s[2] == 'x' );
    TESTCASE( s[3] == 'd' );
}

{
    char tmp[64];
    memset(tmp, 0, sizeof(tmp)); snprintf(tmp, sizeof(tmp), "%+05d", 123); TESTCASE(!strcmp(tmp, "+0123"));
    memset(tmp, 0, sizeof(tmp)); snprintf(tmp, sizeof(tmp), "%+05d", 12345); TESTCASE(!strcmp(tmp, "+12345"));
    memset(tmp, 0, sizeof(tmp)); snprintf(tmp, sizeof(tmp), "%05d", -123); TESTCASE(!strcmp(tmp, "-0123"));
    memset(tmp, 0, sizeof(tmp)); snprintf(tmp, sizeof(tmp), "%05d", -12345); TESTCASE(!strcmp(tmp, "-12345"));
    memset(tmp, 0, sizeof(tmp)); snprintf(tmp, sizeof(tmp), "%5s", "abc"); TESTCASE(!strcmp(tmp, "  abc"));
    memset(tmp, 0, sizeof(tmp)); snprintf(tmp, sizeof(tmp), "%-5s", "abc"); TESTCASE(!strcmp(tmp, "abc  "));
    memset(tmp, 0, sizeof(tmp)); snprintf(tmp, sizeof(tmp), "%.*s", 3, "abcde"); TESTCASE(!strcmp(tmp, "abc"));
    memset(tmp, 0, sizeof(tmp)); snprintf(tmp, sizeof(tmp), "%#o", 0); TESTCASE(!strcmp(tmp, "0"));
    memset(tmp, 0, sizeof(tmp)); snprintf(tmp, sizeof(tmp), "%#o", 3); TESTCASE(!strcmp(tmp, "03"));
    memset(tmp, 0, sizeof(tmp)); snprintf(tmp, sizeof(tmp), "%#x", 0); TESTCASE(!strcmp(tmp, "0"));
    memset(tmp, 0, sizeof(tmp)); snprintf(tmp, sizeof(tmp), "%#x", 3); TESTCASE(!strcmp(tmp, "0x3"));
    memset(tmp, 0, sizeof(tmp)); snprintf(tmp, sizeof(tmp), "%#X", 3); TESTCASE(!strcmp(tmp, "0X3"));
}

{
    char * testv[] = {"program", "-ba", "arg"};
    int ret;
    ret =  getopt(sizeof(testv)/sizeof(testv[0]), testv, "a:b");
    TESTCASE(ret == 'b' && optind == 1);
    ret =  getopt(sizeof(testv)/sizeof(testv[0]), testv, "a:b");
    TESTCASE(ret == 'a' && optind == 3 && optarg == testv[2]);
    ret =  getopt(sizeof(testv)/sizeof(testv[0]), testv, "a:b");
    TESTCASE(ret == -1 && optind == 3);
}

{
    ref_entry entries[3] = {{"bisect"}, {"heads"}, {"tags"}};
    char * key = "worktree";
    TESTCASE(bsearch(key, entries, 3, sizeof(ref_entry), cmp) == NULL);
}

{
    char s[]="Hello";
    char * buf;
    size_t size;
    FILE * f;
    TESTCASE(f = open_memstream(&buf, &size));
    TESTCASE(fwrite(s, 1, strlen(s), f) == strlen(s));
    fclose(f);
    TESTCASE(size == strlen(s));
}

{
    char * end;
    TESTCASE(fabs(strtod("500", &end) - 500) < 0.00001 && !*end);
    TESTCASE(fabs(strtod("11e3", &end) - 11000) < 0.00001 && !*end);
    TESTCASE(fabs(strtod("1e-3", &end) - 0.001) < 0.00001 && !*end);
    TESTCASE(isnan(strtod("nan", &end)) && !*end);
    TESTCASE(isinf(strtod("inf", &end)) && !*end);
    TESTCASE(isinf(strtod("infinity", &end)) && !*end);
}

{
    TESTCASE(fabs(log2(1024) - 10.0) < 0.00001);
    TESTCASE(fabs(log2f(1024) - 10.0) < 0.00001);
}

{
    jmp_buf a;
    volatile int expect = 0;
    int ret = setjmp(a);
    TESTCASE(ret == expect);
    if (!ret) {
        expect = 1;
        longjmp(a, expect);
    }
}

{
    pthread_t t;
    pthread_create(&t, NULL, thread_func, (void *)0xc0de);
    void *ret;
    pthread_join(t, &ret);
    TESTCASE(ret == (void *)0xc0de);
}

    signal(SIGALRM, alarm_handler);
    kill(getpid(), SIGALRM);
    while(!signal_fired) ;

    signal(SIGTRAP, trap_handler);
    asm("int3");
    while(!trap_fired);

    TESTCASE(system("true") == 0);
    TESTCASE(system("false") != 0);

    printf("Tests complete\n");

    if (getpid() == 1) {
        int fd = open("/dev/power", O_WRONLY);
        if (fd != -1) {
            char value = 1;
            write(fd, &value, sizeof(value));
            close(fd);
        }
    }

    return EXIT_SUCCESS;
}
