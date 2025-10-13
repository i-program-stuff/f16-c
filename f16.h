#ifndef F16_H
#define F16_H

#include <stdint.h>

#define SIGN_MASK 0x8000
#define EXPONENT_MASK 0x7C00
#define MANTISSA_MASK 0x03FF

#define SIGN_SCALE 15
#define EXPONENT_SCALE 10

#define BIAS 0xF

#define INF_EXPONENT ((1 << (SIGN_SCALE - EXPONENT_SCALE)) - 1)
#define F16_INFINITY (INF_EXPONENT << EXPONENT_SCALE)

#define SUBNORMAL_EXPONENT 0

#define RECIPROCAL_MAGIC f16_from_int(2*(1 << EXPONENT_SCALE)*BIAS)

typedef uint16_t f16_t;

void f16_to_string(f16_t num, char* buffer);

int f16_to_int(f16_t);
f16_t f16_from_int(int);
f16_t f16_from_int_with_decimal(int num, uint32_t scaling);

f16_t f16_negate(f16_t);
f16_t f16_recip(f16_t);

f16_t f16_add(f16_t, f16_t);
f16_t f16_sub(f16_t, f16_t);
f16_t f16_mul(f16_t, f16_t);
f16_t f16_div(f16_t, f16_t);

#endif