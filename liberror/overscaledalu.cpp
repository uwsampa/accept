#include "overscaledalu.hpp"

namespace {
#include "liberrorutil.h"

  // alu probabilities
  const double ap1 = 0.000001;
  const double ap2 = 0.0001;
  const double ap3 = 0.01;

  inline double getPALU(int level) {
    switch(level) {
    case 1:
      return ap1;
    case 2:
      return ap2;
    case 3:
      return ap3;
    default:
      return ap1;
    }
  }
}

/*
 * For binary integer operations, set low order bytesToInject to
 * random bit pattern if probability <= pALU
 *
 * param is two digits: tens is number of bytes to inject with error
 *                      ones is aggressiveness of approximation
 */
uint64_t OverscaledALU::BinOp(int64_t param, uint64_t ret, const char* type) {
  int numBytes = getNumBytes(type);
  int bytesToInject = (param/10)%10;
  bytesToInject = (bytesToInject < numBytes) ? bytesToInject : numBytes;
  int level = (param%10);
  double pALU = getPALU(level);
  if(getRandomProb() <= pALU) {
    uint64_t r = getRandom64();
    memcpy(&ret, &r, bytesToInject*sizeof(char));
  }
  return ret;
}
