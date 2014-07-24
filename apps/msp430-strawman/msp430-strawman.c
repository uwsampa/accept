#include <enerc.h>

APPROX volatile static int x;

#define ITERS 100

APPROX int identity (APPROX int x) {
    APPROX volatile int y = x;
    return y;
}

#ifdef __clang__
__attribute__((section(".init9"), aligned(2)))
#endif
int main(void) {
    unsigned i;

    accept_roi_begin();

    for (i = 0; i < ITERS; ++i)
        x++;

    accept_roi_end();

    (void)identity(x);

    // loop forever (break here)
    while(1);
}
