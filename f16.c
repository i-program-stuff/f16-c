#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "f16.h"

// returns the number of digits in num when written in binary.
// eg. 3 returns 2 because 0b11, 6 returns 3 because 0b110
uint8_t num_digits(int num) {
#if defined(__GNUC__) || defined(__clang__)
    return sizeof(int)*8 - __builtin_clz(num);
#else
    uint8_t digits;
    for (digits = 1; num > 0; num >>= 1) {
        digits++;
    }
    return digits;
#endif
}

f16_t f16_abs(f16_t num) {
    return num & ~SIGN_MASK;
}

f16_t f16_from_int(int num) {
    if (num == 0) { // Special Case for Zero
        return 0;
    }

    bool is_negative = num < 0;
    uint32_t sign_bit = is_negative ? 1 : 0;
    uint32_t absolute_value = is_negative ? -num : num;

    // the minus 1 is accounting for the leading "1." before the mantissa.
    uint8_t digits = num_digits(absolute_value) - 1;
    uint32_t exponent = digits + BIAS;

    // shift the number to fit in the mantissa
    uint32_t shift = digits - EXPONENT_SCALE;
    uint32_t shifted_value = (digits >= EXPONENT_SCALE)
        ? (absolute_value >> shift)
        : (absolute_value << -shift);

    uint32_t mantissa = shifted_value & MANTISSA_MASK;

    return sign_bit << SIGN_SCALE | exponent << EXPONENT_SCALE | mantissa;
}

// scaling is how much the point should go forward (num * 10^-scaling)
// num: 10, scaling: 0 -> result: 10
// num: 314, scaling: 2 -> result: 3.14
// num: 618, scaling: 3 -> result: 0.618
f16_t f16_from_int_with_decimal(int num, uint32_t scaling) {
    f16_t one_tenth = f16_recip(f16_from_int(10));
    f16_t result = f16_from_int(num);

    for (int _ = 0; _ < scaling; _++) {
        result = f16_mul(result, one_tenth);
    }

    return result;
}

int f16_to_int(f16_t num) {
    int sign = (num & SIGN_MASK) != 0 ? -1 : 1;
    int exponent = ((num & EXPONENT_MASK) >> EXPONENT_SCALE) - BIAS;
    int mantissa = (num & MANTISSA_MASK);

    // the resulting number will be <1 if the exponent is <0,
    // so it will be floored to 0.
    if (exponent < 0) {
        return 0;
    }

    int scale_factor = 1 << exponent;
    int result = sign * (scale_factor + (scale_factor * mantissa >> EXPONENT_SCALE));

    // Rounding
    // result += ((1 << (EXPONENT_SCALE - exponent - 1)) & mantissa) > 0 ? sign : 0;

    return result;
}

f16_t f16_negate(f16_t num) {
    return num ^ SIGN_MASK;
};

f16_t f16_recip(f16_t num) {
    uint32_t sign = num & SIGN_MASK;
    uint32_t absolute_value = f16_abs(num);

    if (absolute_value >= RECIPROCAL_MAGIC) {
        return 0;
    }

    f16_t two = f16_from_int(2);

    uint32_t estimate = RECIPROCAL_MAGIC - absolute_value;

    // Newton's Method: new_estimate = estimate * (2 - absolute_value * estimate)
    estimate = f16_mul(estimate, f16_sub(two, f16_mul(absolute_value, estimate)));
    estimate = f16_mul(estimate, f16_sub(two, f16_mul(absolute_value, estimate)));

    return sign | estimate;
};

