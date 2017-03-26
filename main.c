#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef NDEBUG

static int NINDENT = 0;

#define TRACE() do {                            \
    int i;                                      \
    for (i = 0; i < NINDENT; ++i)               \
        fputc('.', stderr);                     \
    fprintf(stderr, "Entered %s()\n", __func__);     \
    NINDENT += 4;                               \
  } while (0)                                   \

#define UNTRACE() do {                          \
    int i;                                      \
    NINDENT -= 4;                               \
    for (i = 0; i < NINDENT; ++i)               \
      fputc('.', stderr);                            \
    fprintf(stderr, "Exiting %s()\n", __func__);     \
  } while (0)                                   \

#define RETURN(n) do {                          \
    int i;                                      \
    NINDENT -= 4;                               \
    for (i = 0; i < NINDENT; ++i)               \
        fputc('.', stderr);                     \
    fprintf(stderr, "Exiting %s()\n", __func__);     \
    return (n);                                 \
  } while (0)                                   \

#define LOG(...) do {                            \
  int i;                                        \
  for (i = 0; i < NINDENT; ++i)                 \
     fputc('.', stderr);                       \
  fprintf(stderr, __VA_ARGS__);               \
  } while (0)

#else

#define TRACE()
#define UNTRACE()
#define RETURN(n) return n
#define LOG(...)

#endif

#define NELEM(a) (sizeof(a)/sizeof(a[0]))

enum { MAXLEN = 128 };

enum Type { NUM, CONS, SYM };
typedef enum Type Type;

static int32_t NIL;
static int32_t T;

enum { FALSE, TRUE };

/* enum { NIL = -1, T = 1010101 }; */

