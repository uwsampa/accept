#include "dram.hpp"

std::map<uint64_t, uint64_t> DRAM::mem;

namespace {

#include "liberrorarch.h"
#include "liberrorutil.h"

  const double p1 = 1.0e-9; // 10^-9
  const double p2 = 1.0e-5; // 10^-5
  const double p3 = 1.0e-3; // 10^-3

  inline double getPMem(int level) {
    switch(level) {
    case 1:
      return p1;
    case 2:
      return p2;
    case 3:
      return p3;
    default:
      return p1;
    }
  }
}

void DRAM::dramStore(uint64_t address, uint64_t align, uint64_t cycles,
                       const char* type, int64_t param) {
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

uint64_t DRAM::dramLoad(uint64_t address, uint64_t ret, uint64_t align, uint64_t cycles,
                        const char* type, int64_t param) {
  if (!isAligned(address, align)) {
    std::cerr << "Error: unaligned address in load instruction." << std::endl;
    exit(0);
  }
  int level = param % 10;
  int num_bytes = getNumBytes(type);
  int bytes_req = (param / 10) % 10;
  int nAffectedBytes = bytes_req < num_bytes ? bytes_req : num_bytes;
  
  for (int i = 0; i < num_bytes; ++i) {
    if (i < nAffectedBytes) { // Only flip bits in lowest bytes
      const double time_elapsed = static_cast<double>(cycles - mem[address]) /
        CPU_FREQ;
      const double pFlip = getPMem(level) * time_elapsed;
      
      for (int j = 0; j < 8; ++j) {
        flipBit(ret, pFlip, i*8 + j);
      }
    }
    
    mem[address] = cycles;
    address += 1U;
  }
  return ret;
}
