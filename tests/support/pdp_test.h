/* pdp_test.h -- tiny assertion harness for pdp's C tests.
 *
 * No external dependencies: each test file is its own executable whose main()
 * runs checks and ends with PDP_TEST_MAIN_END(), which returns non-zero if any
 * check failed (so CTest sees it).
 */
#ifndef PDP_TEST_H
#define PDP_TEST_H

#include <stdio.h>

static int pdp_test_failures = 0;
static int pdp_test_checks = 0;

#define PDP_CHECK(cond)                                                     \
    do {                                                                    \
        pdp_test_checks++;                                                  \
        if (!(cond)) {                                                      \
            pdp_test_failures++;                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                   \
    } while (0)

#define PDP_CHECK_EQ_INT(a, b)                                              \
    do {                                                                    \
        long pdp_a = (long)(a), pdp_b = (long)(b);                          \
        pdp_test_checks++;                                                  \
        if (pdp_a != pdp_b) {                                               \
            pdp_test_failures++;                                            \
            fprintf(stderr, "FAIL %s:%d: %s (%ld) != %s (%ld)\n",           \
                __FILE__, __LINE__, #a, pdp_a, #b, pdp_b);                  \
        }                                                                   \
    } while (0)

#define PDP_TEST_MAIN_END()                                                 \
    do {                                                                    \
        if (pdp_test_failures) {                                            \
            fprintf(stderr, "%d/%d checks failed\n",                        \
                pdp_test_failures, pdp_test_checks);                        \
            return 1;                                                       \
        }                                                                   \
        printf("ok: %d checks\n", pdp_test_checks);                         \
        return 0;                                                           \
    } while (0)

#endif /* PDP_TEST_H */
