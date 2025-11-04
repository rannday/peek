#define _POSIX_C_SOURCE 200112L

// Ensure modern Windows APIs (Win7+) are exposed for CPU queries
#if defined(_WIN32) && !defined(_WIN32_WINNT)
  #define _WIN32_WINNT 0x0601
#endif

#include "sys.h"

#if defined(_WIN32)
  #include <windows.h>
  #include <lmcons.h>
#else
  #include <unistd.h>
  #include <stdio.h>
  #include <string.h>
  #include <pwd.h>
  #include <sys/utsname.h>
#endif

#include <stdlib.h>   // getenv, malloc, free

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
  #include <sys/types.h>
  #include <sys/sysctl.h>
#endif

const char* detect_arch(void) {
#if defined(__x86_64__) || defined(_M_X64)
  return "x86_64";
#elif defined(__i386) || defined(_M_IX86)
  return "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
  return "aarch64";
#elif defined(__arm__) || defined(_M_ARM)
  return "arm";
#elif defined(__riscv)
  return "riscv";
#else
  return "unknown";
#endif
}

const char* detect_os_family(void) {
#if defined(_WIN32)
  return "windows";
#else
  return "unix";
#endif
}

const char* detect_os(void) {
#if defined(_WIN32)
  return "windows";
#elif defined(__APPLE__)
  return "macos";
#elif defined(__linux__)
  return "linux";
#elif defined(__FreeBSD__)
  return "freebsd";
#elif defined(__NetBSD__)
  return "netbsd";
#elif defined(__OpenBSD__)
  return "openbsd";
#else
  return "unknown";
#endif
}

const char* detect_user(void) {
#if defined(_WIN32)
  const char* u = getenv("USERNAME");
  if (u && *u) return u;
  u = getenv("USER");
  if (u && *u) return u;
  static char buf[UNLEN + 1];
  DWORD n = (DWORD)sizeof buf;
  if (GetUserNameA(buf, &n) && buf[0]) return buf;
  return "unknown";
#else
  const char* u = getenv("USER");
  if (u && *u) return u;
  u = getenv("USERNAME");
  if (u && *u) return u;
  struct passwd *pw = getpwuid(getuid());
  if (pw && pw->pw_name && pw->pw_name[0]) return pw->pw_name;
  return "unknown";
#endif
}

unsigned detect_physical_cpus(void) {
#if defined(__linux__)
  FILE *f = fopen("/proc/cpuinfo", "r");
  if (!f) return 0;
  unsigned ids[256] = {0};
  unsigned count = 0;
  unsigned cores_per_socket = 0;
  char line[256];
  while (fgets(line, sizeof line, f)) {
    if (strncmp(line, "physical id", 11) == 0) {
      unsigned id = 0;
      if (sscanf(line, "physical id : %u", &id) == 1) {
        int seen = 0;
        for (unsigned i = 0; i < count; i++) if (ids[i] == id) { seen = 1; break; }
        if (!seen && count < 256) ids[count++] = id;
      }
    } else if (strncmp(line, "cpu cores", 9) == 0) {
      unsigned val = 0;
      if (sscanf(line, "cpu cores : %u", &val) == 1 && val > cores_per_socket)
        cores_per_socket = val;
    }
  }
  fclose(f);
  if (count && cores_per_socket) return count * cores_per_socket;
  if (cores_per_socket) return cores_per_socket;
  long n = sysconf(_SC_NPROCESSORS_ONLN);
  return (n > 1) ? (unsigned)(n / 2) : (unsigned)(n > 0 ? n : 0);

#elif defined(__APPLE__)
  int cores = 0;
  size_t len = sizeof cores;
  if (sysctlbyname("hw.physicalcpu", &cores, &len, NULL, 0) == 0 && cores > 0)
    return (unsigned)cores;
  return 0;

#elif defined(_WIN32)
  // Count physical cores via GetLogicalProcessorInformationEx
  typedef BOOL (WINAPI *PFN_GetLogicalProcessorInformationEx)(
      LOGICAL_PROCESSOR_RELATIONSHIP, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, PDWORD);

  HMODULE k32 = GetModuleHandleA("kernel32.dll");
  PFN_GetLogicalProcessorInformationEx pGLPIE =
      (PFN_GetLogicalProcessorInformationEx)GetProcAddress(k32, "GetLogicalProcessorInformationEx");

  if (!pGLPIE) {
    // Fallback: approximate from logical CPUs (hyperthreading typical)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    unsigned logi = si.dwNumberOfProcessors ? si.dwNumberOfProcessors : 1u;
    return (logi >= 2) ? logi / 2 : logi;
  }

  DWORD bytes = 0;
  if (pGLPIE(RelationProcessorCore, NULL, &bytes) ||
      GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    unsigned logi = si.dwNumberOfProcessors ? si.dwNumberOfProcessors : 1u;
    return (logi >= 2) ? logi / 2 : logi;
  }

  SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buf =
      (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)malloc(bytes);
  if (!buf) return 0u;

  unsigned cores = 0;
  if (pGLPIE(RelationProcessorCore, buf, &bytes)) {
    BYTE *p = (BYTE *)buf;
    BYTE *end = p + bytes;
    while (p < end) {
      SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *e =
          (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)p;
      if (e->Relationship == RelationProcessorCore) {
        // One entry per physical core (regardless of SMT)
        cores++;
      }
      p += e->Size;
    }
  }
  free(buf);
  return cores;

#else
  return 0;
#endif
}

