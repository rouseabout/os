#include <assert.h>
#include <ctype.h>
#include <elf.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define NB_ELEMS(x) (sizeof(x)/sizeof((x)[0]))

#define PAD16(x) (((x) + 16) & ~15)

static char * mallocf(const char * fmt, ...)
{
    va_list args;
    size_t size;
    char * s;
    va_start(args, fmt);
    size = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    s = malloc(size + 1);
    assert(s);

    va_start(args, fmt);
    vsnprintf(s, size + 1, fmt, args);
    va_end(args);
    return s;
}

static const char * punctuator[]= { /* search algorith is greedy, so place longest punctuators first */
    "%", "[", "]", ":", ",", "$", "+", "-",
};

static int ispunctuator(const char * s)
{
    for (int i = 0; i < NB_ELEMS(punctuator); i++)
        if (!strncmp(s, punctuator[i], strlen(punctuator[i])))
            return strlen(punctuator[i]);
    return 0;
}

typedef enum {
    TOK_NUMBER = 0,
    TOK_STRING,
    TOK_PUNCTUATOR,
    TOK_LITERAL,
    TOK_EOF
} TokenKind;

typedef struct Token Token;
struct Token {
    TokenKind kind;
    char * str;
    int size;
    int at_space;
    int at_begin;
    const char * path;
    Token * next;
};

static void perror2(const char * where)
{
    perror(where);
    exit(EXIT_FAILURE);
}

static Token * make_token(TokenKind kind, char * start, const char * end, int at_space, int at_begin, const char * path)
{
    Token * tok = malloc(sizeof(Token));
    if (!tok)
        perror2("malloc");
    tok->kind = kind;
    tok->str = start;
    tok->size = end - start;
    tok->at_space = at_space;
    tok->at_begin = at_begin;
    tok->path = path;
    tok->next = NULL;
    return tok;
}

static void tokenise_error(const char * path, const char * fmt, ...)
{
    fprintf(stderr, "%s: ", path);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

static void consume_string(const char * path, char ** send, char *s)
{
    int escape_mode = 0;
    while (*s) {
        if (escape_mode) {
            escape_mode = 0;
        } else {
            if (*s == '\\') {
                escape_mode = 1;
            }
            if (*s == '\"')
                break;
        }
        s++;
    }

    *send = s;
    if (escape_mode)
        tokenise_error(path, "unmatched quote");
}

static int ishexdigit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static char unescape_char(const char * path, char ** send, char * s)
{
    if (*s != '\\') {
        *send = s + 1;
        return *s;
    }

    s++;
    if (!*s)
        tokenise_error(path, "incomplete escape char");

    if (*s >= '0' && *s <= '7') {
        int c = *s++ - '0';
        if (*s >= '0' && *s <= '7') {
            c = c*8 + *s++ - '0';
            if (*s >= '0' && *s <= '7')
                c = c*8 + *s++ - '0';
        }
        *send = s;
        return c;
    }

    if (s[0] == 'x' && ishexdigit(s[1]) && ishexdigit(s[2])) {
        int c = strtol(s + 1, NULL, 16);
        *send = s + 3;
        return c;
    }

    switch(*s) {
    case '\\': *send = s + 1; return '\\';
    case '\'': *send = s + 1; return '\'';
    case '"': *send = s + 1; return '"';
    case 'a': *send = s + 1; return '\a';
    case 'b': *send = s + 1; return '\b';
    case 'e': *send = s + 1; return 0x1b;
    case 'f': *send = s + 1; return '\f';
    case 'n': *send = s + 1; return '\n';
    case 'r': *send = s + 1; return '\r';
    case 't': *send = s + 1; return '\t';
    case 'v': *send = s + 1; return '\v';
    }

    tokenise_error(path, "unsuported escape code: '%c'", *s);
    return 0;
}

static char unescape_quoted_char(const char * path, char **send, char * s)
{
    char c = unescape_char(path, &s, s);
    if (*s != '\'')
        tokenise_error(path, "unterminated character");
    *send = s + 1;
    return c;
}

static Token * tokenise(const char * path, char * s)
{
    int at_space = 0;
    int at_begin = 1;
    Token head = {.next=NULL};
    Token * tok = &head;
    while (*s) {
        if (*s == ';') {
            while (*s++ != '\n') ;
            at_space = 1;
            at_begin = 1;
            continue;
        } else if (*s == '\n') {
            s++;
            at_space = 1;
            at_begin = 1;
            continue;
        } else if (isspace(*s)) {
            s++;
            at_space = 1;
            continue;
        } else if (isdigit(*s)) {
            char * t;
            for (t = s + 1; *t && (isdigit(*t) || strchr("abcdefABCDEFxX", *t)); t++) ;
            tok = tok->next = make_token(TOK_NUMBER, s, t, at_space, at_begin, path);
            s = t;
            at_space = 0;
            at_begin = 0;
            continue;
        } else if (ispunctuator(s)) {
            tok = tok->next = make_token(TOK_PUNCTUATOR, s, s + 1, at_space, at_begin, path);
            s++;
            at_space = 0;
            at_begin = 0;
            continue;
        } else if (isalpha(*s) || strchr("_.", *s)) {
            char * t;
            for (t = s + 1; *t && (isalnum(*t) || strchr("_.", *t)); t++) ;
            tok = tok->next = make_token(TOK_LITERAL, s, t, at_space, at_begin, path);
            s = t;
            at_space = 0;
            at_begin = 0;
            continue;
        } else if (*s == '"') {
            char * t;
            consume_string(path, &t, s + 1);
            tok = tok->next = make_token(TOK_STRING, s + 1, t, at_space, at_begin, path);
            s = t + 1;
            at_space = 0;
            at_begin = 0;
            continue;
        } else if (*s == '\'') {
            char c = unescape_quoted_char(path, &s, s + 1);
            char * tmp = mallocf("%d", c);
            tok = tok->next = make_token(TOK_NUMBER, tmp, tmp + strlen(tmp), at_space, at_begin, path);
            at_space = 0;
            at_begin = 0;
            continue;
        }
        tokenise_error(path, "unexpected character %c", *s);
    }

    tok = tok->next = make_token(TOK_EOF, NULL, NULL, 0, 1, path);
    return head.next;
}

static char * read_filez(const char * path)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1)
        perror2(path);
    int size = lseek(fd, 0, SEEK_END);
    char * buf = malloc(size + 1);
    if (!buf)
        perror2("maloc");
    lseek(fd, 0, SEEK_SET);
    read(fd, buf, size);
    close(fd);
    buf[size] = 0;
    return buf;
}

