// Wisp platform: no measurement yet.

#include "msp430/perfctr/perfctr.h"

static unsigned long __accept_roi_result = 0;

void accept_roi_begin() {
    __perfctr_start();
}

void accept_roi_end() {
    __perfctr_stop();
    __accept_roi_result = __perfctr;
    // TODO: something, anything to record results
}
