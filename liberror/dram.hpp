#include <map>

#include <cstring>
#include <ctime>
#include <iostream>
#include <cstdlib>

#include <ios>
#include <iomanip>
#include <string>

#include <stdint.h>

class DRAM {
public:
  static void dramStore(uint64_t address, uint64_t align, uint64_t cycles,
                        const char* type, int64_t param);
  static uint64_t dramLoad(uint64_t address, uint64_t ret, uint64_t align,
                           uint64_t cycles, const char* type, int64_t param);

private:
  static std::map<uint64_t, uint64_t> mem;
};

