#include <cstring>
#include <iostream>
#include <stdint.h>
#include <sys/time.h>

// Rounding mode
// 0: No rounding
// 1: Stochastic
// 2: Rounding to nearest
#define ROUNDING 0


// Voltage scaling mode
// 0: No voltage scaling (use bit-width reduction)
// 1: Voltage scaling - random value
// 2: Voltage scaling - single random bit-flip
// 3: Voltage scaling - MSB bit-flip
// 4: Voltage scaling - previous value
// 5: Voltage scaling - previous MSB
#define VOLTAGE_SCALING 0

// Error probability
#define ERRROR_PROB 0.01

// include approximation models
#include "liberrorutil.h"

#define rdtscll(val) do { \
    unsigned int __a,__d; \
    int tmp; \
    asm volatile("cpuid" : "=a" (tmp) : "0" (1) : "ebx", "ecx", "edx", "memory"); \
    asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
    (val) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
    asm volatile("cpuid" : "=a" (tmp) : "0" (1) : "ebx", "ecx", "edx", "memory"); \
  } while(0)

#define PARAM_MASK ((int64_t)0x0FFFF) // low 16-bits
#define MODEL_MASK ((int64_t)0x0FFFF0000) // high 16-bits

#define MASK_64 0xFFFFFFFFFFFFFFFFULL
#define MASK_32 0x00000000FFFFFFFFULL
#define MASK_16 0x000000000000FFFFULL
#define MASK_8  0x00000000000000FFULL

#define HALF_MANTISSA_W     10
#define HALF_EXPONENT_W     5
#define HALF_EXP_BIAS       ( (1 << (HALF_EXPONENT_W-1)) - 1)
#define HALF_EXP_MASK       ( (1 << HALF_EXPONENT_W) - 1 )
#define HALF_MAN_MASK       ( (1 << HALF_MANTISSA_W) - 1 )
#define FLOAT_MANTISSA_W    23
#define FLOAT_EXPONENT_W    8
#define FLOAT_EXP_BIAS      ( (1 << (FLOAT_EXPONENT_W-1)) - 1)
#define FLOAT_EXP_MASK      ( (1 << FLOAT_EXPONENT_W) - 1 )
#define FLOAT_MAN_MASK      ( (1 << FLOAT_MANTISSA_W) - 1 )
#define DOUBLE_MANTISSA_W   52
#define DOUBLE_EXPONENT_W   11
#define DOUBLE_EXP_BIAS     ( (1 << (DOUBLE_EXPONENT_W-1)) - 1)
#define DOUBLE_EXP_MASK     ( (1 << DOUBLE_EXPONENT_W) - 1 )
#define DOUBLE_MAN_MASK     ( (1ULL << DOUBLE_MANTISSA_W) - 1 )

// Global carry bit
uint64_t carry = 1;

// Global previous MSB
uint64_t prev_val = 0;

/*
 * injectInst drives all of the error injection routines
 * the injection routine(s) called depend on the param input
 *
 *
 *
 *
 */
__attribute__((always_inline))uint64_t injectInst(char* opcode, int64_t param, uint64_t ret, uint64_t op1,
    uint64_t op2, char* type) {

  // store original return value
  uint64_t return_value = ret;

  // Initialize random value generator
  struct timeval time;
  if (!rand_init) {
    gettimeofday(&time,NULL);
    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));
    rand_init = true;
  }

