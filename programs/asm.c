#include <ctype.h>
#include <elf.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define NB_ELEMS(x) (sizeof(x)/sizeof((x)[0]))

static const char * punctuator[]= { /* search algorith is greedy, so place longest punctuators first */
    "%", "[", "]", ":", ",", "+", "-",
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

static int test_define(Token * tok)
{
    char * defines[] = {
#if defined(ARCH_i686)
        "ARCH_i686",
#elif defined(ARCH_x86_64)
        "ARCH_x86_64",
#endif
    };
    for (int i = 0; i < NB_ELEMS(defines); i++) {
        if (tok_is_equal(tok, defines[i]))
            return 1;
    }
    return 0;
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

static int operand_eval(Operand * op, Symbol * symbols, unsigned long * value)
{
    if (op->kind == OPERAND_IMM) {
        *value = op->u.value;
        return 1;
    } else if (op->kind == OPERAND_SYMBOL) {
        Symbol * sym = find_symbol(symbols, op->u.symbol);
        if (!sym)
            return 0;
        *value = sym->offset;
        return 1;
    } else if (op->kind == OPERAND_ADD) {
        unsigned long lvalue, rvalue;
        if (operand_eval(op->u.math.left, symbols, &lvalue) && operand_eval(op->u.math.right, symbols, &rvalue)) {
            *value = lvalue + rvalue;
            return 1;
        }
    } else if (op->kind == OPERAND_SUB) {
        unsigned long lvalue, rvalue;
        if (operand_eval(op->u.math.left, symbols, &lvalue) && operand_eval(op->u.math.right, symbols, &rvalue)) {
            *value = lvalue - rvalue;
            return 1;
        }
    }
    *value = 0;
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
    off_t offset;
    int size;
    Operand * op;
    long adjust;
    Fixup * next;
};

static void buf_write4_imm(Buffer * buf, Operand * op, int adjust, Fixup ** fixups, Symbol * symbols)
{
    unsigned long v;
    if (!operand_eval(op, symbols, &v)) {
        Fixup * f = malloc(sizeof(Fixup));
        f->offset = buf->pos;
        f->size = 4;
        f->op = op;
        f->adjust = adjust;
        f->next = *fixups;
        *fixups = f;
    }
    buf_write4(buf, v);
}

static void write_elf(const char * path, void * text, size_t text_size)
{
#define ORG 0x1000
    int fd = open(path, O_WRONLY|O_CREAT, 0666);
    if (fd == -1)
        perror2(path);
    ElfHeader hdr = {
        .e_ident = {127, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .e_type = 2,
#if defined(ARCH_i686)
        .e_machine = EM_386,
#elif defined(ARCH_x86_64)
        .e_machine = EM_X86_64,
#endif
        .e_phoff = sizeof(hdr),
        .e_phnum = 1,
        .e_ehsize = sizeof(ElfHeader),
        .e_phentsize = sizeof(ElfPHeader),
        .e_shentsize = sizeof(ElfSHeader),
        .e_entry = ORG,
    };
    write(fd, &hdr, sizeof(hdr));
    ElfPHeader phdr = {
        .p_type = PT_LOAD,
        .p_vaddr = ORG,
        .p_memsz = text_size,
        .p_filesz = text_size,
        .p_offset = sizeof(hdr) + sizeof(phdr),
    };
    write(fd, &phdr, sizeof(phdr));
    write(fd, text, text_size);
    close(fd);
}

#if defined(ARCH_i686)
#define ASM_BITS 32
#elif defined(ARCH_x86_64)
#define ASM_BITS 64
#endif

static void assemble(Token * tok)
{
    Symbol * symbols = NULL;
    Fixup * fixups = NULL;
    Token * last_label = NULL;
    int bits = ASM_BITS;
    Buffer buf = {.data = NULL};

    while (tok->kind != TOK_EOF) {
        if (equal(tok, "bits")) {
            bits = parse_number(&tok, tok->next);
            if (bits != ASM_BITS)
                parse_error(tok, "%d bits unsupported", bits);
        } else if (equal(tok, "db")) {
            tok = tok->next;
            do {
                if (tok->kind == TOK_STRING) {
                    buf_expand(&buf, tok->size);
                    buf_write(&buf, tok->str, tok->size);
                    tok = tok->next;
                } else if (tok->kind == TOK_NUMBER) {
                    buf_expand(&buf, 1);
                    buf_write1(&buf, parse_number(&tok, tok));
                } else
                    parse_error(tok, "unexpected token");
                if (tok->at_begin)
                    break;
                tok = expect(tok, ",");
            } while (1);
        } else if (equal(tok, "global")) {
            tok = tok->next;
            if (tok->kind != TOK_LITERAL)
                parse_error(tok, "unexpected token");
            tok = tok->next;
        } else if (equal(tok, "section")) {
            tok = tok->next;
            if (tok->kind != TOK_LITERAL)
                parse_error(tok, "unexpected token");
            tok = tok->next;
        } else if (tok->kind == TOK_LITERAL && equal(tok->next, ":")) {
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
            sym->offset = ORG + buf.pos;
            sym->next = symbols;
            symbols = sym;
            tok = tok->next->next;
        } else if (equal(tok, "nop")) {
            buf_expand(&buf, 1);
            buf_write1(&buf, 0x90);
            tok = tok->next;
        } else if (equal(tok, "int")) {
            buf_expand(&buf, 2);
            buf_write1(&buf, 0xcd);
            buf_write1(&buf, parse_number(&tok, tok->next));
        } else if (equal(tok, "xor")) {
            Operand * op1 = parse_operand(&tok, tok->next);
            tok = expect(tok, ",");
            Operand * op2 = parse_operand(&tok, tok);
            if (op1->kind == OPERAND_REG && op2->kind == OPERAND_REG) {
                buf_expand(&buf, 2);
                buf_write1(&buf, 0x31);
                buf_write1(&buf, 0xC0 + op2->u.reg * 0x8 + op1->u.reg);
            } else
                parse_error(tok, "unsupported operand");
        } else if (equal(tok, "push") || equal(tok, "pop")) {
            int is_push = equal(tok, "push");
            Operand * op = parse_operand(&tok, tok->next);
            if (op->kind == OPERAND_REG) {
                buf_expand(&buf, 2);
                buf_write1(&buf, (is_push ? 0x50 : 0x58) + op->u.reg);
            } else
                parse_error(tok, "unsupported operand");
        } else if (equal(tok, "call")) {
            Operand * op = parse_operand(&tok, tok->next);
            if (operand_is_imm(op)) {
                buf_expand(&buf, 5);
                buf_write1(&buf, 0xE8);
                buf_write4_imm(&buf, op, -(ORG + buf.pos + 4), &fixups, symbols);
            } else
                parse_error(tok, "unsupported operand");
        } else if (equal(tok, "ret")) {
            buf_expand(&buf, 1);
            buf_write1(&buf, 0xC3);
            tok = tok->next;
        } else if (equal(tok, "mov")) {
            Operand * op1 = parse_operand(&tok, tok->next);
            tok = expect(tok, ",");
            Operand * op2 = parse_operand(&tok, tok);
            if (op1->kind == OPERAND_REG && operand_is_imm(op2)) { // MOV r32,imm32
                buf_expand(&buf, 5);
                buf_write1(&buf, 0xB8 + op1->u.reg);
                buf_write4_imm(&buf, op2, 0, &fixups, symbols);
            } else if (op1->kind == OPERAND_ADDR && op1->u.addr->kind == OPERAND_REG && op2->kind == OPERAND_REG) { //MOV r/m32,r32
                buf_expand(&buf, 2);
                buf_write1(&buf, 0x89);
                buf_write1(&buf, op2->u.reg * 0x8 + op1->u.addr->u.reg);
            } else
                parse_error(tok, "unsupported operand");
        } else if (equal(tok, "lea")) {
            Operand * op1 = parse_operand(&tok, tok->next);
            tok = expect(tok, ",");
            Operand * op2 = parse_operand(&tok, tok);
            if (op1->kind == OPERAND_REG && op2->kind == OPERAND_ADDR && op2->u.addr->kind == OPERAND_REG) {
                buf_expand(&buf, 2);
                buf_write1(&buf, 0x8d);
                buf_write1(&buf, op1->u.reg * 0x8 + op2->u.addr->u.reg);
            } else
                parse_error(tok, "unsupported operand");
        } else
            parse_error(tok, "unexpected token");
    }

    for (Fixup * f = fixups; f; f = f->next) {
        unsigned long v;
        if (!operand_eval(f->op, symbols, &v))
            parse_error(tok, "cannot evaluate expression");
        if (f->size == 4) {
            buf.pos = f->offset;
            buf_write4(&buf, v + f->adjust);
        }
    }

    write_elf("a.out", buf.data, buf.size);
}

int main(int argc, char ** argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s FILE\n", argv[0]);
        return EXIT_FAILURE;
    }
    char * s = read_filez(argv[1]);
    Token * t = tokenise(argv[1], s);
    t = preprocess(t);
    assemble(t);
}
