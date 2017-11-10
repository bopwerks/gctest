#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NDEBUG
#ifdef NDEBUG

static int NINDENT = 0;

#define TRACE() do {                                    \
        int i;						\
        for (i = 0; i < NINDENT; ++i)			\
            fputc('.', stderr);                         \
        fprintf(stderr, "Entered %s()\n", __func__);	\
        NINDENT += 4;					\
    } while (0)						\

#define UNTRACE() do {                                  \
        int i;						\
        NINDENT -= 4;					\
        for (i = 0; i < NINDENT; ++i)			\
            fputc('.', stderr);                         \
        fprintf(stderr, "Exiting %s()\n", __func__);	\
    } while (0)						\

#define RETURN(n) do {                                  \
        int i;						\
        NINDENT -= 4;					\
        for (i = 0; i < NINDENT; ++i)			\
            fputc('.', stderr);                         \
        fprintf(stderr, "Exiting %s()\n", __func__);	\
        return (n);					\
    } while (0)						\

#define LOG(...) do {				\
        int i;                                  \
        for (i = 0; i < NINDENT; ++i)           \
            fputc('.', stderr);                 \
        fprintf(stderr, __VA_ARGS__);           \
    } while (0)

#else

#define TRACE()
#define UNTRACE()
#define RETURN(n) return n
#define LOG(...)

#endif

#define NELEM(a) (sizeof(a)/sizeof(a[0]))

enum { MAXLEN = 20 };

enum Type { NUM, CONS, SYM };
typedef enum Type Type;

static int32_t NIL;
static int32_t T;

enum { FALSE, TRUE };

struct Cell {
    union {
        struct {
            int32_t car;
            int32_t cdr;
        } cons;
        int64_t num;
        char *sym;
    };
    Type type;
    char marked;
};
typedef struct Cell Cell;

static Cell pool[MAXLEN];
static int32_t avail = 0;
static int32_t navail = MAXLEN;

static char **symbols;
static uint32_t nsymbols;
static uint32_t maxsymbols;

void initpool(void);
int gc(void);
int32_t getcell(void);
int32_t cons(int32_t a, int32_t b);
int32_t num(int64_t n);
Type type(int32_t ptr);
int32_t car(int32_t ptr);
void setcar(int32_t ptr, int32_t val);
int32_t cdr(int32_t ptr);
void setcdr(int32_t ptr, int32_t val);
int64_t val(int32_t ptr);
void mark(int32_t ptr);
int32_t sweep(void);
void printrec(int32_t ptr);
int32_t eql(int32_t a, int32_t b);
int32_t nullp(int32_t ptr);
void print(int32_t ptr);
int32_t read(FILE *fp);
int32_t readlist(FILE *fp);
void printstats(void);
int32_t sym(char *s);
int32_t length(int32_t list);
int32_t map1(int32_t (*fn)(int32_t), int32_t list);
int32_t map2(int32_t (*fn)(int32_t, int32_t), int32_t list1, int32_t list2);
int32_t mapenv(int32_t (*fn)(int32_t t, int32_t *e), int32_t list, int32_t env);
int32_t eval(int32_t expr, int32_t *env);
int32_t apply(int32_t lambda, int32_t params, int32_t env);
int32_t listp(int32_t obj);
int32_t symbolp(int32_t obj);
char * getsym(int32_t ptr);
int32_t atomp(int32_t obj);
int32_t assoc(int32_t key, int32_t alist);
int32_t append(int32_t list1, int32_t list2);
int32_t first(int32_t list);
int32_t second(int32_t list);
int32_t third(int32_t list);

struct gc_stack_root {
    int32_t cell;
    struct gc_stack_root *prev;
};

struct gc_stack_root *gc_roots = NULL;

#define GC_PROTECT(cell) LOG("Protecting cell %d\n", cell); struct gc_stack_root sr_##cell = { cell, gc_roots }; gc_roots = &sr_##cell;
#define GC_UNPROTECT(c) LOG("Unprotecting cell %d\n", sr_##c .cell); gc_roots = sr_##c.prev

void
printmem(void)
{
    int i;
    int j;
    for (i = 0; i < MAXLEN; ++i) {
        printf("%d: ", i);
        switch (pool[i].type) {
        case NUM:
            printf("NUM: %ld\n", pool[i].num);
            break;
        case SYM:
            printf("SYM: %s\n", getsym(i));
            break;
        case CONS:
            printf("CONS: %d / %d\n", pool[i].cons.car, pool[i].cons.cdr);
            break;
        }
    }
}

