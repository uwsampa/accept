#include <enerc.h>
#include <stdio.h>

int main() {
    int x = 5;
    APPROX int y = 2;
    y = y + 1;
    if (x) {
        y = 3;
    }
    printf("number: %i\n", y);
    return x;
}
