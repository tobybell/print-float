#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

typedef double f64;
typedef float f32;
typedef unsigned long long u64;
typedef unsigned u32;
typedef int i32;
typedef _Bool bool;

static void reverse(char* a, char* b) {
  while (a < --b) {
    char t = *a;
    *a++ = *b;
    *b = t;
  }
}

static __uint128_t pow5(u32 p, i32 leave_gap, i32* reduced) {
  __uint128_t ans = 1;
  __uint128_t n = 5;
  while (p) {
    if (p & 1)
      ans *= n;
    n *= n;
    p /= 2;
  }
  if (ans >> 64) {
    u32 buf = __builtin_clzll(ans >> 64);
    if (buf < leave_gap) {
      i32 shift = leave_gap - buf;
      ans >>= shift;
      *reduced = shift;
      return ans;
    }
  }
  *reduced = 0;
  return ans;
}

// computes `a * 2^b / 10^c`
static u32 floor2(u32 a, i32 b, i32 c) {
  b -= c;
  __uint128_t ans = a;
  i32 prec = 32 - __builtin_clz(a);
  if (c > 0) {
    i32 shiftup = 128 - prec;
    b -= shiftup;
    ans <<= shiftup;
    i32 red;
    __uint128_t den = pow5(c, prec, &red);
    ans /= den;
    b -= red;
  } else if (c < 0) {
    i32 red;
    __uint128_t p = pow5((u32) -c, prec, &red);
    ans *= p;
    b += red;
  }
  if (b < 0)
    return (u32) (ans >> -b);
  return (u32) (ans << b);
}

static u32 ceil2(u32 a, i32 b, i32 c) {
  b -= c;
  __uint128_t ans = a;
  i32 prec = 32 - __builtin_clz(a);
  bool bump = 0;
  if (c > 0) {
    i32 shiftup = 128 - prec;
    b -= shiftup;
    ans <<= shiftup;
    i32 red;
    __uint128_t den = pow5(c, prec, &red);
    bump = !!(ans % den);
    ans /= den;
    b -= red;
  } else if (c < 0) {
    i32 red;
    __uint128_t p = pow5((u32) -c, prec, &red);
    ans *= p;
    b += red;
  }
  if (b < 0)
    return (u32) (ans >> -b) + (!!(ans & (((__uint128_t) 1 << -b) - 1)) || bump);
  return (u32) (ans << b);
}

static i32 floor_div(i32 a, i32 b) { return a / b - (a % b && (a ^ b) < 0); }

static char* float_to_string(f32 x, char* buffer) {
  u32 bits;
  memcpy(&bits, &x, 4);
  bool sign = bits >> 31;
  u32 exp_bits = (u32) (bits >> 23u) & ((1u << 8u) - 1u);
  u32 sig_bits = bits & ((1u << 23u) - 1u);

  bool is_infinite = exp_bits == 0b11111111u;

  if (is_infinite) {
    if (!sig_bits) {
      if (sign)
        *buffer++ = '-';
      memcpy(buffer, "inf", 3);
      return buffer + 3;
    } else {
      memcpy(buffer, "nan", 3);
      return buffer + 3;
    }
  }

  if (sign)
    *buffer++ = '-';

  if (!sig_bits && !exp_bits) {
    memcpy(buffer, "0e0", 3);
    return buffer + 3;
  }

  i32 exp2 = (i32) exp_bits - 149;  // 127 + 23
  u32 sig2 = sig_bits;
  if (exp_bits) {  // normalized vs denormalized
    sig2 |= 1u << 23u;
    --exp2;
  }

  // check if at bottom of exponent range (denser floats below)
  // this influences the interval width `exp10`, and the lower bound value `lo`
  i32 exp10;
  if (sig_bits) {
    exp10 = floor_div(exp2 * 30103, 100000);
  } else {
    exp10 = floor_div((exp2 - 2) * 30103 + 47712, 100000);
  }

  u32 a, b;
  if (sig_bits & 1) {  // odd, round away -> open interval
    a = (sig_bits ? floor2(2 * sig2 - 1, exp2 - 1, exp10) : floor2(4 * sig2 - 1, exp2 - 2, exp10)) + 1;
    b = ceil2(2 * sig2 + 1, exp2 - 1, exp10) - 1;
  } else {  // even, round towards -> closed interval
    a = sig_bits ? ceil2(2 * sig2 - 1, exp2 - 1, exp10) : ceil2(4 * sig2 - 1, exp2 - 2, exp10);
    b = floor2(2 * sig2 + 1, exp2 - 1, exp10);
  }

  u32 d = b / 10;  // power of 10 candidate

  u32 sig10;
  if (d * 10 >= a) {
    sig10 = d;
    ++exp10;
  } else {
    // note: this algorithm currently makes no attempt to pick the closest
    // representation if multiple valid exist, but if you wanted to do that,
    // this would be the place to do it, using round(calc(sig2, exp2, exp10))).
    sig10 = b;
  }


  while (!(sig10 % 10)) {  // divide off extra 0s
    sig10 /= 10;
    ++exp10;
  }

  if (sig10 < 10) {
    *buffer++ = (char) ('0' + sig10);
  } else {
    char* begin = buffer;
    do {
      *buffer++ = (char) ('0' + sig10 % 10);
      sig10 /= 10;
      ++exp10;
    } while (sig10 >= 10);
    *buffer++ = '.';
    *buffer++ = (char) ('0' + sig10);
    reverse(begin, buffer);
  }

  *buffer++ = 'E';
  if (exp10 < 0) {
    *buffer++ = '-';
    exp10 = -exp10;
  }

  char* begin = buffer;
  do {
    *buffer++ = (char) ('0' + exp10 % 10);
    exp10 /= 10;
  } while (exp10);
  reverse(begin, buffer);
  return buffer;
}

static void round_trip(u32 x) {
  f32 fx;
  memcpy(&fx, &x, 4);
  char buf[17];
  *float_to_string(fx, buf) = 0;
  f32 fy = strtof(buf, 0);
  u32 y;
  memcpy(&y, &fy, 4);
  if (x != y)
    abort();
}

int main() {
  printf("checking all floats...\n");
  u32 max_norm = 0b11111111 << 23;
  u32 pct = 0;
  u32 next = 0;
  for (u32 i = 0; i < max_norm; i += 10000000) {
    if (i >= next) {
      printf("%u%%\n", pct);
      next = (u32) ((u64) max_norm * ++pct / 100);
    }
    round_trip(i);
  }
  printf("checked all floats.\n");
}