static int peek = 0;

void
initread(FILE *fp)
{
    peek = fgetc(fp);
}

int
initsym(void)
{
    TRACE();
    symbols = malloc(sizeof *symbols * 1);
    if (symbols == NULL)
        RETURN(0);
    nsymbols = 0;
    maxsymbols = 1;
    RETURN(1);
}

int32_t
third(int32_t list)
{
    assert(listp(list) == T);
    assert(cdr(list) != NIL);
    assert(cdr(cdr(list)) != NIL);
    return car(cdr(cdr(list)));
}

int32_t
second(int32_t list)
{
    assert(listp(list) == T);
    assert(cdr(list) != NIL);
    return car(cdr(list));
}

int32_t
first(int32_t list)
{
    assert(listp(list) == T);
    assert(list != NIL);
    return car(list);
}

int
main(void)
{
    int32_t a;
    int32_t b;
    int32_t c;
    int32_t env;
    int32_t expr;
    TRACE();
    initpool();
    initsym();
    NIL = sym("nil");
    T = sym("t");
    env = cons(cons(NIL, NIL), cons(cons(T, T), NIL));
    GC_PROTECT(env);
    printmem();
    initread(stdin);
    while ((expr = read(stdin)) != EOF) {
        GC_PROTECT(expr);
        expr = eval(expr, &env);
        GC_UNPROTECT(expr);
        if (expr != NIL)
            print(expr);
        printstats();
        printmem();
    }
    RETURN(EXIT_SUCCESS);
}

int32_t
append(int32_t list1, int32_t list2)
{
    int32_t pair;
    TRACE();
    assert(listp(list1) == T);
    assert(listp(list2) == T);
    if (list1 == NIL)
        RETURN(list2);
    if (list2 == NIL)
        RETURN(list1);
    RETURN(cons(car(list1), append(cdr(list1), list2)));
}

int32_t
mapenv(int32_t (*fn)(int32_t t, int32_t *e), int32_t list, int32_t env)
{
    TRACE();
    assert(listp(list) == T);
    if (nullp(list) == T)
        RETURN(NIL);
    RETURN(cons(fn(car(list), &env), mapenv(fn, cdr(list), env)));
}

int32_t
assoc(int32_t key, int32_t alist)
{
    TRACE();
    assert(listp(alist) == T);
    for ( ; nullp(alist) != T; alist = cdr(alist)) {
        if (eql(car(car(alist)), key))
            RETURN(alist);
    }
    RETURN(NIL);
}

int32_t
bool(int val)
{
    if (val == 0)
        return NIL;
    return T;
}

int
symcmp(int32_t sym, char *s)
{
    return strcmp(getsym(sym), s);
}

