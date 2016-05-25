#ifndef _LIBERROR_UTIL_H_
#define _LIBERROR_UTIL_H_

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/*
 * liberrorutil: helper functions for random number generation and bit manipulation
 */
#define SIZEOF_BITS(n) (sizeof(n)*BITS_PER_BYTE)
#define FP_M ((int)23)
#define DP_M ((int)52)
#define BITS_PER_BYTE ((int)8)
#define MAX_RAND ~((uint64_t)0ULL)


// Global random initialization variable
static bool rand_init = false;


// 64bit random integer generator
// From: http://bit.ly/27Io5y8
inline unsigned long long getRandom64() {
    unsigned long long r = 0;
    for (int i = 0; i < 5; ++i) {
        r = (r << 15) | (rand() & RAND_MAX);
    }
    return r & 0xFFFFFFFFFFFFFFFFULL;
}

// 32bit random integer generator
inline unsigned int getRandom32() {
    unsigned int r = 0;
    for (int i = 0; i < 3; ++i) {
        r = (r << 15) | (rand() & RAND_MAX);
    }
    return r & 0xFFFFFFFFU;
}

// 16bit random integer generator
inline unsigned short getRandom16() {
    unsigned short r = 0;
    for (int i = 0; i < 2; ++i) {
        r = (r << 15) | (rand() & RAND_MAX);
    }
    return r & 0xFFFFU;
}

// 8bit random integer generator
inline unsigned char getRandom8() {
    unsigned char r = 0;
    r = rand() & RAND_MAX;
    return r & 0xFFU;
}

/*
 * generate pseudo-random number (double) between 0.0 and 1.0
 */
inline double getRandomProb() {
  return static_cast<double>(getRandom64())/static_cast<double>(MAX_RAND);
}

/*
 * return number of bits for LLVM types
 */
inline int getNumBits(const char* type) {
  if (strcmp(type, "Double") == 0) return 52;
  if (strcmp(type, "Float") == 0) return 23;
  if (strcmp(type, "Half") == 0) return 10;
  if (strcmp(type, "Int64") == 0) return 64;
  if (strcmp(type, "Int32") == 0) return 32;
  if (strcmp(type, "Int16") == 0) return 16;
  if (strcmp(type, "Int8") == 0) return 8;
  if (strcmp(type, "Int1") == 0) return 1;
  return 0;
}

/*
 * return number of bytes for LLVM types
 */
inline int getNumBytes(const char* type) {
  return (getNumBits(type)+7)/8;
}

/*
 * check alignment of an address
 */
inline bool isAligned(uint64_t addr, uint64_t align) {
  if (align == 0) return true;
  return !(addr & (align - 1ULL));
}

/*
 * flip single bit with probability pBitFlip
 */
inline void flipBit(uint64_t &n, double pBitFlip, int bit) {
  if (bit >= SIZEOF_BITS(n) || bit < 0) { return; }
  if (getRandomProb() <= pBitFlip) { n ^= (1ULL << bit); }
}

/*
 * flip up to numBits, independently with probability pBitFlip
 */
inline void flipBits(uint64_t &n, double pBitFlip, int numBits) {
  if (numBits > SIZEOF_BITS(n) || numBits <= 0) { return; }
  for (int i = 0; i < numBits; i++) {
    flipBit(n, pBitFlip, i);
  }
}

/*
 * flip up to numBytes
 */
inline void flipBitsInBytes(uint64_t &n, double pFlip, int numBytes) {
  return;
}

/*
 * mask off floating point mantissa, leaving most significant reqPrecision bits unchanged
 * type specifies "Float" or "Double" to determine mantissa bits
 */
inline void maskMantissa(uint64_t &n, const char *type, int reqPrecision) {
  int mantissabits = 0;
  if (strcmp(type,"Float")==0) { mantissabits = FP_M; }
  else if (strcmp(type,"Double")==0) { mantissabits = DP_M; }
  else { return; }
  if (reqPrecision < 0 || reqPrecision > mantissabits) { return; }
  n &= ((~(0ULL)) << (mantissabits - reqPrecision));
}

#endif /* _LIBERROR_UTIL_H_ */
