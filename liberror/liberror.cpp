#include <cstring>
#include <iostream>

// include approximation models
#include "reducedprecfp.hpp"
#include "lva.hpp"
#include "flikker.hpp"
#include "overscaledalu.hpp"
#include "npu.hpp"
#include "dram.hpp"
#include "sram.hpp"

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
  // static uint64_t instrumentation_time = 0U;
  // uint64_t before_time;
  // rdtscll(before_time);
  // static uint64_t initial_time = before_time;
  // int64_t elapsed_time = before_time - initial_time - instrumentation_time;
  // if (elapsed_time < 0) {
  //   std::cerr << "\nNegative elapsed time\n" << std::endl;
  //   exit(0);
  // }
  // store original return value
  uint64_t return_value = ret;

  // decode parameter bits so we can pick model and pass model parameter
  int64_t model = ((param & MODEL_MASK) >> 16) & PARAM_MASK; // to be safe, re-mask the low 16-bits after shifting
  int64_t model_param = (param & PARAM_MASK);
  // std::cout << "[model,param] = [" << model << "," << model_param << "]" << std::endl;

  // Break down the model_param into a right and left mask
  uint32_t hishift = (model_param >> 8) & 0xFF;
  uint32_t loshift = (model_param & 0xFF);
  // Now generate the LSB mask
  uint64_t lomask = MASK_64 << loshift;
  // Now generate the MSB mask
  uint64_t himask = 0;

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
  // if (ret!=return_value)
  //   std::cout << "[before, after]: [" << std::hex << ret << ", " << return_value << std::dec << "]" << std::endl;

  // uint64_t after_time;
  // rdtscll(after_time);
  // instrumentation_time += after_time - before_time;

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
void injectRegion(int64_t param, int64_t nargs, unsigned char* image, int im_size) {

  static uint64_t instrumentation_time = 0U;
  uint64_t before_time;
  rdtscll(before_time);
  static uint64_t initial_time = before_time;
  int64_t elapsed_time = before_time - initial_time - instrumentation_time;
  if (elapsed_time < 0) {
    std::cerr << "\nNegative elapsed time\n" << std::endl;
    exit(0);
  }

  // decode parameter bits so we can pick model and pass model parameter
  int64_t model = ((param & MODEL_MASK) >> 16) & PARAM_MASK; // to be safe, re-mask the low 16-bits after shifting
  int64_t model_param = (param & PARAM_MASK);

  switch(model) {

    case 0: // 0 = do nothing, otherwise known as precise execution
      break;

    case 1: // Digital NPU (ISCA2014)
      invokeDigitalNPU(model_param, image, im_size);
      break;

    case 2: // Analog NPU (ISCA2014)
      invokeAnalogNPU(model_param, image, im_size);
      break;

    default: // default is precise, do nothing
      break;
  }

  uint64_t after_time;
  rdtscll(after_time);
  instrumentation_time += after_time - before_time;

}
