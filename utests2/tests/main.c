#include "include/utests.h"

char* passed_test() {
  utests_assert(2 < 3);

  return(0);
}
char* failed_test() {
  utests_assert(2 + 2 == 5)

  return(0);
}
char* sigfaulted_test() {
  int *p = 0;

  utests_assert(*p == 255);
  return(0);
}

int utests() {
  utest_t *chain;

  chain = utests_alloc("Test 2 < 3", passed_test,
      utests_alloc("Test 2 + 2 = 5", failed_test,
        utests_alloc("Test *0 == 255", sigfaulted_test, 0)));

  utests_run(chain);
}
