/* vim: ft=c
 *
 * utests.h
 * license: Unlicense (unlicense.org)
 * date: 08 JUN 2022
 *
 * FAIR NOTICE: FUNCTIONS FROM THIS FILE DOES NOT EXISTS
 *   IN NORMAL BUILD. DO NOT CALL IT.
 */


#ifndef __gUjjlRH5_internal_utests_h
#define __gUjjlRH5_internal_utests_h
#ifdef __cplusplus
extern "C" {
#endif

/*
 * UTest function node.
 */
typedef struct UTest {
  char* name;
  char* (*fn)();

  struct UTest *next;
} utest_t;

/*
 * Allocates new utest_t. 
 * NOT EXISTS IN NORMAL BUILD
 */
__attribute__((nonnull(1, 2), malloc))
utest_t *utests_alloc(char* name, char *(*fn)(), utest_t* next);

/*
 * Run tests.
 * NOT EXISTS IN NORMAL BUILD
 */
__attribute__((nonnull(1), noreturn))
void utests_run(utest_t *chain);

/*
 * Assert expression.
 * DO NOT USE NOT IN TEST FUNCTION
 */
#define utests_assert(expr) \
  if (!(expr)) { \
    return("Assertion failed: " #expr); \
  }

#ifdef __cplusplus
}
#endif
#endif
