#include <enerc.h>
#include <stdio.h>

int main() {
    int x = 5;
    APPROX int y = 2;
    accept_roi_begin();
    y = y + 1;
    if (x) {
        y = 3;
    }
    accept_roi_end();
    printf("number: %i\n", y);
    return x;
}