#if 0
static const char * token_kind_name[]={
    [TOK_NUMBER] = "number",
    [TOK_STRING] = "string",
    [TOK_PUNCTUATOR] = "punctuator",
    [TOK_LITERAL] = "literal",
    [TOK_EOF] = "eof",
};

static void dump_tokens(Token * t)
{
    while (t->kind != TOK_EOF) {
        printf("[%s] '%.*s'\n", token_kind_name[t->kind], t->size, t->str);
        t = t->next;
    }
}
#endif

static int tok_equal(Token * a, Token * b)
{
    return a->size == b->size && !memcmp(a->str, b->str, a->size);
}

static int tok_is_equal(Token * tok, const char * str)
{
    return strlen(str) == tok->size && !memcmp(tok->str, str, tok->size);
}

static int equal(Token * tok, const char * s)
{
    return tok->kind != TOK_STRING && tok_is_equal(tok, s);
}

static void parse_error(Token * tok, const char * fmt, ...)
{
    fprintf(stderr, "%s: ", tok->path);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fprintf(stderr, "| %.*s\n", tok->size, tok->str);
    exit(EXIT_FAILURE);
}

typedef struct Define Define;
struct Define {
    char *name;
    Token *tok;
    Define *next;
};

static Define * defines = NULL;

static void make_define(char *name, Token **rtok, Token *tok)
{
    Define * d = malloc(sizeof(Define));
    assert(d);
    d->name = name;

    if (tok->kind != TOK_EOF && !tok->at_begin) {
        d->tok = tok;
        while (tok->next->kind != TOK_EOF && !tok->next->at_begin)
            tok = tok->next;
        *rtok = tok->next;
        tok->next = NULL;
    } else {
        d->tok = NULL;
    }

    d->next = defines;
    defines = d;
}

static void make_define_static(const char *name)
{
    char *name2 = strdup(name);
    assert(name2);
    Token tok = {.kind = TOK_EOF};
    Token *rtok;
    make_define(name2, &rtok, &tok);
}

static const Define * find_define(Token *tok)
{
    for (Define * d = defines; d; d = d->next) {
        if (tok_is_equal(tok, d->name))
            return d;
}
    return NULL;
}

static int test_define(Token * tok)
{
    return !!find_define(tok);
}

static char * parse_literal(Token **rtok, Token *tok)
{
    if (tok->kind != TOK_LITERAL)
        parse_error(tok, "unexpected token");
    char *s = malloc(tok->size + 1);
    assert(s);
    memcpy(s, tok->str, tok->size);
    s[tok->size] = 0;
    *rtok = tok->next;
    return s;
}

typedef struct Stack Stack;
struct Stack {
    int value;
    int included;
    Stack * next;
};

#define push_stack(v) \
{ \
    Stack * s = malloc(sizeof(Stack)); \
    if (!s) \
        perror2("malloc"); \
    s->value = v; \
    s->included = 0; \
    s->next = stack; \
    stack = s; \
}

