/* utests.c
 * license: Unlicense (unlicense.org)
 * date: 09 JUN 2022
 * version: 1.0.0
 * 
 * if you edit this file please change date
 *   and version (eg 1.0.0-modified)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "include/utests.h"

/*
 * Allocates new utest_t
 */
__attribute__((nonnull(1, 2), malloc))
utest_t *utests_alloc(char* name, char *(*fn)(), utest_t* next) {
  utest_t *result;

  result = malloc(sizeof(utest_t));
  result->name = name;
  result->fn = fn;
  result->next = next;

  return(result);
}

char UTESTS_SIGNAL_BUFFER[512];

__attribute__((noreturn))
void utests_signal(int code) {
  UTESTS_SIGNAL_BUFFER[0] = 0;
  sprintf(UTESTS_SIGNAL_BUFFER, "Killed by signal %d (%s)",
      code, strsignal(code));

  pthread_exit(UTESTS_SIGNAL_BUFFER);
}

char* utests_test_runner(utest_t *test) {
  register char i;
  for (i = 1; i < 32; i++) {
    signal(i, utests_signal);
  }

  return(test->fn());
}

/*
 * Run tests
 */
__attribute__((nonnull(1), noreturn))
void utests_run(utest_t *chain) {
  utest_t *current, *_utn;
  unsigned int t_passed, t_failed;

  current = chain;
  t_passed = 0;
  t_failed = 0;
  while (current != 0) {
    pthread_t th;
    char *result;

    printf("* Running test '%s'\n", current->name);

    pthread_create(&th, 0, (void* (*)(void*))utests_test_runner, current);
    pthread_join(th, (void**)&result);

    if (result == 0) {
      t_passed++;
      printf("  Test passed\n");
    } else {
      t_failed++;
      printf("  Test failed: '%s'\n", result);
    }

    _utn = current->next;
    free(current);
    current = _utn;
  }

  printf("---\nTests total: %d\nPassed/failed: %d/%d (%d%%)\n", t_passed + t_failed, t_passed, t_failed, 100 * t_passed / (t_passed + t_failed));

  exit(t_failed > 254 ? 254 : t_failed);
}
