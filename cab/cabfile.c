/* vim: ft=c shiftwidth=2 tabstop=2 expandtab
 *
 * CONFIGURE AND BUILD
 *
 * Build: % clang -O3 -pthread -o cabfile cabfile.c
 *
 * Usage: ./cabfile help
 * Usage: tcc -run cabfile.c help
 *
 * Script author: Amchik
 */

/***********
 * LIBRARY *
 ***********/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

/****************
 * DECLARATIONS *
 ****************/
typedef struct {
  char *name;
  char *cflags;
  char *ldflags;
} TARGET;
typedef struct {
  char *name;
  void (*callback)(char **argv);
  char *docs;
} COMMAND;


/**********
 * CONFIG *
 **********/
#define SOURCE_DIR "src"
#define OUTPUT_DIR "out"
#define OUTPUT_FMT "%s_%s.o"
#define BINARY_DIR "bin"
#define BINARY_FMT "a.out"

#define DEFAULT_CFLAGS  "-pipe"
#define DEFAULT_LDFLAGS ""

const TARGET TARGETS[] = {
  /* NAME      CFLAGS                     LDFLAGS */
  { "release", "-O3 -march=native -flto", "" },
  { "debug",   "-O0 -g",                  "" },
  { 0, 0, 0 }
};


/********************/
void cmdhelp(char **argv);
void cmdinfo(char **argv);
void cmdbuild(char **argv);

const COMMAND COMMANDS[] = {
  /* NAME    CALLBACK  DOCUMENTATION */
  { "help",  cmdhelp,  "Display help message." },
  { "info",  cmdinfo,  "Display build enviroment." },
  { "build", cmdbuild, "Build project." },
  { 0, 0, 0 }
};

/************
 * INTERNAL *
 ************/

#define _STDCFLAGS "-std="
#define _OUTFLAG "-o"
#define _OBJFLAG "-c"

#define DIRLSMAXITER 5

struct ftoc {
  char *full;
  char *nodir;
  struct ftoc *next;
};

char *APPNAME;
char *CC;
char *STD;
const TARGET *STARGET;
char FORCE_REBUILD;

struct ftoc *FTOCS;

char *FCFLAGS;
char *FLDFLAGS;

#define matcharg(arg, exp) __matcharg(arg, exp, sizeof(exp))
#define ARGM(exp) ((cpos = matcharg(argv[i] + 1, exp)) != -1)
int
__matcharg(const char *arg, const char *exp, size_t expsize) {
  register size_t i;

  for (i = 0; i < expsize - 1; i++) {
    if (arg[i] != exp[i]) return(-1);
  }
  if (arg[expsize - 1] == '=') {
    return(expsize);
  }
  if (arg[expsize - 1] != '\0') {
    return(-1);
  }

  return(0);
}

void
initflags() {
  char *env_cflags, *env_ldflags;

  env_cflags = getenv("CFLAGS");
  env_ldflags = getenv("LDFLAGS");

  FCFLAGS = malloc(sizeof(DEFAULT_CFLAGS) + sizeof(_STDCFLAGS) + strlen(STD)
      + (env_cflags != 0 ? strlen(env_cflags) : 0) + strlen(STARGET->cflags) + 3);
  sprintf(FCFLAGS, "%s %s%s %s %s", DEFAULT_CFLAGS, _STDCFLAGS, STD, STARGET->cflags, env_cflags != 0 ? env_cflags : "");

  FLDFLAGS = malloc(sizeof(DEFAULT_LDFLAGS) + (env_ldflags != 0 ? strlen(env_ldflags) : 0)
      + strlen(STARGET->ldflags) + 3);
  sprintf(FLDFLAGS, "%s %s %s", DEFAULT_LDFLAGS, STARGET->ldflags, env_ldflags != 0 ? env_ldflags : "");
}

