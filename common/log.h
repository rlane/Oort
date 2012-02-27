// Copyright 2011 Rich Lane
#ifndef OORT_COMMON_LOG_H_
#define OORT_COMMON_LOG_H_

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

namespace Oort {

inline void log(const char *fmt, ...) {
#ifdef __native_client__
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	void log_handler(char *msg);
	log_handler(strdup(buf));
#else
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	puts("");
	fflush(stdout);
#endif
}

}

#endif
