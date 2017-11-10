#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "log.h"

#define NELEM(a) (sizeof(a)/sizeof(a[0]))

enum { MAXCELLS = 20 };

enum cell_type_t { NUM, CONS, SYM };
typedef enum cell_type_t cell_type_t;

static int32_t T = -1;
static int32_t NIL = -2;

enum { FALSE, TRUE };

struct cell_t {
    union {
        struct {
            int32_t car;
            int32_t cdr;
        } cons;
        int64_t num;
        char *sym;
    };
    cell_type_t type;
    char marked;
};
typedef struct cell_t cell_t;

static cell_t cells[MAXCELLS];
static int32_t avail = 0;
static int32_t navail = MAXCELLS;

void initcells(void);
int gc(void);
int32_t getcell(void);
int32_t cons(int32_t a, int32_t b);
int32_t num(int64_t n);
cell_type_t type(int32_t ptr);
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
    printf("+-----------------------+\n");
    printf("|%11s|%11d|\n", "free head", avail);
    printf("+-----------------------+\n");
    printf("|%11s|%11d|\n", "free count", navail);
    printf("+=======================+\n");
    for (i = 0; i < MAXCELLS; ++i) {
        printf("|%2d|", i);
        switch (cells[i].type) {
        case NUM:
            printf("%6s|%13ld|\n", "number", cells[i].num);
            break;
        case SYM:
            printf("%6s|%13s|\n", "symbol", cells[i].sym);
            break;
        case CONS:
            printf("%6s|", "cons");
            if (cells[i].cons.car == NIL)
                printf("%6s", "nil");
            else if (cells[i].cons.car == T)
                printf("%6s", "t");
            else
                printf("%6d", cells[i].cons.car);
            printf("|");
            if (cells[i].cons.cdr == NIL)
                printf("%6s", "nil");
            else if (cells[i].cons.cdr == T)
                printf("%6s", "t");
            else
                printf("%6d", cells[i].cons.cdr);
            printf("|\n");
            break;
        }
        printf("+-----------------------+\n");
    }
}

static int peek = 0;

void
initread(FILE *fp)
{
    peek = fgetc(fp);
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
    int32_t env;
    int32_t expr;
    int32_t val;
    TRACE();
    initcells();
    env = NIL;
    printmem();
    initread(stdin);
    while ((expr = read(stdin)) != EOF) {
        GC_PROTECT(expr);
        val = eval(expr, &env);
        GC_UNPROTECT(expr);
        print(val);
        printstats();
        printmem();
    }
    RETURN(EXIT_SUCCESS);
}

int32_t
append(int32_t list1, int32_t list2)
{
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
    if (expr == NIL || expr == T)
        RETURN(expr);
    if (symbolp(expr) == T) {
        rval = assoc(expr, *env);
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
        LOG("GOT TO THE DEFINE!");
        assert(cdr(expr) != NIL);
        *env = cons(cons(second(expr), third(expr)), *env);
        RETURN(NIL);
    }
    func = assoc(first(expr), *env);
    if (func == NIL) {
        fprintf(stderr, "Error: Undefined function: %s\n", getsym(first(expr)));
        return NIL;
    }
    LOG("APPLYING!!!");
    RETURN(apply(cdr(car(func)), cdr(expr), *env));
}

int32_t
apply(int32_t lambda, int32_t params, int32_t env)
{
    int32_t pair;
    int32_t rval;
    assert(eql(length(second(lambda)), length(params)));
    /* push the values onto the environment alist as a stack */
    GC_PROTECT(env);
    GC_PROTECT(params);
    env = append(map2(cons, second(lambda), mapenv(eval, params, env)), env);
    GC_UNPROTECT(params);
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
    TRACE();
    assert(listp(list) == T);
    for (len = 0; nullp(list) != T; ++len)
        list = cdr(list);
    RETURN(num(len));
}

int32_t
map1(int32_t (*fn)(int32_t), int32_t list)
{
    if (nullp(list) == T)
        return NIL;
    return cons(fn(car(list)), map1(fn, cdr(list)));
}

int32_t
map2(int32_t (*fn)(int32_t, int32_t), int32_t list1, int32_t list2)
{
    TRACE();
    if (nullp(list1) == T || nullp(list2) == T)
        RETURN(NIL);
    RETURN(cons(fn(car(list1), car(list2)), map2(fn, cdr(list1), cdr(list2))));
}

