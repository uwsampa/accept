#include <enerc.h>

struct foo {
    APPROX int f;
};

void foofunc(struct foo s) {
}

int precisefunc(int val) {
    val = val + 1;
    return val;
}

APPROX int func(APPROX int val) {
    val = val + 1;
    return val;
}

#ifdef __clang__
// work around a bug wherein clang doesn't know that main() belongs in section
// .init9 or that it has to be aligned in a certain way (mspgcc4 introduced
// these things)
__attribute__((section(".init9"), aligned(2)))
#endif
int main() {
    volatile int x = 5;
    volatile APPROX int y = 2;
    y = y + 1;
    if (x) {
        y = 3;
    }
    y = func(y);
    x = precisefunc(x);
    struct foo bar;
    bar.f = 12;
    foofunc(bar);
    return x;
}