int32_t
eval(int32_t expr, int32_t *env)
{
    int32_t rval;
    int32_t func;
    int32_t pair;
    TRACE();
    /* printf("eval env = "); */
    /* print(*env); */
    if (expr == NIL || expr == T)
        RETURN(expr);
    if (symbolp(expr) == T) {
        rval = assoc(expr, *env);
        /* printf("assoc on symbol: "); */
        /* print(rval); */
        if (rval == NIL) {
            fprintf(stderr, "Error: Undefined symbol: %s\n", getsym(expr));
            RETURN(NIL);
        }
        RETURN(cdr(car(rval)));
    }
    if (atomp(expr) == T)
        RETURN(expr);
    if (symcmp(first(expr), "env") == 0)
        RETURN(*env);
    if (symcmp(first(expr), "quote") == 0)
        RETURN(second(expr));
    if (symcmp(first(expr), "nullp") == 0)
        RETURN(nullp(eval(second(expr), env)));
    if (symcmp(first(expr), "atomp") == 0)
        RETURN(atomp(eval(second(expr), env)));
    if (symcmp(first(expr), "lambda") == 0)
        RETURN(expr);
    if (symcmp(first(expr), "cons") == 0)
        RETURN(cons(eval(second(expr), env), eval(third(expr), env)));
    if (symcmp(first(expr), "car") == 0)
        RETURN(car(eval(second(expr), env)));
    if (symcmp(first(expr), "cdr") == 0)
        RETURN(cdr(eval(second(expr), env)));
    if (symcmp(first(expr), "eql") == 0)
        RETURN(eql(eval(second(expr), env), eval(third(expr), env)));
    if (symcmp(first(expr), ">") == 0)
        RETURN(bool(val(eval(second(expr), env)) > val(eval(third(expr), env))));
    if (symcmp(first(expr), ">=") == 0)
        RETURN(bool(val(eval(second(expr), env)) >= val(eval(third(expr), env))));
    if (symcmp(first(expr), "<") == 0)
        RETURN(bool(val(eval(second(expr), env)) < val(eval(third(expr), env))));
    if (symcmp(first(expr), "<=") == 0)
        RETURN(bool(val(eval(second(expr), env)) <= val(eval(third(expr), env))));
    if (symcmp(first(expr), "=") == 0)
        RETURN(bool(val(eval(second(expr), env)) == val(eval(third(expr), env))));
    if (symcmp(first(expr), "*") == 0)
        RETURN(num(val(eval(second(expr), env)) * val(eval(third(expr), env))));
    if (symcmp(first(expr), "+") == 0)
        RETURN(num(val(eval(second(expr), env)) + val(eval(third(expr), env))));
    if (symcmp(first(expr), "-") == 0)
        RETURN(num(val(eval(second(expr), env)) - val(eval(third(expr), env))));
    if (symcmp(first(expr), "or") == 0) {
        assert(cdr(expr) != NIL);
        for (pair = cdr(expr); pair != NIL; pair = cdr(pair))
            if (eval(car(pair), env) == T)
                RETURN(T);
        RETURN(NIL);
    }
    if (symcmp(first(expr), "and") == 0) {
        assert(cdr(expr) != NIL);
        for (pair = cdr(expr); pair != NIL; pair = cdr(pair))
            if (eval(car(pair), env) == NIL)
                RETURN(NIL);
        RETURN(T);
    }
    if (symcmp(first(expr), "not") == 0) {
        assert(cdr(expr) != NIL);
        if (eval(second(expr), env) == NIL)
            RETURN(T);
        RETURN(NIL);
    }
    if (symcmp(first(expr), "if") == 0) {
        assert(cdr(expr) != NIL);
        if (eval(second(expr), env) == T)
            RETURN(eval(third(expr), env));
        else
            RETURN(eval(car(cdr(cdr(cdr(expr)))), env));
    }
    if (symcmp(first(expr), "define") == 0) {
        assert(cdr(expr) != NIL);
        *env = cons(cons(second(expr), eval(third(expr), env)), *env);
        RETURN(NIL);
    }
    func = assoc(first(expr), *env);
    if (func == NIL) {
        fprintf(stderr, "Error: Undefined function: %s\n", getsym(first(expr)));
        return NIL;
    }
    RETURN(apply(cdr(car(func)), cdr(expr), *env));
}

int32_t
apply(int32_t lambda, int32_t params, int32_t env)
{
    int32_t frame;
    int32_t pair;
    int32_t rval;
    assert(eql(length(second(lambda)), length(params)));
    /* push the values onto the environment alist as a stack */
    GC_PROTECT(env);
    env = append(map2(cons, second(lambda), mapenv(eval, params, env)), env);
    GC_UNPROTECT(env);
    /* printf("new env = "); */
    /* print(env); */
    assert(val(length(lambda)) >= 3);
    for (pair = cdr(cdr(lambda)); pair != NIL; pair = cdr(pair)) {
        /* printf("evaluating "); */
        /* print(car(pair)); */
        rval = eval(car(pair), &env);
    }
    return rval;
}

int32_t
length(int32_t list)
{
    int32_t len;
    int32_t elt;
    TRACE();
    assert(listp(list) == T);
    for (len = 0; nullp(list) != T; ++len)
        list = cdr(list);
    RETURN(num(len));
}

int32_t
map1(int32_t (*fn)(int32_t), int32_t list)
{
    int32_t elt;
    if (nullp(list) == T)
        return NIL;
    return cons(fn(car(list)), map1(fn, cdr(list)));
}

int32_t
map2(int32_t (*fn)(int32_t, int32_t), int32_t list1, int32_t list2)
{
    TRACE();
    int32_t a;
    int32_t b;
    if (nullp(list1) == T || nullp(list2) == T)
        RETURN(NIL);
    RETURN(cons(fn(car(list1), car(list2)), map2(fn, cdr(list1), cdr(list2))));
}

void
printstats(void)
{
    TRACE();
    printf("Used %d Free %d Total %d\n",
           MAXLEN-navail, navail, MAXLEN);
    UNTRACE();
}

int32_t
readlist(FILE *fp)
{
    int ch;
    int32_t car;
    int32_t cdr;
    TRACE();
    assert(peek != EOF);
    if (peek == ')') {
        LOG("Returning NIL\n");
        RETURN(NIL);
    }
    /* printf("Consing "); */
    car = read(fp);
    cdr = readlist(fp);
    /* printf("Consing "); */
    /* print(car); */
    /* printf(" onto "); */
    /* print(cdr); */
    /* putchar('\n'); */
    RETURN(cons(car, cdr));
}