#define pop_stack(v) \
{ \
    Stack * s = stack; \
    v = s->value; \
    stack = s->next; \
    free(s); \
}

static Token * preprocess(Token * tok)
{
    Stack * stack = NULL;
    Token head = {.next=NULL};
    Token * cur = &head;
    int state = 1;
    while(tok->kind != TOK_EOF) {
        if (!equal(tok, "%")) {
            if (!state) {
                tok = tok->next;
                continue;
            }

            if (tok->kind == TOK_LITERAL) {
                const Define * d = find_define(tok);
                if (d) {
                    tok = tok->next;

                    if (!d->tok)
                        continue;

                    Token *last = d->tok;
                    while (last->next)
                        last = last->next;
                    last->next = tok;

                    tok = d->tok;
                    continue;
                }
            }

            cur = cur->next = tok;
            tok = tok->next;
            continue;
        }
        tok = tok->next;

        if (equal(tok, "ifdef")) {
            tok = tok->next;
            push_stack(state);
            int v;
            if (state) {
                v = test_define(tok);
                state &= v;
                tok = tok->next;
            } else
                v = 1;
            stack->included = v;
            continue;
        }

        if (equal(tok, "elifdef")) {
            tok = tok->next;
            if (stack->included)
                state = 0;
            else {
                int v;
                if (stack->value) {
                    v = test_define(tok);
                    state = stack->value && v;
                    tok = tok->next;
                } else
                    v = 1;
                stack->included = v;
            }
            continue;
        }

        if (equal(tok, "endif")) {
            if (!stack)
                parse_error(tok, "unbalanced #endif directive");
            tok = tok->next;
            pop_stack(state);
            continue;
        }

        if (equal(tok, "define")) {
            tok = tok->next;
            if (state) {
                char *name = parse_literal(&tok, tok);
                make_define(name, &tok, tok);
            }
            continue;
        }

        parse_error(tok, "unsupported preprocessor directive");
    }
    cur = cur->next = tok;
    return head.next;
}

static Token * expect(Token * tok, const char * s)
{
    if (!equal(tok, s))
        parse_error(tok, "expected %s", s);
    return tok->next;
}

static long parse_number(Token ** rtok, Token * tok)
{
    if (tok->kind != TOK_NUMBER)
        parse_error(tok, "expected number");
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%.*s", tok->size, tok->str);
    *rtok = tok->next;
    if (tmp[strlen(tmp) - 1] == 'b')
        parse_error(tok, "binary number not implemented"); //FIXME:
    if (tmp[0] == '0') {
        if (tmp[1] == 'x' || tmp[1] == 'X')
            return strtol(tmp + 2, NULL, 16);
        return strtol(tmp + 1, NULL, 7);
    }
    return strtol(tmp, NULL, 10);
}

typedef enum {
    REGISTER_EAX = 0,
    REGISTER_ECX,
    REGISTER_EDX,
    REGISTER_EBX,
    REGISTER_ESP,
    REGISTER_EBP,
    REGISTER_ESI,
    REGISTER_EDI,
    NB_REGISTER
} Register;

static const char * register_name[NB_REGISTER] = {
    [REGISTER_EAX] = "eax",
    [REGISTER_ECX] = "ecx",
    [REGISTER_EDX] = "edx",
    [REGISTER_EBX] = "ebx",
    [REGISTER_ESP] = "esp",
    [REGISTER_EBP] = "ebp",
    [REGISTER_ESI] = "esi",
    [REGISTER_EDI] = "edi",
};

typedef enum {
    OPERAND_IMM = 0,
    OPERAND_REG,
    OPERAND_ADDR,
    OPERAND_SYMBOL,
    OPERAND_ADD,
    OPERAND_SUB,
} OperandKind;

typedef struct Operand Operand;
struct Operand {
    OperandKind kind;
    union {
        unsigned long value;
        Register reg;
        Operand * addr;
        Token * symbol;
        struct {
            Operand * left, * right;
        } math;
    } u;
};

typedef struct Symbol Symbol;
struct Symbol {
    int global;
    int section_idx;
    int symbol_idx;
    off_t offset;
    char * name;
    Symbol * next;
};

static Operand * make_operand(OperandKind kind)
{
    Operand * op = malloc(sizeof(Operand));
    if (!op)
        perror2("malloc");
    op->kind = kind;
    return op;
}

static int operand_is_imm(Operand * op)
{
    return op->kind == OPERAND_IMM || op->kind == OPERAND_ADD || op->kind == OPERAND_SUB || op->kind == OPERAND_SYMBOL;
}

