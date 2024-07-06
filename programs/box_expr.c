#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VALIDATE \
    if (i >= argc) { \
        fprintf(stderr, "error\n"); \
        exit(EXIT_FAILURE); \
    }

static int equality(int argc, char ** argv, int i, int * ri);

static int terminal(int argc, char ** argv, int i, int * ri)
{
    VALIDATE
    int v;
    if (!strcmp(argv[i], "(")) {
        i++;
        VALIDATE;
        v = equality(argc, argv, i, &i);
        VALIDATE;
        if (strcmp(argv[i], ")")) {
            fprintf(stderr, "error\n");
            exit(EXIT_FAILURE);
        }
    } else {
        v = atoi(argv[i]);
    }
    *ri = i + 1;
    return v;
}

static int multiplicative(int argc, char ** argv, int i, int * ri)
{
    int v = terminal(argc, argv, i, &i);
    while (1) {
        if (i < argc && !strcmp(argv[i], "*")) {
            v *= terminal(argc, argv, i + 1, &i);
            continue;
        }
        if (i < argc && !strcmp(argv[i], "/")) {
            v /= terminal(argc, argv, i + 1, &i);
            continue;
        }
        if (i < argc && !strcmp(argv[i], "%")) {
            v %= terminal(argc, argv, i + 1, &i);
            continue;
        }
        *ri = i;
        return v;
    }
}

static int additive(int argc, char ** argv, int i, int * ri)
{
    int v = multiplicative(argc, argv, i, &i);
    while (1) {
        if (i < argc && !strcmp(argv[i], "+")) {
            v += multiplicative(argc, argv, i + 1, &i);
            continue;
        }
        if (i < argc && !strcmp(argv[i], "-")) {
            v -= multiplicative(argc, argv, i + 1, &i);
            continue;
        }
        *ri = i;
        return v;
    }
}

static int relational(int argc, char ** argv, int i, int * ri)
{
    int v = additive(argc, argv, i, &i);
    while (1) {
        if (i < argc && !strcmp(argv[i], ">")) {
            v = v > additive(argc, argv, i + 1, &i);
            continue;
        }
        if (i < argc && !strcmp(argv[i], "<")) {
            v = v < additive(argc, argv, i + 1, &i);
            continue;
        }
        *ri = i;
        return v;
    }
}

static int equality(int argc, char ** argv, int i, int * ri)
{
    int v = relational(argc, argv, i, &i);
    while (1) {
        if (i < argc && !strcmp(argv[i], "=")) {
            v = v == relational(argc, argv, i + 1, &i);
            continue;
        }
        *ri = i;
        return v;
    }
}

static int expr_main(int argc, char ** argv, char ** envp)
{
    if (argc < 2) {
        fprintf(stderr, "%s EXPRESSION\n", argv[0]);
        return EXIT_FAILURE;
    }
    int i;
    printf("%d\n", equality(argc, argv, 1, &i));
    return EXIT_SUCCESS;
}