unsigned char
initfiles(const char *_dir, unsigned char _force, int itern) {
  DIR *dir;
  struct dirent *ent;
  size_t dlen;

  if (itern >= DIRLSMAXITER) {
    fprintf(stderr, "\033[31m*ERR*\033[0m Failed to open directory '%s': Limit exceeded %d/%d\n", _dir, itern, DIRLSMAXITER);
    return(0);
  }

  dir = opendir(_dir);
  if (!dir) {
    fprintf(stderr, "\033[31m*ERR*\033[0m Failed to open directory '%s': (%d) %s\n", _dir, errno, strerror(errno));
    return(0);
  }
  dlen = strlen(_dir);

  while ((ent = readdir(dir))) {
    if (ent->d_name[0] == '.' && (ent->d_name[1] == '\0' || (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))
      continue; /* ./ or ../ */
    if (ent->d_type == 4) {
      char newdir[strlen(_dir) + strlen(ent->d_name) + 3];

      sprintf(newdir, "%s/%s", _dir, ent->d_name);
      if (!initfiles(newdir, _force, itern + 1)) {
        /* ok, and? */
      }
    } else {
      size_t flen;
      register size_t i;
      struct ftoc *node;

      flen = strlen(ent->d_name);
      if (ent->d_name[flen - 1] != 'c' || ent->d_name[flen - 2] != '.')
        continue;

      if (!FTOCS) {
        FTOCS = calloc(1, sizeof(struct ftoc));
        node = FTOCS;
      } else {
        for (node = FTOCS; 1; node = node->next) {
          if (node->next == 0) {
            node->next = calloc(1, sizeof(struct ftoc));
            node = node->next;
            break;
          }
        }
      }
      node->full = calloc(dlen + flen + 2, 1);
      sprintf(node->full, "%s/%s", _dir, ent->d_name);

      node->nodir = calloc(dlen + flen + 1 - sizeof(SOURCE_DIR), 1);
      for (i = 0; i <= (dlen + flen - sizeof(SOURCE_DIR)); i++) {
        node->nodir[i] = node->full[sizeof(SOURCE_DIR) + i] == '/' ?
          '.' : node->full[sizeof(SOURCE_DIR) + i];
      }
    }
  }

  return(1);
}

int
docommand(char *cmd) {
  int pid, status;

  pid = fork();
  if (pid == -1) {
    fprintf(stderr, "Internal: fork() failed at thread 0x%lx\nErrno: (%d) %s\n", pthread_self(), errno, strerror(errno));
    exit(127);
  } else if (pid == 0) {
    execl("/bin/sh", "sh", "-c", cmd, 0);
    exit(127);
  } else {
    do {
      if (waitpid(pid, &status, 0) == -1) {
        fprintf(stderr, "Internal: waitpid(%d) failed at thread 0x%lx\nErrno: (%d) %s\n", pid, pthread_self(), errno, strerror(errno));
        exit(127);
      }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    return(status);
  }
}
unsigned char
docompilefile(char *file, char *nodir) {
  char *cmd;
  int status;

  cmd = malloc(strlen(CC) + sizeof(_OBJFLAG) + sizeof(_OUTFLAG) + sizeof(OUTPUT_DIR) + 1 + 255 /* max out file len */ + strlen(file) + strlen(FCFLAGS) + 3);
  sprintf(cmd, "%s %s %s %s/" OUTPUT_FMT " %s %s", CC, _OBJFLAG, _OUTFLAG, OUTPUT_DIR, nodir, STARGET->name, FCFLAGS, file);

  status = docommand(cmd);

  if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
    fprintf(stderr, "\033[1;31mCompilation failed!\033[0m\n - File:    %s\n - Reason:  compiler returned %d exit status\n"
        " - Command: %s\n",
        file, WEXITSTATUS(status), cmd);
    free(cmd);
    return(0);
  } else if (WIFSIGNALED(status)) {
    fprintf(stderr, "\033[1;31mCompilation failed!\033[0m\n - File:    %s\n - Reason:  compiler killed by signal %d, %s\n"
        " - Command: %s\n",
        file, WTERMSIG(status), strsignal(WTERMSIG(status)), cmd);
    free(cmd);
    return(0);
  }
  free(cmd);

  printf("\033[32mOK\033[0m %24s -> %s/" OUTPUT_FMT "\n", file, OUTPUT_DIR, nodir, STARGET->name);

  return(1);
}

__attribute__((noreturn))
int
main(int argc, char **argv) {
  register int i, j;
  char **command;
  int cpos;

  APPNAME = argv[0];
  CC = getenv("CC");
  if (!CC) CC = "cc";
  STD = "c89";
  STARGET = &TARGETS[0];
  FORCE_REBUILD = 0;
  if (argc < 2) {
    goto usage;
  }
  command = 0;
  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (ARGM("t") || ARGM("-target")) {
        if (cpos == 0) {
          fprintf(stderr, "Usage: %s --target=<target> ...\n", APPNAME);
          exit(1);
        }
        for (j = 0; TARGETS[j].name != 0; j++) {
          if (strcmp(TARGETS[j].name, argv[i] + 1 + cpos)) continue;
          STARGET = &TARGETS[j];
          break;
        }
        if (TARGETS[j].name == 0) {
          fprintf(stderr, "Target '%s' does not exists\n", argv[i] + 1 + cpos);
          exit(1);
        }
      } else if (ARGM("-cc") || ARGM("c")) {
        if (cpos == 0) {
          fprintf(stderr, "Usage: %s --cc=<c compiler> ...\n", APPNAME);
          exit(1);
        }
        CC = argv[i] + 1 + cpos;
      } else if (ARGM("-std")) {
        if (cpos == 0) {
          fprintf(stderr, "Usage: %s --std=<standart> ...\n", APPNAME);
          exit(1);
        }
        STD = argv[i] + 1 + cpos;
      } else if (ARGM("V")) {
        puts("0.1.0");
        puts("cabfile");
        exit(0);
      } else if (ARGM("f") || ARGM("-force")) {
        FORCE_REBUILD = 1;
      } else {
        goto usage;
      }
    } else if (command == 0) {
      command = argv + i;
    }
  }

  initflags();
  if (!initfiles(SOURCE_DIR, 0, 0))
    exit(1);

  for (i = 0; COMMANDS[i].name != 0; i++) {
    if (strcmp(COMMANDS[i].name, command[0])) continue;
    COMMANDS[i].callback(command + 1);
    exit(0);
  }

