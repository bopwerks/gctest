#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "log.h"
#include "sym.h"

#define NELEM(a) (sizeof(a)/sizeof(a[0]))

enum { MAXCELLS = 200 };

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
        struct {
            int32_t body; /* procedure body */
            int32_t env; /* environment where lambda was defined */
        } proc;
    };
    enum {
        NUMBER,
        CONS,
        SYMBOL,
        LAMBDA
    } type;
    char marked;
};
typedef struct cell_t cell_t;

static cell_t cells[MAXCELLS];
static int32_t avail = 0;
static int32_t navail = MAXCELLS;

void initcells(void);

int gc(void);
void mark(int32_t ptr);
int32_t sweep(void);
int32_t lookup(int32_t name, int32_t env, int *foundp);

int32_t getcell(void);
int32_t num(int64_t n);
int64_t val(int32_t ptr);
void printrec(int32_t ptr);
void print(int32_t ptr);
int32_t readlist(FILE *fp);
void printstats(void);
int32_t sym(char *s);

int32_t cons(int32_t a, int32_t b);
int32_t car(int32_t ptr);
int32_t cdr(int32_t ptr);
void setcar(int32_t ptr, int32_t val);
void setcdr(int32_t ptr, int32_t val);
int32_t eql(int32_t a, int32_t b);
int32_t nullp(int32_t ptr);
int32_t read(FILE *fp);
int32_t length(int32_t list);
int32_t zip(int32_t (*fn)(int32_t, int32_t), int32_t list1, int32_t list2);
int32_t mapenv(int32_t (*fn)(int32_t t, int32_t e), int32_t list, int32_t env);
int32_t eval(int32_t expr, int32_t env);
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

#define GC_PROTECT(cell) LOG("Protecting cell %d", cell); struct gc_stack_root sr_##cell = { cell, gc_roots }; gc_roots = &sr_##cell;
#define GC_UNPROTECT(c) LOG("Unprotecting cell %d", sr_##c .cell); gc_roots = sr_##c.prev

int32_t
make_proc(int32_t body, int32_t env)
{
    int32_t ptr;

    TRACE();

    GC_PROTECT(body);
    GC_PROTECT(env);
    ptr = getcell();
    GC_UNPROTECT(env);
    GC_UNPROTECT(body);
    assert(ptr != NIL);

    cells[ptr].type = LAMBDA;
    cells[ptr].proc.body = body;
    cells[ptr].proc.env = env;
    RETURN(ptr);
}

