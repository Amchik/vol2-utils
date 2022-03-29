#/*/../bin/env bash

NAME="lps"
CACHE_DIR="/tmp"
#CACHE_DIR="$HOME/.cache/"

if ! [[ 1 -eq 1 ]] 2>/dev/null; then
  exec bash $0 $@
fi

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )" 2>/dev/null
MAIN_FILE="$SCRIPT_DIR/$0"

if [ "$CC" = "" ]; then
  CC=cc
  # lol
  if which clang >/dev/null 2>&1; then
    CC=clang
  fi
fi

SHASUM=$(sha256sum "$MAIN_FILE" | head -c6)

TMP_FILENAME="$CACHE_DIR/__build_$NAME.$SHASUM.out"

mkdir -p "$CACHE_DIR" || exit 255

if ! [ -f "$TMP_FILENAME" ]; then
  echo -e "\e[0;1;33m[COMPILE TIME]\e[0m Build file outdated. Rebuilding..." > /dev/stderr
  $CC -o "$TMP_FILENAME" -O2 -pipe $CFLAGS -xc "$MAIN_FILE"
  if [ $? -ne 0 ]; then
    echo -e "\e[0;1;33m[COMPILE TIME]\e[0m \e[1;31mError:\e[0m $CC exited with non-zero exit code" > /dev/stderr
    exit 255
  fi
fi

if [[ "$@" == *"--compile-time%getpath"* ]]; then
  echo "$TMP_FILENAME"
  exit 0
fi
if [[ "$@" == *"--compile-time%cleanup"* ]]; then
  rm -v $CACHE_DIR/__build_$NAME.*.out
  exit 0
fi

exec "$TMP_FILENAME"

exit 0
# */ if 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define BRAND_NAME "LPS v1.0"
#define BRAND_SIZE 9

#define _CSI(l, n) "\033[" #n l

#define C_UP(n)   _CSI("A", n)
#define C_DOWN(n) _CSI("B", n)
#define C_PREV(n) _CSI("C", n)
#define C_NEXT(n) _CSI("D", n)
#define C_EIL(n)  _CSI("K", n)

#define CK_NX C_EIL(0)
#define CK_PV C_EIL(1)
#define CK_AL C_EIL(2)

#define _ESCM(a) "\033[" #a "m"
#define _ESCM2(a, b) "\033[" #a ";" #b "m"
#define _ESCM3(a, b, c) "\033[" #a ";" #b ";" #c "m"

#define T_NONE _ESCM(0)
#define T_BOLD _ESCM(1)

typedef unsigned long ulong;
typedef unsigned int  uint;
typedef unsigned char uchar;

typedef struct {
  ulong work;
  ulong total;
} cpu_t;
typedef struct {
  cpu_t parent;
  ulong nwork;
  ulong ntotal;
} cpue_t;

/* legacy binding */
#define CPU_WORK_PREV  (CPU_ALL.work)
#define CPU_TOTAL_PREV (CPU_ALL.total)

cpu_t CPU_ALL = { 0, 0 };

uint    CPU_COUNT = 0;
cpue_t *CPU_ARRAY = 0;

uchar get_cpu_work(ulong *work, ulong *total) {
  FILE *fp;
  ulong v[10];
  char *str;
  register size_t i;
  register uint   cpun;
  size_t lnn;

  fp = fopen("/proc/stat", "r");
  if (!fp) {
    return 1;
  }
  fscanf(fp, "cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
      v, v + 1, v + 2, v + 3, v + 4, v + 5, v + 6, v + 7, v + 8, v + 9);
  if (work != 0)
    *work  = v[0] + v[1] + v[2];
  if (total != 0)
    *total = v[0] + v[1] + v[2] + v[3] + v[4] + v[5] + v[6] + v[7] + v[8] + v[9];

  lnn = 0;
  while (getline(&str, &lnn, fp) != 0) {
    if (str[0] != 'c' || str[1] != 'p' || str[2] != 'u') {
      break;
    }
    
    for (i = 3; str[i] != ' ' && str[i] != 0; ++i);
    str[i] = '\0'; /* so smart, TODO: possible segmentation fault */

    cpun = atoi(str + 3);

    if (!CPU_ARRAY) {
      CPU_ARRAY = calloc(64, sizeof(cpue_t));
    } else if ((cpun + 1) >= CPU_COUNT) {
      CPU_COUNT = cpun + 1;
    }
    if (CPU_COUNT >= 64) {
      fprintf(stderr, "You have more than 64 CPUs. Please use normal system monitors like htop\n");
      raise(SIGABRT);
    }

#define atom (CPU_ARRAY[cpun])
    atom.parent.work = atom.nwork;
    atom.parent.total = atom.ntotal;

    /* x2 smart, TODO: (im-)possible segmentation fault */
    sscanf(str + i + 1, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
        v, v + 1, v + 2, v + 3, v + 4, v + 5, v + 6, v + 7, v + 8, v + 9);
    atom.nwork  = v[0] + v[1] + v[2];
    atom.ntotal = v[0] + v[1] + v[2] + v[3] + v[4] + v[5] + v[6] + v[7] + v[8] + v[9];
#undef atom
  }

  fclose(fp);

  return 0;
}

int main() {
  ulong work, total;

  printf("\033[0;7m" BRAND_NAME "\033[0m Starting up...\n\n");

  get_cpu_work(&CPU_WORK_PREV, &CPU_TOTAL_PREV);
  while (1) {
    sleep(1);

    get_cpu_work(&work, &total);

    printf(C_UP(2) C_PREV(BRAND_SIZE) CK_NX "\n");

    printf(CK_AL T_BOLD "cpu%%:" T_NONE " %2.2f", 
        (double)(work - CPU_WORK_PREV) / (total - CPU_TOTAL_PREV) * 100);
    if (CPU_COUNT) {
#define atom (CPU_ARRAY[i])
      register uint i;
      printf(" ");

      for (i = 0; i < CPU_COUNT; ++i) {
        printf("%2.2f",
            (double)(atom.nwork - atom.parent.work) / (atom.ntotal - atom.parent.total) * 100);
        if ((i + 1) < CPU_COUNT) printf("/");
      }
#undef atom
    }
    printf("\n");

    fflush(stdout);

    CPU_WORK_PREV = work;
    CPU_TOTAL_PREV = total;
  }
}
