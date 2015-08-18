#include "flikker.hpp"

namespace {
#include "liberrorarch.h"
#include "liberrorutil.h"
}

std::map<uint64_t, uint64_t> Flikker::mem;

void Flikker::flikkerStore(uint64_t address, uint64_t align,
                           uint64_t cycles, const char* type) {
    if (!isAligned(address, align)) {
      std::cerr << "Error: unaligned address in store instruction." << std::endl;
      exit(0);
    }
    
    int num_bytes = getNumBytes(type);
    for (int i = 0; i < num_bytes; ++i) {
      mem[address] = cycles;
      address += 1U;
    }
  }

  uint64_t Flikker::flikkerLoad(uint64_t address, uint64_t ret, uint64_t align,
                            uint64_t cycles, const char* type, int64_t param) {
    if (!isAligned(address, align)) {
      std::cerr << "Error: unaligned address in load instruction." << std::endl;
      exit(0);
    }
    
    int num_bytes = getNumBytes(type);
    
    double pError = 0.0;
    if (param == 1)
      pError = p1;
    else if (param == 2)
      pError = p20;

    // all bytes are affected
    for (int i = 0; i < num_bytes; ++i) {
      const double time_elapsed = static_cast<double>(cycles - mem[address]) /
        CPU_FREQ;
      const double pFlip = pError * time_elapsed;
      
      for (int j = 0; j < 8; ++j) {
        flipBit(ret, pFlip, i*8 + j);
      }
      
      mem[address] = cycles;
      address += 1U;
    }
    
    return ret;
  }
