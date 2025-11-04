#ifndef TIME_H_UTILS
#define TIME_H_UTILS

#include <stddef.h>  // for size_t

void local_time_12h(char *out, size_t outsz);
void utc_time_12h(char *out, size_t outsz);
void system_uptime(char *out, size_t outsz);

#endif
