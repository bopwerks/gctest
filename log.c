#include <stdarg.h>
#include <assert.h>
#include <stdio.h>

enum { TABSTOP = 2 };
static int nindent = 0;

void
log_trace(const char *func)
{
    int i;
    assert(func);
    for (i = 0; i < nindent; ++i)
        fputc('.', stderr);
    fprintf(stderr, "Entered %s()\n", func);
    nindent += TABSTOP;
}

void
log_untrace(const char *func)
{
    int i;
    assert(func);
    nindent -= TABSTOP;
    for (i = 0; i < nindent; ++i)
        fputc('.', stderr);
    fprintf(stderr, "Exiting %s()\n", func);
}

void
log_msg(char *func, char *fmt, ...)
{
    int i;
    va_list args;
    
    assert(fmt);
    for (i = 0; i < nindent; ++i)
        fputc('.', stderr);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}
