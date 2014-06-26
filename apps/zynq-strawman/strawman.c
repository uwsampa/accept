#include <enerc.h>

int main() {
    int x = 5;
    APPROX int y = 2;
    y = y + 1;
    if (x) {
        y = 3;
    }
    return x;
}
