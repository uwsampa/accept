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

#define MASK_64 0xFFFFFFFFFFFFFFFFull
#define MASK_32 0x00000000FFFFFFFFull
#define MASK_16 0x000000000000FFFFull
#define MASK_8  0x00000000000000FFull


/*
 * injectInst drives all of the error injection routines
 * the injection routine(s) called depend on the param input
 *
 * 
 *
 *
 */
uint64_t injectInst(char* opcode, int64_t param, uint64_t ret, uint64_t op1,
    uint64_t op2, char* type) {
  static uint64_t instrumentation_time = 0U;
  uint64_t before_time;
  rdtscll(before_time);
  static uint64_t initial_time = before_time;
  int64_t elapsed_time = before_time - initial_time - instrumentation_time;
  if (elapsed_time < 0) {
    std::cerr << "\nNegative elapsed time\n" << std::endl;
    exit(0);
  }
  // store original return value
  uint64_t return_value = ret;

  // decode parameter bits so we can pick model and pass model parameter
  int64_t model = ((param & MODEL_MASK) >> 16) & PARAM_MASK; // to be safe, re-mask the low 16-bits after shifting
  int64_t model_param = (param & PARAM_MASK);
  //std::cout << "[model,param] = [" << model << "," << model_param << "]" << std::endl;

  switch(model) {

  case 0: // 0 = do nothing, otherwise known as precise execution
    break;

  case 1: // overscaled alu
    if (strcmp(type, "Float") && strcmp(type, "Double") && // not Float or Double
        strcmp(opcode, "store") && strcmp(opcode, "load")) { // not store or load
      return_value = OverscaledALU::BinOp(model_param, ret, type);
    }
    break;
    
  case 2: // reduced precision fp
    if (strcmp(opcode, "store") && strcmp(opcode, "load") && // not store or load
        (!strcmp(type, "Float") || !strcmp(type, "Double"))) { // is Float or Double
      return_value = ReducedPrecFP::FPOp(model_param, ret, type);
    }
    break;

  case 3: // flikker
    if (!strcmp(opcode, "store")) {
      Flikker::flikkerStore(op1, ret, elapsed_time, type);
    } else if (!strcmp(opcode, "load")) {
      return_value = Flikker::flikkerLoad(op1, ret, op2, elapsed_time, type, model_param);
    }
    break;

  case 4: // load-value approximation
    // applies to FP load instructions
    if (!strcmp(opcode, "load")) {
      return_value = LVA::lvaLoad(op1 /*ld addr*/, ret /*true value*/, type, model_param /*pc*/);
    }
    break;

  case 5: // sram
    if (!strcmp(opcode, "store")) {
      // do nothing on stores (our SRAM model is pessimistic and only injects on loads)
      //SRAM::sramStore(op1, ret, elapsed_time, type, param);
    } else if (!strcmp(opcode, "load")) {
      return_value = SRAM::sramLoad(op1, ret, op2, elapsed_time, type, model_param);
    }
    break;

  case 6: // dram
    if (!strcmp(opcode, "store")) {
      DRAM::dramStore(op1, ret, elapsed_time, type, param);
    } else if (!strcmp(opcode, "load")) {
      return_value = DRAM::dramLoad(op1, ret, op2, elapsed_time, type, model_param);
    }
    break;

    // combo1
  case 7:
    if (!strcmp(opcode, "store")) {
      // do nothing on stores (using SRAM model)
      //SRAM::sramStore(op1, ret, elapsed_time, type, param);
    } else if (!strcmp(opcode, "load")) {
      return_value = SRAM::sramLoad(op1, ret, op2, elapsed_time, type, model_param);
    } else if (!strcmp(type, "Float") || !strcmp(type, "Double")) {
      return_value = ReducedPrecFP::FPOp(model_param, ret, type);
    } else {
      return_value = OverscaledALU::BinOp(model_param, ret, type);
    }
    break;

    // combo2
  case 8:
    if (!strcmp(opcode, "store")) {
      // do nothing on stores
    } else if (!strcmp(opcode, "load")) {
      // do nothing on loads
    } else if (!strcmp(type, "Float") || !strcmp(type, "Double")) {
      return_value = ReducedPrecFP::FPOp(model_param, ret, type);
    } else {
      return_value = OverscaledALU::BinOp(model_param, ret, type);
    }
    break;

  case 9: {
    // Break down the model_param into a right and left mask
    uint32_t hishift = (model_param >> 8) & 0xFF;
    uint32_t loshift = (model_param & 0xFF);
    // Now generate the LSB mask
    uint64_t lomask = MASK_64 << loshift;
    // Now generate the MSB mask
    uint64_t himask = 0;

    // Type dependent himask (only works with ints)
    if (strcmp(type, "Int64")) {
      if (ret&0x8000000000000000) {
        himask = MASK_64 << (64-hishift);
        return_value = ret&lomask|himask;
      } else {
        himask = MASK_64 >> hishift;
        return_value = ret&lomask&himask;
      }
    }
    else if (strcmp(type, "Int32")) {
      if (ret&0x80000000) {
        himask = MASK_32 << (32-hishift);
        return_value = ret&lomask|himask;
      } else {
        himask = MASK_32 >> hishift;
        return_value = ret&lomask&himask;
      }
    }
    else if (strcmp(type, "Int16")) {
      if (ret&0x8000) {
        himask = MASK_16 << (16-hishift);
        return_value = ret&lomask|himask;
      } else {
        himask = MASK_16 >> hishift;
        return_value = ret&lomask&himask;
      }
    }
    else if (strcmp(type, "Int8")) {
      if (ret&0x80) {
        himask = MASK_8 << (8-hishift);
        return_value = ret&lomask|himask;
      } else {
        himask = MASK_8 >> hishift;
        return_value = ret&lomask&himask;
      }
    } else {
      return_value = 0;
      std::cerr << "Cannot apply MSB masking to non-int type\n" << std::endl;
    }
    break;
  }

  default: // default is also precise, do nothing
    break;
  }

  uint64_t after_time;
  rdtscll(after_time);
  instrumentation_time += after_time - before_time;

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