void
printmem(void)
{
    int i;
    printf("+-----------+-----------+\n");
    printf("|%11s|%11d|\n", "free head", avail);
    printf("+-----------+-----------+\n");
    printf("|%11s|%11d|\n", "free count", navail);
    printf("+=======================+\n");
    for (i = 0; i < MAXCELLS; ++i) {
        printf("|%2d|", i);
        switch (cells[i].type) {
        case LAMBDA:
            printf("%6s|%6d|%6d| ", "lambda", cells[i].proc.body, cells[i].proc.env);
//            print(i);
            break;
        case NUMBER:
            printf("%6s|%13ld| ", "number", cells[i].num);
//            print(i);
            break;
        case SYMBOL:
            printf("%6s|%13s| ", "symbol", cells[i].sym);
//            print(i);
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
//            if (cells[i].cons.car != NIL)
//                print(i);
//            else
//                putchar('\n');
            break;
        }
        if (i < MAXCELLS-1) {
            if (cells[i+1].type == CONS) {
                printf("+--+------+------+------+\n");
            } else {
                printf("+--+------+-------------+\n");
            }
        } else {
            if (cells[i].type == CONS) {
                printf("+--+------+------+------+\n");
            } else {
                printf("+--+------+-------------+\n");
            }
        }
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

int32_t
make_env(int32_t parent)
{
    return cons(NIL, parent);
}

void
add_to_env(int32_t env, int32_t name, int32_t val)
{
    setcar(env, cons(cons(name, val), car(env)));
}

int32_t env;
int
main(void)
{
    int32_t expr;
    int32_t val;
    TRACE();
    initcells();
    env = make_env(NIL);
    GC_PROTECT(env);
//    printmem();
    initread(stdin);
    while ((expr = read(stdin)) != EOF) {
//        printf("Env: ");
//        print(env);
//        printf("Read expression: ");
//        print(expr);
        GC_PROTECT(expr);
        val = eval(expr, env);
        GC_UNPROTECT(expr);
        print(val);
//        printstats();
//        printmem();
//        puts("ENV");
//        print(env);
    }
    GC_UNPROTECT(env);
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
mapenv(int32_t (*fn)(int32_t t, int32_t e), int32_t list, int32_t env)
{
    TRACE();
    assert(listp(list) == T);
    if (nullp(list) == T)
        RETURN(NIL);
    RETURN(cons(fn(car(list), env), mapenv(fn, cdr(list), env)));
}

int32_t
assoc(int32_t key, int32_t alist)
{
    TRACE();
    assert(listp(alist) == T);
    if (car(alist) == NIL) {
        return NIL;
    }
    for ( ; alist != NIL; alist = cdr(alist)) {
        if (cells[car(car(alist))].sym == cells[key].sym)
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
    TRACE();
//    printf("symcmp: s = %s\n", s);
    RETURN(strcmp(getsym(sym), s));
}

int32_t
eval(int32_t expr, int32_t env)
{
    int32_t rval;
    int32_t pair;
    int32_t proc;
    int32_t name;
    int32_t binding;
    int foundp;
    TRACE();
//    print(expr);
    if (expr == NIL || expr == T)
        RETURN(expr);
    if (symbolp(expr) == T) {
        rval = lookup(expr, env, &foundp);
        if (!foundp) {
            fprintf(stderr, "Error: Undefined symbol: %s\n", getsym(expr));
            RETURN(NIL);
        }
        RETURN(cdr(rval));
    }
    if (atomp(expr) == T)
        RETURN(expr);
    name = first(expr);
    if (symcmp(name, "env") == 0)
        RETURN(env);
    if (symcmp(name, "quote") == 0)
        RETURN(second(expr));
    if (symcmp(name, "nullp") == 0)
        RETURN(nullp(eval(second(expr), env)));
    if (symcmp(name, "atomp") == 0)
        RETURN(atomp(eval(second(expr), env)));
    if (symcmp(name, "lambda") == 0)
        RETURN(make_proc(expr, env));
    if (symcmp(name, "cons") == 0)
        RETURN(cons(eval(second(expr), env), eval(third(expr), env)));
    if (symcmp(name, "car") == 0)
        RETURN(car(eval(second(expr), env)));
    if (symcmp(name, "cdr") == 0)
        RETURN(cdr(eval(second(expr), env)));
    if (symcmp(name, "eql") == 0)
        RETURN(eql(eval(second(expr), env), eval(third(expr), env)));
    if (symcmp(name, ">") == 0)
        RETURN(bool(val(eval(second(expr), env)) > val(eval(third(expr), env))));
    if (symcmp(name, ">=") == 0)
        RETURN(bool(val(eval(second(expr), env)) >= val(eval(third(expr), env))));
    if (symcmp(name, "<") == 0)
        RETURN(bool(val(eval(second(expr), env)) < val(eval(third(expr), env))));
    if (symcmp(name, "<=") == 0)
        RETURN(bool(val(eval(second(expr), env)) <= val(eval(third(expr), env))));
    if (symcmp(name, "=") == 0)
        RETURN(bool(val(eval(second(expr), env)) == val(eval(third(expr), env))));
    if (symcmp(name, "*") == 0)
        RETURN(num(val(eval(second(expr), env)) * val(eval(third(expr), env))));
    if (symcmp(name, "+") == 0)
        RETURN(num(val(eval(second(expr), env)) + val(eval(third(expr), env))));
    if (symcmp(name, "-") == 0)
        RETURN(num(val(eval(second(expr), env)) - val(eval(third(expr), env))));
    if (symcmp(name, "or") == 0) {
        assert(cdr(expr) != NIL);
        for (pair = cdr(expr); pair != NIL; pair = cdr(pair))
            if (eval(car(pair), env) == T)
                RETURN(T);
        RETURN(NIL);
    }
    if (symcmp(name, "set!") == 0) {
        assert(cells[second(expr)].type == SYMBOL);
//        printf("set! env: ");
//        print(env);
        binding = lookup(second(expr), env, &foundp);
//        printf("set! binding: ");
//        print(binding);
        rval = eval(third(expr), env);
        if (!foundp) {
            add_to_env(env, second(expr), rval);
        } else {
            setcdr(binding, rval);
        }
        RETURN(rval);
    }
    if (symcmp(name, "and") == 0) {
        assert(cdr(expr) != NIL);
        for (pair = cdr(expr); pair != NIL; pair = cdr(pair))
            if (eval(car(pair), env) == NIL)
                RETURN(NIL);
        RETURN(T);
    }
    if (symcmp(name, "not") == 0) {
        assert(cdr(expr) != NIL);
        if (eval(second(expr), env) == NIL)
            RETURN(T);
        RETURN(NIL);
    }
    if (symcmp(name, "if") == 0) {
        assert(cdr(expr) != NIL);
        if (eval(second(expr), env) == T)
            RETURN(eval(third(expr), env));
        else
            RETURN(eval(car(cdr(cdr(cdr(expr)))), env));
    }
    if (symcmp(name, "define") == 0) {
        LOG("GOT TO THE DEFINE!");
        assert(cdr(expr) != NIL);
        proc = eval(third(expr), env);
        name = second(expr);
//        puts("NAME");
//        print(name);
//        puts("PROC");
//        print(proc);
        add_to_env(env, name, proc);
//        puts("CAR ENV");
//        print(car(env));
        RETURN(proc);
    }
//    LOG("APPLYING!!!");
//    LOG("CAR ENV");
//    print(env);
//    print(car(env));
    proc = lookup(name, env, &foundp);
    if (!foundp) {
        fprintf(stderr, "Error: Undefined function: %s\n", getsym(name));
        RETURN(NIL);
    }
//    printf("Found function for symbol %s: ", getsym(name));
//    print(proc);
    RETURN(apply(cdr(proc), cdr(expr), env));
}

int32_t
lookup(int32_t name, int32_t env, int *foundp)
{
    int32_t binding;
    TRACE();
    if (foundp) {
        *foundp = 0;
    }
    for ( ; env != NIL; env = cdr(env)) {
        binding = assoc(name, car(env));
        if (binding != NIL) {
            if (foundp) {
                *foundp = 1;
            }
            RETURN(car(binding));
        }
    }
    RETURN(NIL);
}

int32_t
apply(int32_t proc, int32_t args, int32_t env)
{
    int32_t body;
    int32_t rval;
    int32_t frame;
    int32_t expr;
    int32_t cooked_args;
    //assert(eql(length(second(lambda)), length(params)));
    /* push the values onto the environment alist as a stack */
    TRACE();
//    print(proc);
    body = cells[proc].proc.body;
    GC_PROTECT(env);
//    printf("Env: ");
//    print(env);
//    printf("Body: ");
//    print(body);
    frame = make_env(cells[proc].proc.env);
    GC_PROTECT(frame);
    GC_PROTECT(args);
//    printf("Raw args: ");
//    print(args);
    cooked_args = zip(cons, second(body), mapenv(eval, args, env));
    GC_PROTECT(cooked_args);
//    printf("Cooked args: ");
//    print(cooked_args);
    setcar(frame, cooked_args);
//    printf("Apply frame: ");
//    print(frame);
    for (expr = cdr(cdr(body)); expr != NIL; expr = cdr(expr)) {
        rval = eval(car(expr), frame);
    }
    GC_UNPROTECT(cooked_args);
    GC_UNPROTECT(args);
    GC_UNPROTECT(frame);
    GC_UNPROTECT(env);
    RETURN(rval);
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
zip(int32_t (*fn)(int32_t, int32_t), int32_t list1, int32_t list2)
{
    int32_t pair;
    int32_t rval;
    TRACE();
    if (nullp(list1) == T || nullp(list2) == T)
        RETURN(NIL);
    GC_PROTECT(list1);
    GC_PROTECT(list2);
    pair = fn(car(list1), car(list2));
    GC_PROTECT(pair);
    rval = cons(pair, zip(fn, cdr(list1), cdr(list2)));
    RETURN(rval);
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
    int32_t list;
    TRACE();
    assert(peek != EOF);
    if (peek == ')') {
        LOG("Returning NIL");
        RETURN(NIL);
    }
    /* printf("Consing "); */
    car = read(fp);
    assert(car != NIL);
    GC_PROTECT(car);
    cdr = readlist(fp);
    assert(cdr != NIL);
    GC_PROTECT(cdr);
    list = cons(car, cdr);
    assert(list != NIL);
    GC_UNPROTECT(cdr);
    GC_UNPROTECT(car);
    /* printf("Consing "); */
    /* print(car); */
    /* printf(" onto "); */
    /* print(cdr); */
    /* putchar('\n'); */
    RETURN(list);
}

int32_t
read(FILE *fp)
{
    char buf[128];
    int i, j;
    int32_t root;
    assert(fp != NULL);
    TRACE();
//    LOG("Starting on char '%c'", peek);
    while (isspace(peek))
        peek = fgetc(fp);
    if (peek == EOF) {
        LOG("Reached EOF");
        RETURN(EOF);
    }
    if (isdigit(peek)) {
        j = peek - '0';
        while ((peek = fgetc(fp)) != EOF && isdigit(peek))
            j = j*10 + (peek - '0');
        LOG("Read number %d", j);
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
    LOG("Allocating symbol '%s' in cell %d", s, ptr);
    cells[ptr].type = SYMBOL;
    cells[ptr].sym = intern(s);
//    printmem();
    RETURN(ptr);
}

int32_t
atomp(int32_t ptr)
{
    TRACE();
    RETURN((ptr == NIL ||
            ptr == T ||
            cells[ptr].type == SYMBOL ||
            cells[ptr].type == NUMBER) ? T : NIL);
}

int32_t
listp(int32_t obj)
{
    TRACE();
    if (obj == NIL)
        RETURN(T);
    if (cells[obj].type == CONS)
        RETURN(T);
    RETURN(NIL);
}

void
initcells(void)
{
    int32_t i;
    for (i = 0; i < NELEM(cells)-1; ++i) {
        cells[i].type = CONS;
        cells[i].cons.car = NIL;
        cells[i].cons.cdr = i+1;
        cells[i].marked = FALSE;
    }
    cells[i].type = CONS;
    cells[i].cons.car = NIL;
    cells[i].cons.cdr = NIL;
    cells[i].marked = FALSE;
    avail = 0;
    navail = MAXCELLS;
}

int
gc(void)
{
    struct gc_stack_root *root;
//    printf("Collecting garbage...\n");
//    printf("Env: %d\n", env);
    for (root = gc_roots; root; root = root->prev) {
//        printf("Starting from cell %d...\n", root->cell);
        mark(root->cell);
    }
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
    LOG("Allocating cons cell %d", ptr);
//    printmem();
    return ptr;
}

int32_t
num(int64_t n)
{
    int32_t ptr;
    TRACE();
    ptr = getcell();
    assert(ptr != NIL);
    LOG("Allocating number %ld at cell %d", n, ptr);
    cells[ptr].type = NUMBER;
    cells[ptr].num = n;
//    printmem();
    RETURN(ptr);
}

int32_t
car(int32_t ptr)
{
    TRACE();
    assert(cells[ptr].type == CONS);
    RETURN(cells[ptr].cons.car);
}

void
setcar(int32_t ptr, int32_t val)
{
    TRACE();
    assert(cells[ptr].type == CONS);
    cells[ptr].cons.car = val;
    UNTRACE();
}

int32_t
cdr(int32_t ptr)
{
    TRACE();
    assert(cells[ptr].type == CONS);
    RETURN(cells[ptr].cons.cdr);
}

void
setcdr(int32_t ptr, int32_t val)
{
    TRACE();
    assert(cells[ptr].type == CONS);
    cells[ptr].cons.cdr = val;
    UNTRACE();
}

int64_t
val(int32_t ptr)
{
    assert(cells[ptr].type == NUMBER);
    return cells[ptr].num;
}

void
mark(int32_t ptr)
{
    TRACE();
    if (ptr < 0 || cells[ptr].marked) {
        return;
    }
    cells[ptr].marked = TRUE;
//    printf("Marked cell %d\n", ptr);
    switch (cells[ptr].type) {
    case LAMBDA:
        mark(cells[ptr].proc.body);
        mark(cells[ptr].proc.env);
        break;
    case CONS:
        mark(car(ptr));
        mark(cdr(ptr));
        break;
    case NUMBER:
    case SYMBOL:
        break;
    }
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
//            printf("Reclaiming cell %d ", i);
//            print(i);
            cells[i].type = CONS;
            cells[i].cons.car = NIL;
            cells[i].cons.cdr = avail;
            avail = i;
        } else
            ++nmarked;
        cells[i].marked = FALSE;
    }
    navail = MAXCELLS - nmarked;
    LOG("%d cells free", navail);
    RETURN(navail);
}

char *
getsym(int32_t ptr)
{
    assert(cells[ptr].type == SYMBOL);
    return cells[ptr].sym;
}

void
printrec(int32_t ptr)
{
    TRACE();
    if (ptr == NIL) {
        printf("nil");
    } else if (ptr == T) {
        printf("t");
    } else {
        switch (cells[ptr].type) {
        case SYMBOL:
            printf("%s", cells[ptr].sym);
            break;
        case LAMBDA:
            printf("<procedure@%d/%d>", cells[ptr].proc.body, cells[ptr].proc.env);
            break;
        case NUMBER:
            printf("%ld", cells[ptr].num);
            break;
        case CONS:
            putchar('(');
            printrec(car(ptr));
            if (cdr(ptr) != NIL && cells[cdr(ptr)].type != CONS) {
                printf(" . ");
                printrec(cdr(ptr));
            } else {
                for (ptr = cdr(ptr); ptr != NIL && cells[ptr].type == CONS; ptr = cdr(ptr)) {
                    putchar(' ');
                    printrec(car(ptr));
                }
            }
            putchar(')');
            break;
        }
    }
    UNTRACE();
}

int32_t
eql(int32_t a, int32_t b)
{
    TRACE();
    if (a == b) {
        RETURN(T);
    }
    if (cells[a].type != cells[b].type) {
        RETURN(NIL);
    }
    if (cells[a].type == NUMBER) {
        if (cells[a].num == cells[b].num)
            RETURN(T);
        RETURN(NIL);
    }
    if (cells[a].type == SYMBOL) {
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
symbolp(int32_t ptr)
{
    return (ptr == NIL || ptr == T || cells[ptr].type == SYMBOL) ? T : NIL;
}
