#include <enerc.h>

#ifdef __clang__
// work around a bug wherein clang doesn't know that main() belongs in section
// .init9 or that it has to be aligned in a certain way (mspgcc4 introduced
// these things)
__attribute__((section(".init9"), aligned(2)))
#endif
int main() {
    unsigned i = 0; // precise
    volatile APPROX int x = 0;
    while (i++ < 1000) {
        ++x;
    }
    return 17;
}
