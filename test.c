#include <stdio.h>
#include <math.h>

#include <stdbool.h>
#include "f16.h"

#define LEN(ARR) (sizeof(ARR)/sizeof((ARR)[0]))

#define ASSERT_EQ_INT(actual, expected, label) \
    if (actual != expected) { \
        printf("%s FAILED: got %d, expected %d\n", (label), actual, expected); \
        all_passed = false; \
    } \

#define F16_TO_INT_TEST(input, expected) \
    ASSERT_EQ_INT(f16_to_int(input), expected, "f16_to_int(" #input ")")

#define F16_CONVERSION_TEST(input, expected) \
    ASSERT_EQ_INT(f16_to_int(f16_from_int(input)), expected, "conversion of " #input)

#define F16_ADD_TEST(a, b, expected) \
    ASSERT_EQ_INT(f16_to_int(f16_add(f16_from_int(a), f16_from_int(b))), expected, \
        "f16_add(" #a ", " #b ") != " #expected )

#define F16_MUL_TEST(a, b, expected) \
    ASSERT_EQ_INT(f16_to_int(f16_mul(f16_from_int(a), f16_from_int(b))), expected, \
        #a " * " #b " != " #expected )

void unit_test() {
    bool all_passed = true;

    F16_TO_INT_TEST(0x3c01, 1);
    F16_TO_INT_TEST(0x3c00, 1);
    F16_TO_INT_TEST(0x3bff, 0);
    F16_TO_INT_TEST(0x8000, 0);
    F16_TO_INT_TEST(0x7bff, 65504);
    F16_TO_INT_TEST(0xc000, -2);

    F16_CONVERSION_TEST(1, 1);
    F16_CONVERSION_TEST(65504, 65504);
    F16_CONVERSION_TEST(5678, 5676);
    F16_CONVERSION_TEST(-11, -11);;

    F16_ADD_TEST(8188, 8188, 16376);
    F16_ADD_TEST(0, 0, 0);
    F16_ADD_TEST(65504, 0, 65504);
    F16_ADD_TEST(32767, 32767, 65504);
    F16_ADD_TEST(-32767, -32767, -65504);
    F16_ADD_TEST(-120, 5, -115);
    F16_ADD_TEST(1, -5, -4);
    F16_ADD_TEST(-7, 7, 0);

    F16_MUL_TEST(7, 7, 49);
    F16_MUL_TEST(1, -1, -1);
    F16_MUL_TEST(-11, -13, 143);
    F16_MUL_TEST(0, 0, 0);
    F16_MUL_TEST(0, -501, 0);
    F16_MUL_TEST(1, 65504, 65504);

    if (all_passed) { printf("ALL PASS\n"); }
}

float pow2(int exponent) {
    if (exponent >= 0) {
        return (float)(1 << exponent);
    } else {
        return 1.0f / (float)(1 << -exponent);
    }
}

float f16_to_float(f16_t num) {
    float sign = (num & SIGN_MASK) ? -1 : 1;
    float mantissa = num & MANTISSA_MASK;
    int exponent = ((num & EXPONENT_MASK) >> EXPONENT_SCALE) - BIAS;

    float m = 1.0f + (mantissa / (1 << EXPONENT_SCALE));
    float value = m * pow2(exponent);
    return sign * value;
}

void print_division_error_rate(int range) {
    float total_error = 0.0f;
    for (int i = 0; i <= range; ++i) {
        f16_t val = f16_from_int(i);
        f16_t recip = f16_recip(val);
        float f16_val = f16_to_float(recip);
        float true_val = 1.0f / (float)i;
        float error = fabsf(true_val - f16_val);
        total_error += error;
        printf("1/%d: f16=0x%x, float=%f, true=%f, error=%g\n", i, recip, f16_val, true_val, error);
    }
    printf("Mean error: %g\n", total_error / (float)range);
    printf("RECIPROCAL_MAGIC: 0x%x\n", RECIPROCAL_MAGIC);
}

int main() {
    // print_division_error_rate(100);
    unit_test();
    return 0;
}
