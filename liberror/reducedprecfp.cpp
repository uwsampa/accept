#include "reducedprecfp.hpp"

namespace {
#include "liberrorutil.h"
  
  const int fp1 = 16;
  const int fp2 = 8;
  const int fp3 = 4;

  const int dp1 = 32;
  const int dp2 = 16;
  const int dp3 = 8;

  inline int getFPPrec(int level) {
    switch (level) {
    case 1:
      return fp1;
    case 2:
      return fp2;
    case 3:
      return fp3;
    default:
      return fp1;
    }
  }

  inline int getDPPrec(int level) {
    switch (level) {
    case 1:
      return dp1;
    case 2:
      return dp2;
    case 3:
      return dp3;
    default:
      return dp1;
    }
  }

  inline int getPrec(const char* type, int level) {
    if (!strcmp(type,"Float")) {
      return getFPPrec(level);
    } else if (!strcmp(type, "Double")) {
      return getDPPrec(level);
    } else {
      return -1;
    }
  }

}

uint64_t ReducedPrecFP::FPOp(int64_t param, uint64_t ret, const char* type) {
  int level = (param%10);
  int reqPrec = getPrec(type, level);
  if (reqPrec == -1) { return ret; }
  maskMantissa(ret, type, reqPrec);
  return ret;
}
