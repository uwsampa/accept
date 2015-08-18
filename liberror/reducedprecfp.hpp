#ifndef _LIBERROR_REDUCEDPRECFP_H_
#define _LIBERROR_REDUCEDPRECFP_H_

#include <stdint.h>

class ReducedPrecFP {
public:
  static uint64_t FPOp(int64_t param, uint64_t ret, const char* type);
};

#endif /* _LIBERROR_REDUCEDPRECFP_H_ */