static Symbol * find_symbol(Symbol * symbols, Token * tok)
{
    for (Symbol * sym = symbols; sym; sym = sym->next) {
        if (tok_is_equal(tok, sym->name))
            return sym;
    }
    return NULL;
}

static int operand_eval(Operand * op, Symbol * symbols, unsigned long * value, Symbol ** symbol)
{
    if (op->kind == OPERAND_IMM) {
        *value = op->u.value;
        *symbol = NULL;
        return 1;
    } else if (op->kind == OPERAND_SYMBOL) {
        Symbol * sym = find_symbol(symbols, op->u.symbol);
        if (!sym)
            return 0;
        *value = sym->offset;
        *symbol = sym;
        return 1;
    } else if (op->kind == OPERAND_ADD) {
        unsigned long lvalue, rvalue;
        Symbol * lsymbol, * rsymbol;
        if (operand_eval(op->u.math.left, symbols, &lvalue, &lsymbol) && operand_eval(op->u.math.right, symbols, &rvalue, &rsymbol)) {
            *value = lvalue + rvalue;
            *symbol = NULL; //FIXME: lsymbol/rsymbol
            return 1;
        }
    } else if (op->kind == OPERAND_SUB) {
        unsigned long lvalue, rvalue;
        Symbol * lsymbol, * rsymbol;
        if (operand_eval(op->u.math.left, symbols, &lvalue, &lsymbol) && operand_eval(op->u.math.right, symbols, &rvalue, &rsymbol)) {
            *value = lvalue - rvalue;
            *symbol = NULL; //FIXME: lsymbol/rsymbol
            return 1;
        }
    }
    *value = 0;
    *symbol = NULL;
    return 0;
}

static Operand * parse_operand(Token ** rtok, Token * tok);

static Operand * parse_operand2(Token ** rtok, Token * tok)
{
    Operand * op;
    if (equal(tok, "[")) {
        op = make_operand(OPERAND_ADDR);
        op->u.addr = parse_operand(&tok, tok->next);
        *rtok = expect(tok, "]");
        return op;
    } else if (tok->kind == TOK_NUMBER) {
        op = make_operand(OPERAND_IMM);
        op->u.value = parse_number(rtok, tok);
        return op;
    }
    for (int i = 0; i < NB_REGISTER; i++) {
        if (equal(tok, register_name[i])) {
            op = make_operand(OPERAND_REG);
            op->u.reg = i;
            *rtok = tok->next;
            return op;
        }
    }
    if (tok->kind == TOK_LITERAL) {
        op = make_operand(OPERAND_SYMBOL);
        op->u.symbol = tok;
        *rtok = tok->next;
        return op;
    }
    parse_error(tok, "unexpected token");
    return NULL; /* never reach here */
}

static Operand * parse_operand(Token ** rtok, Token * tok)
{
    Operand * op = parse_operand2(&tok, tok);
    while (!tok->at_begin) {
        if (equal(tok, "+")) {
            Operand * op2 = make_operand(OPERAND_ADD);
            op2->u.math.left = op;
            op2->u.math.right = parse_operand2(&tok, tok->next);
            op = op2;
            continue;
        }
        if (equal(tok, "-")) {
            Operand * op2 = make_operand(OPERAND_SUB);
            op2->u.math.left = op;
            op2->u.math.right = parse_operand2(&tok, tok->next);
            op = op2;
            continue;
        }
        break;
    }
    *rtok = tok;
    return op;
}

#if 0
static void print_operand(Operand * op)
{
    if (op->kind == OPERAND_IMM) {
        printf("0x%lx", op->u.value);
    } else if (op->kind == OPERAND_REG) {
        printf("%s", register_name[op->u.reg]);
    } else if (op->kind == OPERAND_ADDR) {
        printf("["); print_operand(op->u.addr); printf("]");
    } else if (op->kind == OPERAND_SYMBOL) {
        printf("%.*s", op->u.symbol->size, op->u.symbol->str);
    } else if (op->kind == OPERAND_SUB) {
        print_operand(op->u.math.left); printf(" - "); print_operand(op->u.math.right);
    }
}
#endif

typedef struct {
    uint8_t * data;
    size_t size;
    size_t pos;
} Buffer;

static void buf_expand(Buffer * buf, size_t extra)
{
    buf->data = realloc(buf->data, buf->size + extra);
    if (!buf->data)
        perror2("realloc");
    buf->size += extra;
}

static void buf_write(Buffer * buf, void * data, size_t size)
{
    memcpy(buf->data + buf->pos, data, size);
    buf->pos += size;
}

static void buf_write1(Buffer * buf, uint8_t v)
{
    buf->data[buf->pos++] = v;
}

static void buf_write4(Buffer * buf, uint32_t v)
{
    buf_write(buf, &v, sizeof(v));
}

