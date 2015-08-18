#include "sram.hpp"

/*
 * This model represents lowered SRAM supply voltage (as cited by EnerJ)
 * For simplicity (to avoid memory simulation), we assume that
 * errors only occur on loads (read upsets), but we use the probabilities
 * for write failures (which are more severe). Since we don't model memory,
 * we don't actually "persist" read upsets (or write failures), and instead
 * approximate the behavior by flipping bits on loads (reads).
 *
 * By approximating the model, we should see pessimistic error results for two reasons:
 * 1. using higher probability of failures
 * 2. loads are more common than stores, thus using the error probabilities for
 *    stores (which are worse), we compound the effect
 *
 */

namespace {

#include "liberrorarch.h"
#include "liberrorutil.h"

  // sram write failure probabilities
  const double p1 = 2.5703957827688647e-6; // 10^-5.59
  const double p2 = 1.1481536214968817e-5; // 10^-4.94
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

uint64_t SRAM::sramLoad(uint64_t address, uint64_t ret, uint64_t align, uint64_t cycles,
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
      const double pFlip = getPMem(level);
      
      for (int j = 0; j < 8; ++j) {
        flipBit(ret, pFlip, i*8 + j);
      }
    }
  }
  return ret;
}