usage:
  fprintf(stderr, "Usage: %s <command> ...\n       %s help\n", APPNAME, APPNAME);
  exit(1);
}

void
cmdhelp(char **argv __attribute__((unused))) {
  register int i;

  printf("cabfile version 0.1\n\nList of commands:\n");
  for (i = 0; COMMANDS[i].name != 0; i++) {
    printf(" * \033[1m%s\033[0m %s\n", COMMANDS[i].name, COMMANDS[i].docs);
  }
  printf("\nList of targets:\n");
  for (i = 0; TARGETS[i].name != 0; i++) {
    printf(" * \033[1;32m%s\033[0m\n", TARGETS[i].name);
  }
  printf("\nCommand line options:\n"
      " * \033[1m--target=<target>\033[0m  Sets target\n"
      " * \033[1m--cc=<c compiler>\033[0m  Sets $CC\n"
      " * \033[1m--std=<standart>\033[0m   Sets C standart\n"
      "\n"
      "Example:\n"
      " %% %s build\n"
      , APPNAME);
}
void
cmdinfo(char **argv __attribute__((unused))) {
  printf(
      "\033[35m%s\033[0m:\n"
      " - bin: " BINARY_DIR "\n"
      " - obj: " OUTPUT_DIR " (fmt: " OUTPUT_FMT ")\n"
      "Selected target \033[32m%s\033[0m:\n"
      " CFLAGS:  %s\n"
      " LDFLAGS: %s\n"
      "Selected standart \033[32m%s\033[0m via \033[32m%s\033[0m\n"
      "\n"
      "Final CFLAGS:  %s\n"
      "Final LDFLAGS: %s\n"
      ,
      BINARY_FMT, "<filename>", "<target>", STARGET->name, STARGET->cflags,
      STARGET->ldflags, STD, CC, FCFLAGS, FLDFLAGS);
}
void
cmdbuild(char **argv __attribute__((unused))) {
  struct ftoc *cur;
  char *cmd, *files;
  size_t files_len;
  int status, skipped;

  skipped = 0;
  files_len = 1;
  files = malloc(files_len);
  files[0] = '\0';
  for (cur = FTOCS; cur != 0; cur = cur->next) {
    size_t file_len = sizeof(OUTPUT_DIR) + strlen(cur->nodir) + 102;
    char file[file_len];
    struct stat src_stat, file_stat;

    sprintf(file, "%s/" OUTPUT_FMT, OUTPUT_DIR, cur->nodir, STARGET->name);
    
    if (!FORCE_REBUILD && -1 == stat(cur->full, &src_stat)) {
      fprintf(stderr, "\033[1;31mCompilation failed!\033[0m\n - File:    %s\n - Reason:  failed to stat file.\n"
          " - Errno:   (#%d) %s\n",
          file, errno, strerror(errno));
      return;
    }
    if (!FORCE_REBUILD
        && -1 != stat(file, &file_stat)
        && file_stat.st_mtim.tv_sec > src_stat.st_mtim.tv_sec) {
      /* skip... */
      skipped += 1;
    }
    else {
      if (1 != docompilefile(cur->full, cur->nodir)) {
        exit(1);
      }
    }

    files_len += sizeof(OUTPUT_DIR) + strlen(cur->nodir) + 102;
    files = realloc(files, files_len);
    sprintf(files, "%s %s/" OUTPUT_FMT, files, OUTPUT_DIR, cur->nodir, STARGET->name);/* looks like sigsegv */
  }

  cmd = malloc(strlen(CC) + sizeof(_OUTFLAG) + sizeof(BINARY_DIR) + sizeof(BINARY_FMT) + files_len + strlen(FLDFLAGS) + 5);
  sprintf(cmd, "%s %s %s/%s%s %s", CC, _OUTFLAG, BINARY_DIR, BINARY_FMT, files, FLDFLAGS);
  free(files);

  if (skipped) {
    printf("\033[0;35mSKIPPED\033[0;1m %d\033[0m files skipped. Run cabfile with \033[1m--force\033[0m argument for recompile it.\n", skipped);
  }

  status = docommand(cmd);

  if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
    fprintf(stderr, "\033[1;31mLinking failed!\033[0m\n - File:    %s\n - Reason:  linker returned %d exit status\n"
        " - Command: %s\n",
        BINARY_DIR "/" BINARY_FMT, WEXITSTATUS(status), cmd);
    free(cmd);
    exit(1);
  } else if (WIFSIGNALED(status)) {
    fprintf(stderr, "\033[1;31mLinking failed!\033[0m\n - File:    %s\n - Reason:  linker killed by signal %d, %s\n"
        " - Command: %s\n",
        BINARY_DIR "/" BINARY_FMT, WTERMSIG(status), strsignal(WTERMSIG(status)), cmd);
    free(cmd);
    exit(1);
  }
  free(cmd);

  printf("\033[32mDONE\033[0m \033[35m%s/%s\033[0m (target \033[32m%s\033[0m)\n", BINARY_DIR, BINARY_FMT, STARGET->name);
}

