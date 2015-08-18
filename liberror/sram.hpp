#include <map>

#include <cstring>
#include <ctime>
#include <iostream>
#include <cstdlib>

#include <ios>
#include <iomanip>
#include <string>

#include <stdint.h>

class SRAM {
public:
  static uint64_t sramLoad(uint64_t address, uint64_t ret, uint64_t align,
      uint64_t cycles, const char* type, int64_t param);
};