void
printstats(void)
{
    TRACE();
    printf("Used %d Free %d Total %d\n",
           MAXCELLS-navail, navail, MAXCELLS);
    UNTRACE();
}

int32_t
readlist(FILE *fp)
{
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
    int i, j;
    int32_t root;
    assert(fp != NULL);
    TRACE();
    LOG("Starting on char '%c'", peek);
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
    assert(ptr >= 0);
    LOG("Allocating symbol cell %d @ '%s\n", ptr, s);
    cells[ptr].type = SYM;
    cells[ptr].sym = strdup(s);
    printmem();
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
initcells(void)
{
    int32_t i;
    for (i = 0; i < NELEM(cells)-1; ++i) {
        cells[i].type = CONS;
        cells[i].cons.car = 0;
        cells[i].cons.cdr = i+1;
        cells[i].marked = FALSE;
    }
    cells[i].type = CONS;
    cells[i].cons.car = 0;
    cells[i].cons.cdr = NIL;
    cells[i].marked = FALSE;
    avail = 0;
    navail = MAXCELLS;
}

int
gc(void)
{
    struct gc_stack_root *root;
    LOG("Collecting garbage...");
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
    if (avail == NIL && !gc()) {
        assert(0);
        RETURN(-1);
    }
    ptr = avail;
    avail = cells[ptr].cons.cdr;
    cells[ptr].cons.car = 0;
    cells[ptr].cons.cdr = 0;
    cells[ptr].marked = FALSE;
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
    assert(ptr != NIL);
    cells[ptr].cons.car = a;
    cells[ptr].cons.cdr = b;
    LOG("Allocating cons cell %d\n", ptr);
    printmem();
    return ptr;
}

int32_t
num(int64_t n)
{
    int32_t ptr;
    TRACE();
    ptr = getcell();
    assert(ptr != NIL);
    LOG("Allocating number cell %d @ %ld\n", ptr, n);
    cells[ptr].type = NUM;
    cells[ptr].num = n;
    printmem();
    RETURN(ptr);
}

cell_type_t
type(int32_t ptr)
{
    TRACE();
    RETURN(cells[ptr].type);
}

int32_t
car(int32_t ptr)
{
    TRACE();
    assert(type(ptr) == CONS);
    RETURN(cells[ptr].cons.car);
}

void
setcar(int32_t ptr, int32_t val)
{
    TRACE();
    assert(type(ptr) == CONS);
    cells[ptr].cons.car = val;
    UNTRACE();
}

int32_t
cdr(int32_t ptr)
{
    TRACE();
    assert(type(ptr) == CONS);
    RETURN(cells[ptr].cons.cdr);
}

void
setcdr(int32_t ptr, int32_t val)
{
    TRACE();
    assert(type(ptr) == CONS);
    cells[ptr].cons.cdr = val;
    UNTRACE();
}

int64_t
val(int32_t ptr)
{
    assert(type(ptr) == NUM);
    return cells[ptr].num;
}

void
mark(int32_t ptr)
{
    TRACE();
    LOG("Visiting cell %d... ", ptr);
    if (cells[ptr].marked) {
        LOG("\n");
        return;
    }
    cells[ptr].marked = TRUE;
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
    avail = NIL;
    LOG("Sweeping...");
    for (i = nmarked = 0; i < NELEM(cells); ++i) {
        if (!cells[i].marked) {
            LOG("Reclaiming cell %d", i);
            cells[i].type = CONS;
            cells[i].cons.car = NIL;
            cells[i].cons.cdr = avail;
            avail = i;
        } else
            ++nmarked;
        cells[i].marked = FALSE;
    }
    navail = MAXCELLS - nmarked;
    LOG("%d cells free\n", navail);
    RETURN(navail);
}

char *
getsym(int32_t ptr)
{
    assert(type(ptr) == SYM);
    return cells[ptr].sym;
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
        RETURN(NIL);
    }
    if (type(a) == NUM) {
        if (val(a) == val(b))
            RETURN(T);
        RETURN(NIL);
    }
    if (type(a) == SYM) {
        if (strcmp(getsym(a), getsym(b)) == 0)
            RETURN(T);
        return NIL;
    }
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
    return cells[obj].type != CONS && cells[obj].type != NUM;
}
