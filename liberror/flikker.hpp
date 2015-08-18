#ifndef _LIBERROR_FLIKKER_H_
#define _LIBERROR_FLIKKER_H_

#include <iostream>
#include <map>
#include <stdint.h>

class Flikker {
private:
  // probability of bit flip per second
  // 1s refresh cycle, 65 x 10^-9
  static const double p1 = 0.000000000400;
  // 20s refresh cycle, 400 x 10^-12
  static const double p20 = 0.000000065;
  // byte addressable memory, maps address to cycle last accessed
  static std::map<uint64_t, uint64_t> mem;

public:
  static void flikkerStore(uint64_t address, uint64_t align,
                           uint64_t cycles, const char* type);
  static uint64_t flikkerLoad(uint64_t address, uint64_t ret, uint64_t align,
                              uint64_t cycles, const char* type, int64_t param);
};

#endif /* _LIBERROR_FLIKKER_H_ */