int32_t
read(FILE *fp)
{
    char buf[128];
    int ch;
    int i, j;
    int32_t root;
    int32_t elt;
    assert(fp != NULL);
    TRACE();
    LOG("Starting on char '%c'\n", peek);
    while (isspace(peek))
        peek = fgetc(fp);
    if (peek == EOF) {
        LOG("Reached EOF\n");
        RETURN(EOF);
    }
    if (isdigit(peek)) {
        j = peek - '0';
        while ((peek = fgetc(fp)) != EOF && isdigit(peek))
            j = j*10 + (peek - '0');
        LOG("Read a number: %d\n", j);
        RETURN(num(j));
    }
    if (peek == '(') {
        peek = fgetc(fp);
        /* printf("Reading list\n"); */
        root = readlist(fp);
        /* printf("Returning list @ cell %d\n", root); */
        assert(peek == ')');
        peek = fgetc(fp);
        RETURN(root);
    }
    if (isgraph(peek)) {
        i = 0;
        buf[i++] = peek;
        for ( ; i < sizeof(buf)-1 && (peek = fgetc(fp)) != EOF && peek != '(' && peek != ')' && !isspace(peek); ++i)
            buf[i] = peek;
        buf[i++] = '\0';
        if (i == sizeof(buf)) {
            while (peek != EOF && isalnum(peek))
                peek = fgetc(fp);
        }
        if (strcmp(buf, "nil") == 0)
            RETURN(NIL);
        if (strcmp(buf, "t") == 0)
            RETURN(T);
        root = sym(buf);
        /* printf("Returning symbol cell %d @ '%s\n", root, buf); */
        RETURN(root);
    }
    RETURN(NIL);
}

int32_t
sym(char *s)
{
    int32_t ptr;
    TRACE();
    ptr = getcell();
    assert(ptr != -1);
    LOG("Allocating symbol cell %d @ '%s\n", ptr, s);
    pool[ptr].type = SYM;
    pool[ptr].sym = strdup(s);
    RETURN(ptr);
}

int32_t
atomp(int32_t obj)
{
    TRACE();
    if (obj == NIL)
        RETURN(T);
    if (type(obj) != CONS)
        RETURN(T);
    RETURN(NIL);
}

int32_t
listp(int32_t obj)
{
    TRACE();
    if (obj == NIL)
        RETURN(T);
    if (type(obj) == CONS)
        RETURN(T);
    RETURN(NIL);
}

void
initpool(void)
{
    int32_t i;
    for (i = 0; i < NELEM(pool)-1; ++i) {
        pool[i].type = CONS;
        pool[i].cons.car = 0;
        pool[i].cons.cdr = i+1;
        pool[i].marked = FALSE;
    }
    pool[i].type = CONS;
    pool[i].cons.car = 0;
    pool[i].cons.cdr = -1;
    pool[i].marked = FALSE;
    avail = 0;
    navail = MAXLEN;
}

int
gc(void)
{
    struct gc_stack_root *root;
    int32_t i;
    puts("Collecting garbage...");
    for (root = gc_roots; root; root = root->prev) {
        LOG("Starting from cell %d...\n", root->cell);
        mark(root->cell);
    }
    /*
      for (i = 0; i < nvars; ++i) {
      LOG("Starting from cell %d...\n", *vars[i]);
      mark(*vars[i]);
      }
    */
    return sweep();
}

int32_t
getcell(void)
{
    int32_t ptr;
    TRACE();
    if (avail == -1 && !gc()) {
        assert(0);
        RETURN(-1);
    }
    ptr = avail;
    avail = pool[ptr].cons.cdr;
    pool[ptr].cons.car = 0;
    pool[ptr].cons.cdr = 0;
    pool[ptr].marked = FALSE;
    --navail;
    RETURN(ptr);
}

int32_t
cons(int32_t a, int32_t b)
{
    int32_t ptr;
    GC_PROTECT(a);
    GC_PROTECT(b);
    ptr = getcell();
    GC_UNPROTECT(b);
    GC_UNPROTECT(a);
    assert(ptr != -1);
    pool[ptr].cons.car = a;
    pool[ptr].cons.cdr = b;
    LOG("Allocating cons cell %d\n", ptr);
    /* print(ptr); */
    return ptr;
}

