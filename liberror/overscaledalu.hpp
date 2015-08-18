#ifndef _LIBERROR_OVERSCALEDALU_H_
#define _LIBERROR_OVERSCALEDALU_H_

#include <stdint.h>

class OverscaledALU {
public:
  static uint64_t BinOp(int64_t param, uint64_t ret, const char* type);
};

#endif /* _LIBERROR_OVERSCALEDALU_H_ */