#if VOLTAGE_SCALING==0

  // decode parameter bits so we can pick model and pass model parameter
  int64_t model = ((param & MODEL_MASK) >> 16) & PARAM_MASK; // to be safe, re-mask the low 16-bits after shifting
  int64_t model_param = (param & PARAM_MASK);

  // Break down the model_param into a right and left mask
  uint32_t hishift = (model_param >> 8) & 0xFF;
  uint32_t loshift = (model_param & 0xFF);
  // Now generate the LSB mask
  uint64_t lomask = MASK_64 << loshift;
  // Now generate the MSB mask
  uint64_t himask = 0;

  #if ROUNDING==1
    ret += (carry<<loshift);
    carry = (carry==1) ? 0 : 1;
  #elif ROUNDING==2
    ret += (carry<<loshift)/2;
  #endif //ROUNDING

  // Type dependent himask (only works with ints)
  if (!strcmp(type, "Int64")) {
    if (ret&0x8000000000000000) {
      himask = MASK_64 << (64-hishift);
      return_value = (ret&lomask)|himask;
    } else {
      himask = MASK_64 >> hishift;
      return_value = (ret&lomask)&himask;
    }
  }
  else if (!strcmp(type, "Int32")) {
    if (ret&0x80000000) {
      himask = MASK_32 << (32-hishift);
      return_value = (ret&lomask)|himask;
    } else {
      himask = MASK_32 >> hishift;
      return_value = (ret&lomask)&himask;
    }
  }
  else if (!strcmp(type, "Int16")) {
    if (ret&0x8000) {
      himask = MASK_16 << (16-hishift);
      return_value = (ret&lomask)|himask;
    } else {
      himask = MASK_16 >> hishift;
      return_value = (ret&lomask)&himask;
    }
  }
  else if (!strcmp(type, "Int8")) {
    if (ret&0x80) {
      himask = MASK_8 << (8-hishift);
      return_value = (ret&lomask)|himask;
    } else {
      himask = MASK_8 >> hishift;
      return_value = (ret&lomask)&himask;
    }
  } else if (!strcmp(type, "Half")) {
    if (model==0) {
      return_value = ret&lomask;
    } else {
      // Fixed-point evaluation emulation
      int32_t max_exp = model;
      int32_t exponent = ( (ret >> HALF_MANTISSA_W) & HALF_EXP_MASK );
      // FIXME: Can't find a better solution at the moment...
      if (exponent <= max_exp) {
        loshift += (max_exp - exponent);
      }
      if (loshift > HALF_MANTISSA_W) {
        return_value = 0;
      } else {
        lomask = MASK_64 << loshift;
        return_value = ret&lomask;
      }
    }
  } else if (!strcmp(type, "Float")) {
    if (model==0) {
      return_value = ret&lomask;
    } else {
      // Fixed-point evaluation emulation
      int32_t max_exp = model;
      int32_t exponent = ( (ret >> FLOAT_MANTISSA_W) & FLOAT_EXP_MASK );
      // FIXME: Can't find a better solution at the moment...
      if (exponent <= max_exp) {
        loshift += (max_exp - exponent);
      }
      if (loshift > FLOAT_MANTISSA_W) {
        return_value = 0;
      } else {
        lomask = MASK_64 << loshift;
        return_value = ret&lomask;
      }
    }
  } else if (!strcmp(type, "Double")) {
    if (model==0) {
      return_value = ret&lomask;
    } else {
      // Fixed-point evaluation emulation
      int32_t max_exp = model;
      int32_t exponent = ( (ret >> DOUBLE_MANTISSA_W) & DOUBLE_EXP_MASK );
      // FIXME: Can't find a better solution at the moment...
      if (exponent <= max_exp) {
        loshift += (max_exp - exponent);
      }
      if (loshift > DOUBLE_MANTISSA_W) {
        return_value = 0;
      } else {
        lomask = MASK_64 << loshift;
        return_value = ret&lomask;
      }
    }
  } else {
    return_value = ret&lomask;
  }

#elif VOLTAGE_SCALING==1 // Voltage scaling - random value

  // Inject error with ERROR_PROB probability
  double draw = static_cast<double>(rand())/static_cast<double>(RAND_MAX);
  if (draw <= ERRROR_PROB) {
    // std::cout << "Injecting error (random value)!!!" << std::endl;
    if (!strcmp(type, "Int64") | !strcmp(type, "Double")) {
      uint64_t r = getRandom64();
      return_value = r;
    }
    else if (!strcmp(type, "Int32") | !strcmp(type, "Float")) {
      uint32_t r = getRandom32();
      return_value = r;
    }
    else if (!strcmp(type, "Int16") | !strcmp(type, "Half")) {
      uint16_t r = getRandom16();
      return_value = r;
    }
    else if (!strcmp(type, "Int8")) {
      uint8_t r = getRandom8();
      return_value = r;
    }
    // std::cout << std::hex << "    Before " << ret << ", After " << return_value << std::endl;
  }

