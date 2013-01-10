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

int main() {
    int x = 5;
    APPROX int y = 2;
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
