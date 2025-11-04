#include <stdio.h>
#include <string.h>
#include "utils/time.h"
#include "utils/sys.h"

#ifndef VERSION
#define VERSION "0.1.0"  // fallback if -DVERSION= isn't set
#endif

static void usage(const char *prog) {
  printf("Usage: %s [--json] [--help] [--version]\n", prog);
}

static void json_escape(const char *in, char *out, size_t outsz) {
  size_t o = 0;
  for (size_t i = 0; in && in[i] != '\0'; i++) {
    unsigned char c = (unsigned char)in[i];
    if (c == '\"' || c == '\\') {
      if (o + 2 >= outsz) break;
      out[o++] = '\\';
      out[o++] = (char)c;
    } else if (c < 0x20) {
      if (o + 6 >= outsz) break;
      out[o++] = '\\'; out[o++] = 'u'; out[o++] = '0'; out[o++] = '0';
      const char hex[17] = "0123456789abcdef";
      out[o++] = hex[(c >> 4) & 0xF];
      out[o++] = hex[c & 0xF];
    } else {
      if (o + 1 >= outsz) break;
      out[o++] = (char)c;
    }
  }
  if (outsz) out[o < outsz ? o : outsz - 1] = '\0';
}

int main(int argc, char **argv) {
  int want_json = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--json") == 0) want_json = 1;
    else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      usage(argv[0]);
      return 0;
    } else if (strcmp(argv[i], "--version") == 0) {
      printf("peek %s\n", VERSION);
      return 0;
    } else {
      fprintf(stderr, "Unknown option: %s\n", argv[i]);
      usage(argv[0]);
      return 1;
    }
  }

  char local_buf[32];
  char utc_buf[32];
  char uptime_buf[64];
  local_time_12h(local_buf, sizeof local_buf);
  utc_time_12h(utc_buf, sizeof utc_buf);
  system_uptime(uptime_buf, sizeof uptime_buf);

  const char *arch   = detect_arch();
  const char *family = detect_os_family();
  const char *os     = detect_os();
  const char *user   = detect_user();
  const char *host   = detect_hostname();
  unsigned long long ram = detect_total_ram_bytes();
  unsigned physical = detect_physical_cpus();
  unsigned logical  = detect_logical_cpus();

  if (!want_json) {
    printf("peek - %s\n", local_buf);
    printf(" UTC Time     : %s\n", utc_buf);
    printf(" Uptime       : %s\n", uptime_buf);
    printf(" Architecture : %s\n", arch);
    printf(" OS Family    : %s\n", family);
    printf(" OS           : %s\n", os);
    printf(" Hostname     : %s\n", host);
    printf(" User         : %s\n", user);
    printf(" CPU Count    : %u\n", physical);
    printf(" CPU Threads  : %u\n", logical);
    if (ram) printf(" Total RAM    : %llu MB\n", ram / (1024ULL * 1024ULL));
    return 0;
  }

  char esc_user[256], esc_local[64], esc_utc[64], esc_uptime[64],
       esc_arch[64], esc_fam[64], esc_os[64], esc_host[256];

  json_escape(user,   esc_user, sizeof esc_user);
  json_escape(local_buf, esc_local, sizeof esc_local);
  json_escape(utc_buf,   esc_utc,   sizeof esc_utc);
  json_escape(uptime_buf, esc_uptime, sizeof esc_uptime);
  json_escape(arch,      esc_arch,  sizeof esc_arch);
  json_escape(family,    esc_fam,   sizeof esc_fam);
  json_escape(os,        esc_os,    sizeof esc_os);
  json_escape(host,      esc_host,  sizeof esc_host);

  printf("{\n");
  printf("  \"time_local\": \"%s\",\n", esc_local);
  printf("  \"time_utc\": \"%s\",\n", esc_utc);
  printf("  \"uptime\": \"%s\",\n", esc_uptime);
  printf("  \"arch\": \"%s\",\n", esc_arch);
  printf("  \"os_family\": \"%s\",\n", esc_fam);
  printf("  \"os\": \"%s\",\n", esc_os);
  printf("  \"hostname\": \"%s\",\n", esc_host);
  printf("  \"user\": \"%s\",\n", esc_user);
  printf("  \"physical_cpus\": %u,\n", physical);
  printf("  \"logical_cpus\": %u", logical);
  if (ram) printf(",\n  \"total_ram_bytes\": %llu", ram);
  printf("\n}\n");
  return 0;
}
