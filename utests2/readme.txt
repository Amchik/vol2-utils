utests2
=======
A C project template with Unit-Testing framework.

1. Usage
========
Template structure is:
 * /
  * src/             Source code
   * include/         .h files
  * tests/           Unit tests
   * include/         Symlink to ../src/include
   * utests.c         Unit-Tests implementation
  * bin/             Executable files
   * ${TARGET}        Target directory (see makefile)
    * appname          Final application
    * appname.tests    Testing executable
  * obj/             Object files (.o)
   * ${TARGET}        Object files for ${TARGET}
   * ${TARGET}.tests  Patched object files for tests
  * makefile         Makefile

2. Makefile
===========
This template uses Amchik/plant.git (may not released) makefile.

Default makefile has some receipts:
  * all      Build final application and test executable
  * release  Build in release mode (= TARGET=RELEASE make all)
  * debug    Build in debug mode (= TARGET=DEBUG make all)
  * info     Displays info about target
  * clean    Cleans obj/
  * check    Runs tests

Default target is DEBUG, only final application (no tests).

To use another target run TARGET=my_target make...

2.1. Changing application name
------------------------------
Set NAME variable to own value. (near 37th line)

2.2. Creating new target
------------------------
Add to makefile lines (mytarget is a target name):

   |[makefile]
85 | _mytarget_CFLAGS=-O3
86 | _mytarget_CFLAGS_DEFINES=-D_DEFAULT_SOURCE
87 | _mytarget_LDFLAGS=-lncursesw
88 | _mytarget_CC=my-cool-clang
89 | _mytarget_STD=gnu99

Now check it: TARGET=mytarget make info

3. Writing unit-tests
=====================
This template have very very simple unit-tests
framework: utests2.

Create file tests/main.c and:

   |[file: tests/main.c]
 1 | #include <stdio.h>
 2 |
 3 | #include "include/utests.h"
 4 |
 5 | char* test_two_plus_two() {
 6 |   /* if test function return 0 (NULL) 
 7 |      test will be passed, otherwise it
 8 |      will be failed with returned reason */
 9 |   char _result = (2 + 2) == 4;
10 |   if (_result == 0 /* false */) {
11 |     return("Expected: 4, got not 4");
12 |   }
13 |   return(0); /* test passed */
14 | }
15 |
16 | char* test_two_plus_three() {
17 |   /* macro utests_assert can be used
18 |      to beautifully anyway-ugly code: */
19 |   utests_assert(2 + 3 == 6); /* Failed reason: 
20 |                                 'Assertion failed: 2 + 3 == 6' */
21 |   return(0); /* test passed */
22 | }
23 | 
24 | int utests() {
25 |   utest_t *chain = utests_alloc("Test 2 + 2 == 4", test_two_plus_two,
26 |       utests_alloc("Test 2 + 3 == 6", test_two_plus_three, 0 /* NULL */));
27 | 
28 |   utests_run(chain);
29 |   /* no return needed, because utests_run is a noreturn function */
30 | }

4. Licenses
===========
This template licensed under Unlicense (unlicense.org).
Copyright: [1] 34181 /bin/less Segmentation fault (core dumped)

Author: Amchik <am4ik1337@gmail.com>
This template developed under vol2-utils project.
This links works for 8 June 2022:
 - https://git.er2.tech/Amchik/vol2-utils
 - https://git.justaplant.xyz/vol2-utils.git (download)
 - https://git.justaplant.xyz/?p=vol2-utils.git;a=summary
 - https://github.com/Amchik/vol2-utils