unsigned detect_logical_cpus(void) {
#if defined(_WIN32)
  // Group-aware logical CPU count
  typedef DWORD (WINAPI *PFN_GetActiveProcessorCount)(WORD);
  HMODULE k32 = GetModuleHandleA("kernel32.dll");
  PFN_GetActiveProcessorCount pGetActiveProcessorCount =
      (PFN_GetActiveProcessorCount)GetProcAddress(k32, "GetActiveProcessorCount");

  if (pGetActiveProcessorCount) {
    DWORD n = pGetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    return n ? (unsigned)n : 0u;
  } else {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors ? (unsigned)si.dwNumberOfProcessors : 0u;
  }
#else
  long n = sysconf(_SC_NPROCESSORS_ONLN);
  return (n > 0) ? (unsigned)n : 0u;
#endif
}

const char* detect_hostname(void) {
  static char name[256];
#if defined(_WIN32)
  DWORD sz = (DWORD)sizeof name;
  if (GetComputerNameExA(ComputerNameDnsHostname, name, &sz) && name[0]) return name;
  sz = (DWORD)sizeof name;
  if (GetComputerNameA(name, &sz) && name[0]) return name;
  return "unknown";
#else
  // Prefer gethostname; declared with _POSIX_C_SOURCE 200112L
  if (gethostname(name, sizeof name) == 0 && name[0]) {
    name[sizeof name - 1] = '\0';
    return name;
  }
  // Fallback to uname, but COPY into our static buffer (don’t return stack memory)
  struct utsname u;
  if (uname(&u) == 0 && u.nodename[0]) {
    size_t i = 0;
    while (i + 1 < sizeof name && u.nodename[i]) {
      name[i] = u.nodename[i];
      i++;
    }
    name[i] = '\0';
    return name;
  }
  return "unknown";
#endif
}

unsigned long long detect_total_ram_bytes(void) {
#if defined(_WIN32)
  MEMORYSTATUSEX st;
  st.dwLength = sizeof st;
  if (GlobalMemoryStatusEx(&st)) return (unsigned long long)st.ullTotalPhys;
  return 0ULL;

#elif defined(__APPLE__)
  uint64_t mem = 0;
  size_t len = sizeof mem;
  if (sysctlbyname("hw.memsize", &mem, &len, NULL, 0) == 0) return mem;
  return 0ULL;

#elif defined(__linux__)
  long pages = sysconf(_SC_PHYS_PAGES);
  long psize = sysconf(_SC_PAGESIZE);
  if (pages > 0 && psize > 0) return (unsigned long long)pages * (unsigned long long)psize;
  return 0ULL;

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
  unsigned long mem = 0;
  size_t len = sizeof mem;
  int mib[2] = { CTL_HW, HW_PHYSMEM };
  if (sysctl(mib, 2, &mem, &len, NULL, 0) == 0) return (unsigned long long)mem;
  return 0ULL;

#else
  return 0ULL;
#endif
}
