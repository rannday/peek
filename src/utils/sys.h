#ifndef SYS_H_UTILS
#define SYS_H_UTILS

const char* detect_arch(void);
const char* detect_os_family(void);
const char* detect_os(void);
const char* detect_hostname(void);
const char* detect_user(void);
unsigned detect_physical_cpus(void);
unsigned detect_logical_cpus(void);
unsigned long long detect_total_ram_bytes(void);

#endif