struct Cell {
  union {
    struct {
      int32_t car;
      int32_t cdr;
    } cons;
    int64_t num;
    int64_t sym;
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
void regvar(int32_t *p);
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
int32_t lremove(int32_t num, int32_t lst);
void print(int32_t ptr);
int32_t seq(int32_t start, int32_t end);
int32_t sum(int32_t list);
int32_t filter(int32_t (*fn)(int32_t), int32_t lst);
int32_t rfilter(int32_t (*fn)(int32_t), int32_t lst);
int32_t read(FILE *fp);
int32_t readlist(FILE *fp);
void printstats(void);
int32_t evenp(int32_t obj);
int32_t sym(char *s);

enum { MAXVARS = 32 };
static int32_t *vars[MAXVARS];
static int32_t nvars = 0;

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

uint32_t
addsym(char *s)
{
  char **tmp;
  TRACE();
  if (nsymbols == maxsymbols) {
    LOG("Growing symbol list\n");
    tmp = realloc(symbols, sizeof(*symbols) * maxsymbols * 2);
    if (tmp == NULL)
      RETURN(NIL);
    symbols = tmp;
    maxsymbols *= 2;
    LOG("Symbol cap = %d count = %d\n", maxsymbols, nsymbols);
  }
  symbols[nsymbols] = malloc(strlen(s)+1);
  if (symbols[nsymbols] == NULL)
    RETURN(NIL);
  strcpy(symbols[nsymbols], s);
  RETURN(nsymbols++);
}

int
main(void)
{
  int32_t a;
  int32_t b;
  int32_t c;
  TRACE();
  initread(stdin);
  initpool();
  initsym();
  NIL = sym("nil");
  T = sym("t");
  regvar(&NIL);
  regvar(&T);
  regvar(&a);
  printstats();
  /* a = seq(num(1), num(5)); */
  /* print(a); */
  /* printstats(); */
  /* a = seq(num(1), num(2)); */
  a = read(stdin);
  print(a);
  /* for (c = 0; c < 100; ++c) { */
  /*     b = rfilter(evenp, a); */
  /*     print(b); */
  /* } */
  /* print(a); */
  /* printstats(); */
  /* a = seq(num(1), num(2)); */
  /* print(a); */
  /* printstats(); */
  /* gc(); */
  /* printstats(); */
  /* regvar(&b); */
  /* regvar(&c); */
  /* a = seq(num(1), num(500)); */
  /* print(a); */
  /* b = filter(evenp, a); */
  /* print(b); */
  /* print(a); */
  /* b = seq(num(4), num(7)); */
  /* print(b); */
  /* c = cons(a, cons(b, NIL)); */
  /* print(c); */
  /* print(lremove(num(5), b)); */
  /* print(sum(b)); */
  RETURN(EXIT_SUCCESS);
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
evenp(int32_t obj)
{
  assert(type(obj) == NUM);
  TRACE();
  RETURN(((val(obj) % 2) == 0) ? T : NIL);
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
    RETURN(NIL);
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
    printf("Reading list\n");
    root = readlist(fp);
    printf("Returning list @ cell %d\n", root);
    assert(peek == ')');
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
    root = sym(buf);
    printf("Returning symbol cell %d @ '%s\n", root, buf);
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
  pool[ptr].sym = addsym(s);
  RETURN(ptr);
}

int32_t
filter(int32_t (*fn)(int32_t), int32_t lst)
{
  TRACE();
  assert(type(lst) == CONS);
  if (nullp(lst) == T)
    RETURN(NIL);
  else if (fn(car(lst)) == T)
    RETURN(cons(car(lst), filter(fn, cdr(lst))));
  else
    RETURN(filter(fn, cdr(lst)));
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

int32_t
rfilter(int32_t (*fn)(int32_t), int32_t lst)
{
  assert(type(lst) == CONS);
  if (nullp(lst) == T)
    return NIL;
  if (atomp(car(lst)) == T) {
    if (fn(car(lst)) == T)
      return cons(car(lst), rfilter(fn, cdr(lst)));
    return rfilter(fn, cdr(lst));
  }
  return cons(rfilter(fn, car(lst)),
              rfilter(fn, cdr(lst)));
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

void
regvar(int32_t *p)
{
  vars[nvars++] = p;
}

int
gc(void)
{
  int32_t i;
  puts("Collecting garbage...");
  for (i = 0; i < nvars; ++i) {
    LOG("Starting from cell %d...\n", *vars[i]);
    mark(*vars[i]);
  }
  return sweep();
}

int32_t
getcell(void)
{
  int32_t ptr;
  if (avail == -1 && !gc())
    return -1;
  ptr = avail;
  avail = pool[ptr].cons.cdr;
  pool[ptr].cons.car = 0;
  pool[ptr].cons.cdr = 0;
  pool[ptr].marked = FALSE;
  --navail;
  return ptr;
}

int32_t
cons(int32_t a, int32_t b)
{
  int32_t ptr;
  ptr = getcell();
  assert(ptr != -1);
  pool[ptr].cons.car = a;
  pool[ptr].cons.cdr = b;
  LOG("Allocating cons cell %d\n", ptr);
  print(ptr);
  return ptr;
}

int32_t
num(int64_t n)
{
  int32_t ptr;
  TRACE();
  ptr = getcell();
  assert(ptr != -1);
  LOG("Allocating number cell %d @ %lld\n", ptr, n);
  pool[ptr].type = NUM;
  pool[ptr].num = n;
  RETURN(ptr);
}

Type
type(int32_t ptr)
{
  TRACE();
  if (ptr == NIL)
    RETURN(CONS);
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
    putchar('\n');
    return;
  }
  pool[ptr].marked = TRUE;
  LOG("MARKED\n");
  if (type(ptr) != CONS) {
    UNTRACE();
    return;
  }
  if (nullp(car(ptr)) != T)
    mark(car(ptr));
  if (nullp(cdr(ptr)) != T)
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
  return symbols[pool[ptr].sym];
}

void
printrec(int32_t ptr)
{
  TRACE();
  if (ptr == NIL) {
    printf("NIL");
    UNTRACE();
    return;
  }
  switch (type(ptr)) {
    case SYM:
      printf("%s", getsym(ptr));
      UNTRACE();
      return;
    case NUM:
      printf("%lld", val(ptr));
      UNTRACE();
      return;
    default:
      break;
  }
  /* if (ptr == NIL) { */
  /*     printf("()"); */
  /*     return; */
  /* } */
  /* if (ptr == T) { */
  /*     printf("T"); */
  /*     return; */
  /* } */
  /* if (type(ptr) == NUM) { */
  /*     return; */
  /* } */
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
  if (type(a) != type(b))
    RETURN(NIL);
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
  if (a == b)
    RETURN(T);
  RETURN(NIL);
}

int32_t
nullp(int32_t ptr)
{
  TRACE();
  if (eql(ptr, NIL))
    RETURN(T);
  RETURN(NIL);
}

int32_t
lremove(int32_t num, int32_t lst)
{
  assert(type(num) == NUM);
  assert(type(lst) == CONS);
  if (nullp(lst) == T)
    return NIL;
  else if (eql(num, car(lst)) == T)
    return lremove(num, cdr(lst));
  else
    return cons(car(lst), lremove(num, cdr(lst)));
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
seq(int32_t start, int32_t end)
{
  assert(type(start) == NUM);
  assert(type(end) == NUM);
  if (val(start) >= val(end))
    return NIL;
  return cons(start, seq(num(val(start)+1), end));
}

int32_t
sum(int32_t list)
{
  assert(type(list) == CONS);
  if (nullp(list) == T)
    return num(0);
  return num(val(car(list)) + val(sum(cdr(list))));
}