#elif VOLTAGE_SCALING==2 // Voltage scaling - single random bit-flip
  // Inject error with ERROR_PROB probability
  double draw = static_cast<double>(rand())/static_cast<double>(RAND_MAX);
  if (draw <= ERRROR_PROB) {
    // std::cout << "Injecting error (single random bit-flip)!!!" << std::endl;
    int pos = rand() % getNumBits(type);
    uint64_t mask = 1ULL << pos;
    return_value = ret ^ mask;
    // std::cout << std::hex << "    Before " << ret << ", After " << return_value << std::endl;
  }

#elif VOLTAGE_SCALING==3 // Voltage scaling - MSB bit flip

  // Inject error with ERROR_PROB probability
  double draw = static_cast<double>(rand())/static_cast<double>(RAND_MAX);
  if (draw <= ERRROR_PROB) {
    // std::cout << "Injecting error (MSB bit flip)!!!" << std::endl;
    int pos = getNumBits(type)-1;
    uint64_t mask = 1ULL << pos;
    return_value = ret ^ mask;
    // std::cout << std::hex << "    Before " << ret << ", After " << return_value << std::endl;
  }

#elif VOLTAGE_SCALING==4 // Voltage scaling - previous value

  // Inject error with ERROR_PROB probability
  double draw = static_cast<double>(rand())/static_cast<double>(RAND_MAX);
  if (draw <= ERRROR_PROB) {
    // std::cout << "Injecting error (last value replacement)!!!" << std::endl;
    return_value = prev_val;
    // if (ret != return_value) {
    //   std::cout << std::hex << "    Before " << ret << ", After " << return_value << std::endl;
    // }
  }

#elif VOLTAGE_SCALING==5 // Voltage scaling - previous MSB
  int pos = getNumBits(type)-1;
  uint64_t mask = 1ULL << pos;

  // Inject error with ERROR_PROB probability
  double draw = static_cast<double>(rand())/static_cast<double>(RAND_MAX);
  if (draw <= ERRROR_PROB) {
    // std::cout << "Injecting error (last MSB replacement)!!!" << std::endl;
    if (prev_val & mask) {
      return_value = ret | mask;
    } else {
      return_value = ret & (~mask);
    }
    // if (ret != return_value) {
    //   std::cout << std::hex << "    Before " << ret << ", After " << return_value << std::endl;
    // }
  }
#endif //VOLTAGE_SCALING

  prev_val = return_value;
  return return_value;
}

/*
 * injectRegion drives all of the coarse error injection routines
 * the injection routine(s) called depend on the param input
 *
 *
 *
 *
 */
// void injectRegion(int64_t param, int64_t nargs, unsigned char* image, int im_size) {

//   static uint64_t instrumentation_time = 0U;
//   uint64_t before_time;
//   rdtscll(before_time);
//   static uint64_t initial_time = before_time;
//   int64_t elapsed_time = before_time - initial_time - instrumentation_time;
//   if (elapsed_time < 0) {
//     std::cerr << "\nNegative elapsed time\n" << std::endl;
//     exit(0);
//   }

//   // decode parameter bits so we can pick model and pass model parameter
//   int64_t model = ((param & MODEL_MASK) >> 16) & PARAM_MASK; // to be safe, re-mask the low 16-bits after shifting
//   int64_t model_param = (param & PARAM_MASK);

//   switch(model) {

//     case 0: // 0 = do nothing, otherwise known as precise execution
//       break;

//     case 1: // Digital NPU (ISCA2014)
//       invokeDigitalNPU(model_param, image, im_size);
//       break;

//     case 2: // Analog NPU (ISCA2014)
//       invokeAnalogNPU(model_param, image, im_size);
//       break;

//     default: // default is precise, do nothing
//       break;
//   }

//   uint64_t after_time;
//   rdtscll(after_time);
//   instrumentation_time += after_time - before_time;

// }