typedef struct Fixup Fixup;
struct Fixup {
    int section_idx;
    off_t offset;
    int size;
    Operand * op;
    int pc_relative;
    Fixup * next;
};

static void buf_write4_imm(Buffer * buf, int section_idx, Operand * op, int pc_relative, Fixup ** fixups, Symbol * symbols)
{
    unsigned long v;
    Symbol * symbol;
    if (!operand_eval(op, symbols, &v, &symbol) || symbol) {
        Fixup * f = malloc(sizeof(Fixup));
        f->section_idx = section_idx;
        f->offset = buf->pos;
        f->size = 4;
        f->op = op;
        f->pc_relative = pc_relative;
        f->next = *fixups;
        *fixups = f;
    }
    buf_write4(buf, v);
}

typedef struct Section Section;
struct Section {
    Token * name;
    Buffer buf;
    ElfSHeader sh;

    int symtab_idx;

    int nb_rel;
    ElfRel * rel;
};

static void write_elf(const char * path, Section * sections, int nb_sections, int shstrtab_idx)
{
    int fd = open(path, O_WRONLY|O_CREAT, 0666);
    if (fd == -1)
        perror2(path);

    lseek(fd, PAD16(sizeof(ElfHeader)), SEEK_SET);

    for (int i = 0; i < nb_sections; i++) {
        sections[i].sh.sh_offset = lseek(fd, 0, SEEK_CUR);
        sections[i].sh.sh_size = sections[i].buf.size;
        write(fd, sections[i].buf.data, sections[i].buf.size);
        lseek(fd, PAD16(sections[i].sh.sh_offset + sections[i].buf.size), SEEK_SET);
    }

    int shoff = lseek(fd, 0, SEEK_CUR);
    write(fd, &(ElfSHeader){0}, sizeof(ElfSHeader));
    for (int i = 0; i < nb_sections; i++) {
         write(fd, &sections[i].sh, sizeof(sections[i].sh));
    }

    lseek(fd, 0, SEEK_SET);

    ElfHeader hdr = {
#if defined(ARCH_i686)
        .e_ident = {127, 'E', 'L', 'F', 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
#elif defined(ARCH_x86_64)
        .e_ident = {127, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
#endif
        .e_type = ET_REL,
#if defined(ARCH_i686)
        .e_machine = EM_386,
#elif defined(ARCH_x86_64)
        .e_machine = EM_X86_64,
#endif
        .e_version = 1,
        .e_shoff = shoff,
        .e_shnum = nb_sections + 1,
        .e_shstrndx = shstrtab_idx + 1,
        .e_ehsize = sizeof(ElfHeader),
        .e_shentsize = sizeof(ElfSHeader),
    };
    write(fd, &hdr, sizeof(hdr));
    close(fd);
}

#if defined(ARCH_i686)
#define ASM_BITS 32
#elif defined(ARCH_x86_64)
#define ASM_BITS 64
#endif

static int add_section(Section ** sections, int * nb_sections, Token * name, int sh_type, int sh_flags, int align, int entsize)
{
    Section * new_sections;
    new_sections = realloc(*sections, (*nb_sections + 1) * sizeof(Section));
    assert(new_sections);

    int idx = *nb_sections;
    new_sections[idx] = (Section){.name=name, .buf={.data=NULL}, .sh={.sh_type=sh_type, .sh_flags=sh_flags, .sh_addralign=align, .sh_entsize=entsize}, .nb_rel=0, .rel=NULL};

    *sections = new_sections;
    (*nb_sections)++;

    return idx;
}

static int find_section(Section * sections, int nb_sections, Token * name)
{
    for (int i = 0; i < nb_sections; i++)
        if (tok_equal(sections[i].name, name))
            return i;
    return -1;
}

static char * textz = ".text";
static char * strtabz = ".strtab";
static char * symtabz = ".symtab";
static char * shstrtabz = ".shstrtab";

static void assemble(char *path, Token * tok, const char * output_path)
{
    Symbol * symbols = NULL;
    Fixup * fixups = NULL;
    Token * last_label = NULL;
    int bits = ASM_BITS;

    Section * sections = NULL;
    int nb_sections = 0;
    int current_section = add_section(&sections, &nb_sections, make_token(TOK_LITERAL, textz, textz + strlen(textz), 0, 1, ""), SHT_PROGBITS, SHF_ALLOC|SHF_EXECINSTR, 16, 0);
    Buffer * buf = &sections[current_section].buf;

    while (tok->kind != TOK_EOF) {
        if (equal(tok, "add") || equal(tok, "sub")) {
            int add = equal(tok, "add");
            Operand * op1 = parse_operand(&tok, tok->next);
            tok = expect(tok, ",");
            Operand * op2 = parse_operand(&tok, tok);

            if (op1->kind == OPERAND_REG && operand_is_imm(op2)) {
                buf_expand(buf, 3);
                buf_write1(buf, 0x83);
                buf_write1(buf, (add ? 0xc0 : 0xe8) + op1->u.reg);
                buf_write1(buf, op2->u.value); //FIXME: op2 fixup
            } else
                parse_error(tok, "unsupported operand");
        } else if (equal(tok, "align")) {
            int align = parse_number(&tok, tok->next);
            if (align > sections[current_section].sh.sh_addralign)
                sections[current_section].sh.sh_addralign = align;
            int pad = (-buf->pos) & (1-align);
            if (pad) {
                buf_expand(buf, pad);
                memset(buf->data + buf->pos, 0, pad);
                buf->pos += pad;
            }
        } else if (equal(tok, "bits")) {
            bits = parse_number(&tok, tok->next);
            if (bits != ASM_BITS)
                parse_error(tok, "%d bits unsupported", bits);
        } else if (equal(tok, "db")) {
            tok = tok->next;
            do {
                if (tok->kind == TOK_STRING) {
                    buf_expand(buf, tok->size);
                    buf_write(buf, tok->str, tok->size);
                    tok = tok->next;
                } else if (tok->kind == TOK_NUMBER) {
                    buf_expand(buf, 1);
                    buf_write1(buf, parse_number(&tok, tok));
                } else
                    parse_error(tok, "unexpected token");
                if (tok->at_begin)
                    break;
                tok = expect(tok, ",");
                if (tok->at_begin)
                    break;
            } while (1);
        } else if (equal(tok, "extern")) {
            tok = tok->next;
            if (tok->kind != TOK_LITERAL)
                parse_error(tok, "unexpected token");
            Symbol * sym = malloc(sizeof(Symbol));
            sym->name = malloc(tok->size + 1);
            assert(sym->name);
            memcpy(sym->name, tok->str, tok->size);
            sym->name[tok->size] = 0;
            sym->global = 1;
            sym->section_idx = -1;
            sym->offset = 0;
            sym->next = symbols;
            symbols = sym;
            tok = tok->next;
        } else if (equal(tok, "global")) {
            tok = tok->next;
            if (tok->kind != TOK_LITERAL)
                parse_error(tok, "unexpected token");
            tok = tok->next;
        } else if (equal(tok, "section")) {
            tok = tok->next;
            if (tok->kind != TOK_LITERAL)
                parse_error(tok, "unexpected token");
            current_section = find_section(sections, nb_sections, tok);
            if (current_section < 0)
                current_section = add_section(&sections, &nb_sections, tok, SHT_PROGBITS, SHF_WRITE|SHF_ALLOC, 16, 0);
            buf = &sections[current_section].buf;
            tok = tok->next;
        } else if ((equal(tok, "$") && tok->next && equal(tok->next->next, ":")) || (tok->kind == TOK_LITERAL && equal(tok->next, ":"))) {
            if (equal(tok, "$"))
                tok = tok->next;
            Symbol * sym = malloc(sizeof(Symbol));
            if (tok->str[0] == '.') {
                if (!last_label)
                    parse_error(tok, "no absolute symbol");
                int size = last_label->size + tok->size + 1;
                sym->name = malloc(size);
                if (!sym->name)
                    perror2("malloc");
                snprintf(sym->name, size, "%.*s%.*s", last_label->size, last_label->str, tok->size, tok->str);
            } else {
                last_label = tok;
                sym->name = malloc(tok->size + 1);
                if (!sym->name)
                    perror2("malloc");
                memcpy(sym->name, tok->str, tok->size);
                sym->name[tok->size] = 0;
            }
            sym->global = 1;
            sym->section_idx = current_section;
            sym->offset = buf->pos;
            sym->next = symbols;
            symbols = sym;

            tok = tok->next->next;
        } else if (equal(tok, "nop")) {
            buf_expand(buf, 1);
            buf_write1(buf, 0x90);
            tok = tok->next;
        } else if (equal(tok, "int3")) {
            buf_expand(buf, 1);
            buf_write1(buf, 0xcc);
            tok = tok->next;
        } else if (equal(tok, "int")) {
            buf_expand(buf, 2);
            buf_write1(buf, 0xcd);
            buf_write1(buf, parse_number(&tok, tok->next));
        } else if (equal(tok, "xor")) {
            Operand * op1 = parse_operand(&tok, tok->next);
            tok = expect(tok, ",");
            Operand * op2 = parse_operand(&tok, tok);
            if (op1->kind == OPERAND_REG && op2->kind == OPERAND_REG) {
                buf_expand(buf, 2);
                buf_write1(buf, 0x31);
                buf_write1(buf, 0xC0 + op2->u.reg * 0x8 + op1->u.reg);
            } else
                parse_error(tok, "unsupported operand");
        } else if (equal(tok, "push") || equal(tok, "pop")) {
            int is_push = equal(tok, "push");
            Operand * op = parse_operand(&tok, tok->next);
            if (op->kind == OPERAND_REG) {
                buf_expand(buf, 2);
                buf_write1(buf, (is_push ? 0x50 : 0x58) + op->u.reg);
            } else if (is_push && op->kind == OPERAND_SYMBOL) {
                buf_expand(buf, 5);
                buf_write1(buf, 0x68);
                buf_write4_imm(buf, current_section, op, 0, &fixups, symbols);
            } else
                parse_error(tok, "unsupported operand");
        } else if (equal(tok, "call")) {
            Operand * op = parse_operand(&tok, tok->next);
            if (operand_is_imm(op)) {
                buf_expand(buf, 5);
                buf_write1(buf, 0xE8);
                buf_write4_imm(buf, current_section, op, 1, &fixups, symbols);
            } else
                parse_error(tok, "unsupported operand");
        } else if (equal(tok, "ret")) {
            buf_expand(buf, 1);
            buf_write1(buf, 0xC3);
            tok = tok->next;
        } else if (equal(tok, "mov")) {
            Operand * op1 = parse_operand(&tok, tok->next);
            tok = expect(tok, ",");
            Operand * op2 = parse_operand(&tok, tok);
            if (op1->kind == OPERAND_REG && operand_is_imm(op2)) { // MOV r32,imm32
                buf_expand(buf, 5);
                buf_write1(buf, 0xB8 + op1->u.reg);
                buf_write4_imm(buf, current_section, op2, 0, &fixups, symbols);
            } else if (op1->kind == OPERAND_ADDR && op1->u.addr->kind == OPERAND_REG && op2->kind == OPERAND_REG) { //MOV r/m32,r32
                buf_expand(buf, 2);
                buf_write1(buf, 0x89);
                buf_write1(buf, op2->u.reg * 0x8 + op1->u.addr->u.reg);
            } else
                parse_error(tok, "unsupported operand");
        } else if (equal(tok, "lea")) {
            Operand * op1 = parse_operand(&tok, tok->next);
            tok = expect(tok, ",");
            Operand * op2 = parse_operand(&tok, tok);
            if (op1->kind == OPERAND_REG && op2->kind == OPERAND_ADDR && op2->u.addr->kind == OPERAND_REG) {
                buf_expand(buf, 2);
                buf_write1(buf, 0x8d);
                buf_write1(buf, op1->u.reg * 0x8 + op2->u.addr->u.reg);
            } else
                parse_error(tok, "unsupported operand");
        } else
            parse_error(tok, "unexpected token");
    }

    int strtab_idx = add_section(&sections, &nb_sections, make_token(TOK_LITERAL, strtabz, strtabz + strlen(strtabz), 0, 1, ""), SHT_STRTAB, 0, 1, 0);
    buf_expand(&sections[strtab_idx].buf, 1);
    buf_write1(&sections[strtab_idx].buf, 0);

    int file_strtab_pos = sections[strtab_idx].buf.pos;
    buf_expand(&sections[strtab_idx].buf, strlen(path) + 1);
    buf_write(&sections[strtab_idx].buf, path, strlen(path) + 1);

    int symtab_idx = add_section(&sections, &nb_sections, make_token(TOK_LITERAL, symtabz, symtabz + strlen(symtabz), 0, 1, ""), SHT_SYMTAB, 0, 4, sizeof(ElfSym));
    sections[symtab_idx].sh.sh_link = strtab_idx + 1;
    sections[symtab_idx].sh.sh_info = 1; //symbol count
    buf_expand(&sections[symtab_idx].buf, sizeof(ElfSym) * 2);
    buf_write(&sections[symtab_idx].buf, &(ElfSym){0}, sizeof(ElfSym));
    buf_write(&sections[symtab_idx].buf, &(ElfSym){.st_name=file_strtab_pos, .st_info=STB_LOCAL<<4|STT_FILE, .st_shndx=0xfff1}, sizeof(ElfSym));

    for (int i = 0; i < nb_sections; i++) {
        if (sections[i].sh.sh_type != SHT_PROGBITS)
            continue;

        sections[i].symtab_idx = sections[symtab_idx].sh.sh_info;
        buf_expand(&sections[symtab_idx].buf, sizeof(ElfSym));
        buf_write(&sections[symtab_idx].buf, &(ElfSym){.st_name=0, .st_info=(STB_LOCAL<<4)|STT_SECTION, .st_shndx=i+1}, sizeof(ElfSym));
        sections[symtab_idx].sh.sh_info++;
   }

    for (Symbol * sym = symbols; sym; sym = sym->next) {
        if (!sym->global)
            continue;

        int sym_strtab_pos = sections[strtab_idx].buf.pos;
        buf_expand(&sections[strtab_idx].buf, strlen(sym->name) + 1);
        buf_write(&sections[strtab_idx].buf, sym->name, strlen(sym->name) + 1);

        buf_expand(&sections[symtab_idx].buf, sizeof(ElfSym));
        buf_write(&sections[symtab_idx].buf, &(ElfSym){.st_name=sym_strtab_pos, .st_value=sym->offset, .st_info=(STB_GLOBAL<<4)|STT_NOTYPE, .st_shndx=sym->section_idx+1}, sizeof(ElfSym));
        sym->symbol_idx = sections[symtab_idx].sh.sh_info;
        sections[symtab_idx].sh.sh_info++;
    }

    for (Fixup * f = fixups; f; f = f->next) {
        unsigned long v;
        Symbol * symbol; //section_idx, symbol_idx;
        if (!operand_eval(f->op, symbols, &v, &symbol))
            parse_error(tok, "cannot evaluate expression");
        if (f->size == 4) {
            Section * s = &sections[f->section_idx];
            s->buf.pos = f->offset;

            if (!symbol) {
                assert(!f->pc_relative);
                buf_write4(&s->buf, v);
            } else if (symbol->section_idx == f->section_idx && f->pc_relative) { //relative, within same section
                v -= 4 + f->offset;
                buf_write4(&s->buf, v);
            } else {
                if (f->pc_relative) //relative, within local section
                    v -= 4;
                buf_write4(&s->buf, v);

                s->rel = realloc(s->rel, (s->nb_rel + 1)*sizeof(ElfRel));
                assert(s->rel);
                int rt = f->pc_relative ? R_386_PC32 : R_386_32;
                int rsymbol_idx = symbol->section_idx >= 0 ? (sections[symbol->section_idx].symtab_idx + 1) : (symbol->symbol_idx + 1);
                s->rel[s->nb_rel++] = (ElfRel){.r_offset = f->offset, .r_info=rsymbol_idx<<8|rt};
            }
        }
    }

    for (int i = 0; i < nb_sections; i++) {
       if (sections[i].nb_rel) {
           char * relname = malloc(64);
           snprintf(relname, 64, ".rel%.*s", sections[i].name->size, sections[i].name->str);
           int rel_idx = add_section(&sections, &nb_sections, make_token(TOK_LITERAL, relname, relname + strlen(relname), 0, 1, ""), SHT_REL, 0, 4, 8);
           sections[rel_idx].sh.sh_link = symtab_idx + 1;
           sections[rel_idx].sh.sh_info = i + 1;
           buf_expand(&sections[rel_idx].buf, sections[i].nb_rel*sizeof(ElfRel));
           buf_write(&sections[rel_idx].buf, sections[i].rel, sections[i].nb_rel*sizeof(ElfRel)); //FIXME:unnecessary copy
       }
    }

    int shstrtab_idx = add_section(&sections, &nb_sections, make_token(TOK_LITERAL, shstrtabz, shstrtabz + strlen(shstrtabz), 0, 1, ""), SHT_STRTAB, 0, 1, 0);
    buf_expand(&sections[shstrtab_idx].buf, 1);
    buf_write1(&sections[shstrtab_idx].buf, 0);
    for (int i = 0; i < nb_sections; i++) {
        sections[i].sh.sh_name = sections[shstrtab_idx].buf.pos;
        buf_expand(&sections[shstrtab_idx].buf, sections[i].name->size + 1);
        buf_write(&sections[shstrtab_idx].buf, sections[i].name->str, sections[i].name->size);
        buf_write1(&sections[shstrtab_idx].buf, 0);
    }

    write_elf(output_path, sections, nb_sections, shstrtab_idx);
}

static char * change_file_ext(char * s, char * ext)
{
    s = basename(strdup(s));
    char * dot = strrchr(s, '.');
    if (dot)
        *dot = 0;
    return mallocf("%s%s", s, ext);
}

int main(int argc, char ** argv)
{
    const char * o = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "ho:")) != -1) {
        switch (opt) {
        case 'o':
            o = optarg;
            break;
        case 'h':
        default:
            fprintf(stderr, "usage: %s [-o output] path\n", argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "no input file specified\n");
        return EXIT_FAILURE;
    }

    if (!o)
        o = change_file_ext(argv[optind], ".o");

#if defined(ARCH_i686)
    make_define_static("ARCH_i686");
#elif defined(ARCH_x86_64)
    make_define_static("ARCH_x86_64");
#endif

    char * s = read_filez(argv[optind]);
    Token * t = tokenise(argv[optind], s);
    t = preprocess(t);
    assemble(argv[optind], t, o);
}
