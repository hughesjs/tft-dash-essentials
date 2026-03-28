/*
 * test_helpers.h — Shared test assertion macros for TFT Dash test suites
 */

#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#define ASSERT_TRUE(cond)  assert(cond)
#define ASSERT_FALSE(cond) assert(!(cond))
#define ASSERT_EQ(a, b)    assert((a) == (b))
#define ASSERT_NEQ(a, b)   assert((a) != (b))
#define ASSERT_NULL(p)     assert((p) == NULL)
#define ASSERT_NOT_NULL(p) assert((p) != NULL)
#define ASSERT_STR_EQ(a, b) assert(strcmp((a), (b)) == 0)
#define ASSERT_FLOAT_NEAR(a, b) assert(fabs((double)(a) - (double)(b)) < 0.01)

#endif /* TEST_HELPERS_H */
