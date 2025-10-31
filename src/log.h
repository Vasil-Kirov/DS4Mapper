#pragma once

typedef enum {
	LOG_FATAL,
	LOG_ERROR,
	LOG_WARN,
	LOG_INFO,
	LOG_DEBUG,
} LogLevel;


#define VINFO(fmt, ...) _vlog(LOG_INFO, fmt, __VA_ARGS__)
#define VWARN(fmt, ...) _vlog(LOG_WARN, fmt, __VA_ARGS__)
#define VERRO(fmt, ...) _vlog(LOG_ERROR, fmt, __VA_ARGS__)
#define VFATL(fmt, ...) _vlog(LOG_FATAL, fmt, __VA_ARGS__)

void _vlog(LogLevel level, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
void log(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void set_iostream(struct SDL_IOStream *stream);

extern SDL_IOStream *g_log_output;
extern bool log_debug;
extern bool log_info;
extern bool log_warn;
extern bool log_error;

