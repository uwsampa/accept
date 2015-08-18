#include <stdint.h>

class LVA {
    typedef union {
	double d;
	float f;
	int8_t  i8;
	int16_t i16;
	int32_t i32;
	uint64_t b;
    } bitmess;

    typedef struct {
	uint64_t tag;
	int confidence;
	int degree;
	double LHB[4];
	int LHB_head;
    } lva_entry;


public:
  static uint64_t lvaLoad(uint64_t ld_address, uint64_t ret,
      const char* type, uint64_t pc);

private:
  //    static const uint64_t max_rand;
    static lva_entry approximator[512];
    static bitmess GHB[4];
    static int GHB_head;
    static float threshold;
    static float pHitRate;
    static int   degree;
    static int   hash_method;
    static bool  init_done;
    static uint64_t stats_accesses;
    static uint64_t stats_cache_misses;
    static uint64_t stats_predictions;
    static uint64_t stats_fetch_avoided;

    static void init(uint64_t param);
    static bool isCacheHit(uint64_t ld_address);
    static uint64_t getHash(uint64_t pc);
    static void print_summary();
};