f16_t f16_add(f16_t a, f16_t b) {
    int32_t exponent_a = ((a & EXPONENT_MASK) >> EXPONENT_SCALE);
    int32_t exponent_b = ((b & EXPONENT_MASK) >> EXPONENT_SCALE);

    if (exponent_a == INF_EXPONENT) {
        return a;
    } else if (exponent_b == INF_EXPONENT) {
        return b;
    }

    uint32_t mantissa_a = (1 << EXPONENT_SCALE) | (a & MANTISSA_MASK);
    uint32_t mantissa_b = (1 << EXPONENT_SCALE) | (b & MANTISSA_MASK);
    
    int32_t exponent_with_bias;
    if (exponent_a > exponent_b) {
        exponent_with_bias = exponent_a;
        mantissa_b >>= exponent_a - exponent_b;
    } else {
        exponent_with_bias = exponent_b;
        mantissa_a >>= exponent_b - exponent_a;
    }

    bool a_is_positive = (a & SIGN_MASK) == 0;
    bool b_is_positive = (b & SIGN_MASK) == 0;

    uint32_t unscaled_mantissa;
    uint32_t sign;

    if (a_is_positive && b_is_positive) {
        unscaled_mantissa = mantissa_a + mantissa_b;
        sign = 0; // Positive

    } else if (!a_is_positive && !b_is_positive) {
        unscaled_mantissa = mantissa_a + mantissa_b;
        sign = SIGN_MASK; // Negative

    } else if (a_is_positive && !b_is_positive) {
        if (mantissa_a >= mantissa_b) {
            unscaled_mantissa = mantissa_a - mantissa_b;
            sign = 0; // Positive
        } else {
            unscaled_mantissa = mantissa_b - mantissa_a;
            sign = SIGN_MASK; // Negative
        }

    } else if (!a_is_positive && b_is_positive) {
        if (mantissa_a <= mantissa_b) {
            unscaled_mantissa = mantissa_b - mantissa_a;
            sign = 0; // Positive
        } else {
            unscaled_mantissa = mantissa_a - mantissa_b;
            sign = SIGN_MASK; // Negative
        }
    }

    uint8_t digits = num_digits(unscaled_mantissa) - 1;

    int32_t shift = digits - EXPONENT_SCALE;
    uint32_t shifted_value = (digits >= EXPONENT_SCALE)
        ? (unscaled_mantissa >> shift)
        : (unscaled_mantissa << -shift);

    uint32_t mantissa = shifted_value & MANTISSA_MASK;

    if (shift != -EXPONENT_SCALE) {
        exponent_with_bias += shift;
    } else {
        exponent_with_bias = 0; // special case for when subtraction results in 0
    }

    return sign | exponent_with_bias << EXPONENT_SCALE | mantissa;
}

f16_t f16_sub(f16_t a, f16_t b) {
    return f16_add(a, f16_negate(b));
}

f16_t f16_mul(f16_t a, f16_t b) {
    uint32_t sign = (a & SIGN_MASK) ^ (b & SIGN_MASK);

    // if a or b is zero, return zero
    if ((a & ~SIGN_MASK) == 0 || (b & ~SIGN_MASK) == 0) {
        return sign;
    }

    int32_t exponent_a_with_bias = ((a & EXPONENT_MASK) >> EXPONENT_SCALE);
    int32_t exponent_b_with_bias = ((b & EXPONENT_MASK) >> EXPONENT_SCALE);

    if (exponent_a_with_bias == INF_EXPONENT || exponent_b_with_bias == INF_EXPONENT) {
        return sign | F16_INFINITY;
    }

    int32_t exponent_with_bias = exponent_a_with_bias + exponent_b_with_bias - BIAS;

    if (exponent_with_bias <= SUBNORMAL_EXPONENT) {
        return 0; // TODO: IMPLEMENT SUBNORMAL NUMBERS
    }

    uint32_t mantissa_a = (a & MANTISSA_MASK);
    uint32_t mantissa_b = (b & MANTISSA_MASK);

    // unscaled_mantissa = 2^EXPONENT_SCALE * (1 + M_a/2^EXPONENT_SCALE)(1 + M_b/2^EXPONENT_SCALE)
    uint32_t unscaled_mantissa =
        (1 << EXPONENT_SCALE) + ((mantissa_a*mantissa_b) >> EXPONENT_SCALE) + mantissa_a + mantissa_b;

    uint32_t mantissa = unscaled_mantissa;

    // shift mantissa to fit in the mantissa field
    if (unscaled_mantissa & (2 << EXPONENT_SCALE)) {
        mantissa >>= 1;
        exponent_with_bias += 1;
    }

    mantissa &= MANTISSA_MASK;

    // make nearest even
    if (unscaled_mantissa & 1 && exponent_with_bias != INF_EXPONENT - 1) {
        mantissa += 1;

        if (mantissa & ~MANTISSA_MASK) {
            mantissa &= MANTISSA_MASK;

            exponent_with_bias += 1;
        }
    }

    if (exponent_with_bias > INF_EXPONENT || exponent_with_bias < 0) {
        return sign | F16_INFINITY;
    }

    return sign | exponent_with_bias << EXPONENT_SCALE | mantissa;
}

f16_t f16_div(f16_t a, f16_t b) {
    return f16_mul(a, f16_recip(b));
}