int32_t
num(int64_t n)
{
    int32_t ptr;
    TRACE();
    ptr = getcell();
    assert(ptr != -1);
    LOG("Allocating number cell %d @ %ld\n", ptr, n);
    pool[ptr].type = NUM;
    pool[ptr].num = n;
    RETURN(ptr);
}

Type
type(int32_t ptr)
{
    TRACE();
    RETURN(pool[ptr].type);
}

int32_t
car(int32_t ptr)
{
    TRACE();
    assert(type(ptr) == CONS);
    RETURN(pool[ptr].cons.car);
}

void
setcar(int32_t ptr, int32_t val)
{
    TRACE();
    assert(type(ptr) == CONS);
    pool[ptr].cons.car = val;
    UNTRACE();
}

int32_t
cdr(int32_t ptr)
{
    TRACE();
    assert(type(ptr) == CONS);
    RETURN(pool[ptr].cons.cdr);
}

void
setcdr(int32_t ptr, int32_t val)
{
    TRACE();
    assert(type(ptr) == CONS);
    pool[ptr].cons.cdr = val;
    UNTRACE();
}

int64_t
val(int32_t ptr)
{
    assert(type(ptr) == NUM);
    return pool[ptr].num;
}

void
mark(int32_t ptr)
{
    TRACE();
    LOG("Visiting cell %d... ", ptr);
    if (pool[ptr].marked) {
        LOG("\n");
        return;
    }
    pool[ptr].marked = TRUE;
    LOG("MARKED\n");
    if (type(ptr) != CONS) {
        UNTRACE();
        return;
    }
    if (car(ptr) != NIL)
        mark(car(ptr));
    if (cdr(ptr) != NIL)
        mark(cdr(ptr));
    UNTRACE();
}

int32_t
sweep(void)
{
    int32_t i;
    int32_t nmarked;
    TRACE();
    avail = -1;
    LOG("Sweeping...");
    for (i = nmarked = 0; i < NELEM(pool); ++i) {
        if (!pool[i].marked) {
            LOG("Reclaiming cell %d\n", i);
            pool[i].type = CONS;
            pool[i].cons.car = NIL;
            pool[i].cons.cdr = avail;
            avail = i;
        } else
            ++nmarked;
        pool[i].marked = FALSE;
    }
    navail = MAXLEN - nmarked;
    LOG("%d cells free\n", navail);
    RETURN(navail);
}

char *
getsym(int32_t ptr)
{
    assert(type(ptr) == SYM);
    return pool[ptr].sym;
}

void
printrec(int32_t ptr)
{
    TRACE();
    if (ptr == NIL) {
        printf("nil");
        UNTRACE();
        return;
    }
    switch (type(ptr)) {
    case SYM:
        printf("%s", getsym(ptr));
        UNTRACE();
        return;
    case NUM:
        printf("%ld", val(ptr));
        UNTRACE();
        return;
    default:
        break;
    }
    putchar('(');
    printrec(car(ptr));
    if (type(cdr(ptr)) == NUM) {
        printf(" . ");
        printrec(cdr(ptr));
    }
    for (ptr = cdr(ptr); !eql(ptr, NIL) && type(ptr) == CONS; ptr = cdr(ptr)) {
        putchar(' ');
        printrec(car(ptr));
    }
    putchar(')');
    UNTRACE();
}

int32_t
eql(int32_t a, int32_t b)
{
    TRACE();
    if (a == b)
        RETURN(T);
    if (type(a) != type(b)) {
        /* puts("Returning NIL because types differ"); */
        /* printf("Type A = %d B = %d\n", type(a), type(b)); */
        RETURN(NIL);
    }
    if (type(a) == NUM) {
        if (val(a) == val(b))
            RETURN(T);
        /* puts("Returning NIL because numeric values differ"); */
        RETURN(NIL);
    }
    if (type(a) == SYM) {
        if (strcmp(getsym(a), getsym(b)) == 0)
            RETURN(T);
        /* puts("Returning NIL because symbols don't match"); */
        return NIL;
    }
    /* puts("Returning NIL because something else"); */
    RETURN(NIL);
}

int32_t
nullp(int32_t ptr)
{
    TRACE();
    if (ptr == NIL)
        RETURN(T);
    RETURN(NIL);
}

void
print(int32_t ptr)
{
    TRACE();
    printrec(ptr);
    putchar('\n');
    UNTRACE();
}

int32_t
symbolp(int32_t obj)
{
    return pool[obj].type != CONS && pool[obj].type != NUM;
}
