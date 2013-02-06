#include <enerc.h>

void func(APPROX int *xp) {
    *(xp + 1) = 3;
    APPROX int i = *xp;
}

int main() {
}
