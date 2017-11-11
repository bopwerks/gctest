#include <stdlib.h>
#include <string.h>

enum { NBUCKETS = 256 };

struct entry {
    char *s;
    struct entry *next;
};
typedef struct entry entry;

static entry *buckets[NBUCKETS];

static unsigned hash(char *s);

char *
intern(char *s)
{
    entry *e;
    unsigned h;

    h = hash(s) % NBUCKETS;
    for (e = buckets[h]; e; e = e->next) {
        if (!strcmp(e->s, s)) {
            return e->s;
        }
    }
    /* no match found. add to the bucket. */
    e = malloc(sizeof *e + strlen(s) + 1);
    if (e == NULL) {
        return NULL;
    }
    e->s = (char *) (e + 1);
    strcpy(e->s, s);
    e->next = buckets[h];
    buckets[h] = e;
    return e->s;
}

static unsigned
hash(char *s)
{
    unsigned hash;
    for (hash = 0; *s; ++s) {
        hash = (hash * 31) + *s;
    }
    return hash;
}
