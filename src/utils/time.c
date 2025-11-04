#define _XOPEN_SOURCE 700
#include "time.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

#if defined(_WIN32)
  #include <windows.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
  #include <sys/types.h>
  #include <sys/sysctl.h>
  #include <sys/time.h>
#else
  #include <unistd.h>
  #include <sys/sysinfo.h>
  #if defined(CLOCK_BOOTTIME)
    #include <time.h>
  #endif
#endif

static void fmt_12h(const struct tm *tm, char *out, size_t outsz) {
  int hour24 = tm->tm_hour;
  int hour12 = hour24 % 12;
  if (hour12 == 0) hour12 = 12;
  const char *ampm = (hour24 >= 12) ? "PM" : "AM";
  /* hour with no leading zero by design; minutes/seconds zero-padded */
  snprintf(out, outsz, "%d:%02d:%02d %s", hour12, tm->tm_min, tm->tm_sec, ampm);
}

void local_time_12h(char *out, size_t outsz) {
  time_t t = time(NULL);
#if defined(_WIN32)
  struct tm tm_local;
  localtime_s(&tm_local, &t);
  fmt_12h(&tm_local, out, outsz);
#else
  struct tm tm_local;
  localtime_r(&t, &tm_local);
  fmt_12h(&tm_local, out, outsz);
#endif
}

void utc_time_12h(char *out, size_t outsz) {
  time_t t = time(NULL);
#if defined(_WIN32)
  struct tm tm_utc;
  gmtime_s(&tm_utc, &t);
  fmt_12h(&tm_utc, out, outsz);
#else
  struct tm tm_utc;
  gmtime_r(&t, &tm_utc);
  fmt_12h(&tm_utc, out, outsz);
#endif
}

void system_uptime(char *out, size_t outsz) {
  unsigned long long seconds = 0;

#if defined(_WIN32)
  ULONGLONG ms = GetTickCount64();
  seconds = ms / 1000ULL;

#elif defined(__linux__)
  /* Prefer sysinfo; if unavailable, fallback to CLOCK_BOOTTIME when present */
  struct sysinfo info;
  if (sysinfo(&info) == 0) {
    seconds = (unsigned long long)info.uptime;
  }
  #if defined(CLOCK_BOOTTIME)
  if (seconds == 0) {
    struct timespec ts;
    if (clock_gettime(CLOCK_BOOTTIME, &ts) == 0) {
      seconds = (unsigned long long)ts.tv_sec;
    }
  }
  #endif

#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
  /* Use kern.boottime */
  struct timeval boottv;
  size_t len = sizeof(boottv);
  int mib[2] = { CTL_KERN, KERN_BOOTTIME };
  if (sysctl(mib, 2, &boottv, &len, NULL, 0) == 0 && boottv.tv_sec > 0) {
    time_t now = time(NULL);
    if (now > boottv.tv_sec) {
      seconds = (unsigned long long)(now - (time_t)boottv.tv_sec);
    }
  }
#endif

  /* Format human-readable */
  unsigned days  = (unsigned)(seconds / 86400ULL);
  unsigned hours = (unsigned)((seconds % 86400ULL) / 3600ULL);
  unsigned mins  = (unsigned)((seconds % 3600ULL) / 60ULL);
  unsigned secs  = (unsigned)(seconds % 60ULL);

  if (seconds == 0) {
    /* Unknown or failed detection */
    snprintf(out, outsz, "unknown");
    return;
  }

  if (days > 0)
    snprintf(out, outsz, "%u days, %u:%u:%u", days, hours, mins, secs);
  else
    snprintf(out, outsz, "%u:%u:%u", hours, mins, secs);
}
