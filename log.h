#ifndef NDEBUG

extern void log_trace  (const char *func);
extern void log_untrace(const char *func);
extern void log_msg    (const char *fmt, ...);

#define TRACE() log_trace(__func__)
#define UNTRACE() log_untrace(__func__)
#define RETURN(val) do {              \
        log_untrace(__func__);      \
        return (val);					\
    } while (0)

#define LOG(...) log_msg(__func__, __VA_ARGS__)

#else

#define TRACE()
#define UNTRACE()
#define RETURN(n) return (n)
#define LOG(...)

#endif
