#include <log.h>
#include "SDL3/SDL_iostream.h"
#include <stdarg.h>
#include <string.h>

static const char * const log_labels[] = {
	"[FATAL] ", "[ERROR] ", "[WARNING] ", "[INFO] ", "[DEBUG] "
};

bool log_debug = true;
bool log_info = true;
bool log_warn = true;
bool log_error = true;

SDL_IOStream *g_log_output = NULL;

void set_iostream(SDL_IOStream *stream)
{
	g_log_output = stream;
}

void _vlog(LogLevel level, const char *fmt, ...)
{
	if(g_log_output == NULL) return;

	switch(level)
	{
		case LOG_DEBUG:
		{
			if(!log_debug) return;
		} break;
		case LOG_INFO:
		{
			if(!log_info) return;
		} break;
		case LOG_WARN:
		{
			if(!log_warn) return;
		} break;
		case LOG_ERROR:
		{
			if(!log_error) return;
		} break;
		case LOG_FATAL: {} break;
	}

	SDL_WriteIO(g_log_output, log_labels[level], strlen(log_labels[level]));

	va_list args;
	va_start(args, fmt);
	
	SDL_IOvprintf(g_log_output, fmt, args);
	
	va_end(args);

	SDL_WriteU8(g_log_output, '\n');
}

void log(const char *fmt, ...)
{
	if(g_log_output == NULL) return;

	va_list args;
	va_start(args, fmt);
	
	SDL_IOvprintf(g_log_output, fmt, args);
	
	va_end(args);
	SDL_WriteU8(g_log_output, '\n');
}

